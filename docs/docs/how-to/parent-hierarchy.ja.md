---
title: 親リポジトリ階層
---

# 親リポジトリ階層

リポジトリは 1 つ以上の*親*リポジトリを宣言できます。走査・クエリ時には階層が再帰的に辿られ、親の行は子を通して見えるようになります。これにより、行を複製せずに共有ベーステーブルとドメイン固有のオーバーレイを構築できます。

## シナリオ

このガイドでは典型的なアイテムショップのシナリオを使います。

- `DI_AllItems` — ゲーム内の全アイテムを持つグローバルアイテムマスター
- `DI_ShopA` — 特定の NPC ショップの在庫。アイテムの一部を取り扱い、一部の価格をオーバーライドしている

```
DI_AllItems  （親）
     └── DI_ShopA  （子 — 行の追加・価格のオーバーライド）
```

## Step 1 — 親リポジトリを作成する

`DI_AllItems` を通常の手順で作成します。右クリック → **Miscellaneous → DataIndexer → Repository** で作成し、`BP_ItemSchema` をバインドして全ベースアイテム行をオーサリングしてください。

## Step 2 — 子リポジトリを作成して親を設定する

1. 同じスキーマ（`BP_ItemSchema`）で `DI_ShopA` を作成する
2. `DI_ShopA` を Data View で開く
3. **Details** パネルで **Parent Repositories** を探し、`DI_AllItems` を追加する
4. `DI_ShopA` を保存する

Data View に両リポジトリの行が表示されます。親由来の行はエディタテーマに応じて視覚的に区別されます（薄いテキストやロックアイコンなど）。

!!! note "複数の親"
    リポジトリには複数の親を設定できます。すべての親の行が見えるようになり、エディタが循環参照を自動で検出します。

## Step 3 — 子のみの行を追加する

`DI_ShopA` の Data View で **Insert** を押すと、ショップのみに存在する行を追加できます。これらの行は `DI_AllItems` からは見えません。

## オーバーライドモード { #override-mode }

親行のフィールドを変更したい場合（例: セールのための `ShopPrice` 引き下げ）は、子の Data View でその行を編集します。

1. `DI_ShopA` を開く
2. グリッドで親由来の行をクリックする
3. その行が**オーバーライドモード**に入る — フィールドを編集すると `DI_AllItems` を変更せずに `DI_ShopA` の `LocalEntries` にローカルコピーが作られる

オーバーライドされた行は両方に存在します。`DI_AllItems` は元の値を保持し、`DI_ShopA` のローカルコピーが変更後の値を持ちます。`DI_ShopA` をクエリするとオーバーライドされた値が返ります。

オーバーライドを戻すには、行を選択してツールバーの **Reset to Parent** ボタンを押してください。

### 変更不可フィールドのロック

`BaseValue` と `Rarity` を変更不可にしたい場合は、UDStruct でそれらに `NotOverridable` を設定します。子が親行を編集する際、それらのフィールドは読み取り専用のままになります。設定手順は [リポジトリメタデータ → NotOverridable](repository-metadata.md#notoverridable) を参照してください。

## 部分的な在庫管理 { #selective-override }

ショップが `DI_AllItems` の*一部*のアイテムのみを扱いたい場合は、親階層を使わないことをお勧めします — 親の行は常に全件含まれます。

この場合は `DI_ShopA` をスタンドアロンのリポジトリとして管理し、`FDataIndexerHandle` の配列やセカンダリインデックスでショップの在庫を明示的に定義してください。親階層は、全件の可視性とオプションのフィールドオーバーライドが必要なときに使用するものです。サブセット管理には適していません。

## ランタイムクエリ { #runtime-querying }

`DI_ShopA` をクエリすると、子と全親の行が透過的に返ります。特別な処理は不要です。

=== "C++"

    ```cpp
    // GetAllPrimaryKeys は階層全体を走査する
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetAllPrimaryKeys(ShopARepository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(ShopARepository, Key))
        {
            // Row は ShopA（自身またはオーバーライド）か AllItems（親）に由来する
        }
    }
    ```

    元の親の値だけを取得したい場合（例: 元のステータス表示）は、親リポジトリに直接クエリします。

    ```cpp
    if (const FItemRow* BaseRow = FItemInterface::FindRow(AllItemsRepository, Key))
    {
        // 親の元の値
    }
    ```

=== "Blueprint"

    **Get All Primary Keys** に `DI_ShopA` を渡すと、階層全体で見えるすべてのプライマリキーが返ります。各キーを通常通り **Get Row** に渡してください。

    親の元の値と比較したい場合は、同じキーで `DI_AllItems` を別の **Get Row** ノードに渡します。

## 多段階層

合成は再帰的です。`DI_RegionSale` が `DI_ShopA` を親に持ち、`DI_ShopA` がさらに `DI_AllItems` を親に持つ構成も可能です。

```
DI_AllItems
    └── DI_ShopA
            └── DI_RegionSale
```

`DI_RegionSale` をクエリすると全チェーンが走査されます。`DI_ShopA` のオーバーライドは `DI_AllItems` より優先され、`DI_RegionSale` のオーバーライドはさらにその上書きになります。

循環した親参照を誤って作ってしまった場合もエディタのサイクル検出が無限ループを防ぎます。

## 関連ドキュメント

- [リポジトリ — 親 Repository の合成](../concepts/repository.md#parent-repository-composition)
- [リポジトリメタデータ → NotOverridable](repository-metadata.md#notoverridable)
