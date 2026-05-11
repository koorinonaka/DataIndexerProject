---
title: クイックスタート
---

# クイックスタート

スキーマと Repository の作成から行のオーサリング、ランタイムでのクエリまでを一通り説明します。

!!! tip "前提"
    - Unreal Engine 5.7 以降
    - DataIndexer プラグインが有効化済みであること — セットアップは [Unreal Engine でのプラグインの使用方法](https://dev.epicgames.com/documentation/unreal-engine/working-with-plugins-in-unreal-engine) を参照してください。

---

## 1. 構造体とスキーマを作成

=== ":material-puzzle: エディタ & Blueprint"

    **Blueprint 構造体を作成**{ .step-label }

    行データの形状を Blueprint 構造体として Content Browser で定義します。

    ![行データ用の Blueprint 構造体を作成](assets/images/bp-struct-creation.png)

    1. **Content Browser** で右クリック → **Blueprints → Structure**
    2. 名前を付け（例：`S_ItemRow`）、ダブルクリックで開く
    3. データフィールドごとに変数を追加 — `DisplayName`（Text）、`Type`（列挙型）、`BaseValue`（Integer）など

    **Schema Blueprint を作成**{ .step-label }

    Schema Blueprint は構造体を Repository に紐付け、エディタの動作を制御します。

    1. 右クリック → **Blueprint Class**、`DataIndexerSchema` を検索して選択
    2. 名前を付け（例：`BP_ItemSchema`）、ダブルクリックで開く
    3. **Class Defaults → Row Struct** に作成した構造体（`S_ItemRow`）を設定

    ![Schema Blueprint で Row Struct を設定](assets/images/schema-blueprint-creation.png)

    **Get Row Display Name** を実装すると、Data View に各行の読みやすいラベルを表示できます：

    ![GetRowDisplayName の実装例](assets/images/schema-get-row-display-name.png)

    !!! note
        DataTable の RowName に相当する表示はこのロジックが担当します。関数を参照して自動でリネームされるため、常に最新の表示名が維持されます。

=== ":material-language-cpp: C++"

    **行構造体を定義**{ .step-label }

    行データを格納する構造体と、型安全なクエリ用のエイリアスを宣言します：

    ```cpp title="ItemTypes.h"
    #pragma once
    #include "DataIndexerSchemaInterface.h"
    #include "ItemTypes.generated.h"

    UENUM(BlueprintType)
    enum class EItemType : uint8
    {
        Weapon, Armor, Accessory, Material,
    };

    UENUM(BlueprintType)
    enum class EItemRarity : uint8
    {
        Common, Uncommon, Rare, Epic, Legendary,
    };

    USTRUCT(BlueprintType)
    struct FItemRow
    {
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        FText DisplayName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        EItemType Type = EItemType::Weapon;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        EItemRarity Rarity = EItemRarity::Common;

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 BaseValue = 0;
    };

    using FItemInterface = DataIndexer::TNativeSchemaInterface<FItemRow>;
    ```

    **C++ スキーマを実装**{ .step-label }

    `RowStruct` を設定し、`GetRowDisplayName` を実装します：

    ```cpp title="ItemSchema.h"
    #pragma once
    #include "DataIndexerSchema.h"
    #include "ItemSchema.generated.h"

    UCLASS()
    class UItemSchema : public UDataIndexerSchema
    {
        GENERATED_BODY()

    public:
        UItemSchema();

    protected:
        virtual FText GetRowDisplayName_Implementation(
            const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity) const override;
    };
    ```

    ```cpp title="ItemSchema.cpp"
    #include "ItemSchema.h"
    #include "ItemTypes.h"

    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();
    }

    FText UItemSchema::GetRowDisplayName_Implementation(
        const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity) const
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            return Row->DisplayName;
        }

        return Super::GetRowDisplayName_Implementation(PrimaryKey, RowEntity);
    }
    ```

---

## 2. Repository の作成と行のオーサリング

**Repository を作成**{ .step-label }

1. 右クリック → **Miscellaneous → DataIndexer** アセットを選択
2. Pick Class Dialog でスキーマ（例：`BP_ItemSchema` または `UItemSchema`）を選択
3. 名前を付けてアセットを開く

![Details パネルでスキーマをバインドした Repository](assets/images/repository-schema-binding.png)

!!! note "スキーマの変更"
    **Row Struct が一致していれば**、バインド済みスキーマは別のスキーマに変更できます。Row Struct が異なる場合は JSON Export / Import でマイグレーションしてください。

**行をオーサリング**{ .step-label }

Repository アセットをダブルクリックすると Data View が開きます。

![Insert キーで行を追加](assets/images/data-view-add-row.png)

- **Insert** キーで行を追加 — GUID のプライマリキーが自動生成されます
- グリッドでインライン編集するか、行を選択して右側の **Selection Details** パネルで編集
- **保存** — 逆引きテーブルが自動再構築されます

エディタの詳細は [エディタガイド](editor-guide/index.md) を参照してください。

---

## 3. クエリ例

=== ":material-puzzle: エディタ & Blueprint"

    **特定の行を参照する**{ .step-label }

    **Get Row** ノードをグラフにスポーンします：

    1. グラフ上で右クリック → **Get Row** を検索して追加
    2. **Handle Repository** に作成した Repository を設定
    3. **Handle Primary Key** の ComboBox で参照する行を選択
    4. **Out Row** 出力ピンをロジックに接続して使います

    ![Get Row ノードの例](assets/images/get-row-node.png)

    !!! note
        Handle 変数を使ったより複雑なフローは別のドキュメントで紹介します。

    **全行を反復する**{ .step-label }

    **Get All Primary Keys** に Repository を渡して全プライマリキーを取得し、**For Each Loop** で各キーに **Get Row** を呼び出します。

    ![全行を反復するノードグラフ](assets/images/iterate-all-rows.png)

    インデックスによるフィルタリングは [Blueprint API → 関数ライブラリ](api-reference/function-library.md) を参照してください。

=== ":material-language-cpp: C++"

    `FItemInterface`（`TNativeSchemaInterface<FItemRow>` のエイリアス）を使います。

    **特定の行を参照する**{ .step-label }

    ```cpp title="例"
    if (const FItemRow* Row = FItemInterface::FindRow(Repository, PrimaryKey))
    {
        FText Name = Row->DisplayName;
        // ...
    }
    ```

    **全行を反復する**{ .step-label }

    ```cpp title="例"
    for (const FDataIndexerPrimaryKey& Key : FItemInterface::GetAllPrimaryKeys(Repository))
    {
        if (const FItemRow* Row = FItemInterface::FindRow(Repository, Key))
        {
            // ...
        }
    }
    ```

---

## 次のステップ

<div class="grid cards" markdown>

- :material-graph-outline:{ .lg .middle } &nbsp; **[コアコンセプト](concepts/index.md)**

    ---

    Repository / Schema / Keys / Indexes の関係を体系的に理解。

- :material-puzzle-outline:{ .lg .middle } &nbsp; **[Blueprint API](api-reference/index.md)**

    ---

    Blueprint ノード・関数ライブラリ・Driven Collection の完全リファレンス。

- :material-language-cpp:{ .lg .middle } &nbsp; **[C++ API](api-reference/index.md)**

    ---

    型安全なクエリパターンと Native Schema Interface。

- :material-table-edit:{ .lg .middle } &nbsp; **[エディタガイド](editor-guide/index.md)**

    ---

    データビュー・JSON インポート / エクスポート・カスタムウィジェット。

</div>
