---
title: クイックスタート
---

# クイックスタート

スキーマと Repository の作成から行のオーサリング、ランタイムでのクエリまでを一通り説明します。

!!! tip "前提"
    本ガイドは [インストール](installation.md) が完了していることを前提とします。プラグインの組み込みがまだの方は、先にそちらを実施してください。

---

## 1. 構造体とスキーマを作成

=== ":material-puzzle: エディタ & Blueprint"

    **Blueprint 構造体を作成**

    行データの形状を Blueprint 構造体として Content Browser で定義します。

    ![行データ用の Blueprint 構造体を作成](assets/images/bp-struct-creation.png)

    1. **Content Browser** で右クリック → **Blueprints → Structure**
    2. 名前を付け（例：`S_ItemRow`）、ダブルクリックで開く
    3. データフィールドごとに変数を追加 — `DisplayName`（Text）、`Type`（列挙型）、`BaseValue`（Integer）など

    **Schema Blueprint を作成**

    Schema Blueprint は構造体を Repository に紐付け、エディタの動作を制御します。

    1. 右クリック → **Blueprint Class**、`DataIndexerSchema` を検索して選択
    2. 名前を付け（例：`BP_ItemSchema`）、ダブルクリックで開く
    3. **Class Defaults → Row Struct** に作成した構造体（`S_ItemRow`）を設定

    ![Schema Blueprint で Row Struct を設定](assets/images/schema-blueprint-creation.png)

    **Get Row Display Name** を実装すると、Data View に各行の読みやすいラベルを表示できます：

    ![GetRowDisplayName の実装例](assets/images/schema-get-row-display-name.png)

=== ":material-language-cpp: C++"

    **行構造体を定義**

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

    **C++ スキーマを実装**

    `DI_DEFINE_INDEX` でインデックスを宣言し、コンストラクタでビルド関数を登録します：

    ```cpp title="ItemSchema.h"
    #pragma once
    #include "DataIndexerKeyHelpers.h"
    #include "DataIndexerSchema.h"
    #include "ItemSchema.generated.h"

    UCLASS()
    class UItemSchema : public UDataIndexerSchema
    {
        GENERATED_BODY()

    public:
        UItemSchema();

        DI_DEFINE_INDEX(ByTypeIndex);
        DI_DEFINE_INDEX(ByRarityIndex);
        DI_DEFINE_INDEX(ByTypeAndRarityIndex);

    protected:
        virtual FText GetRowDisplayName_Implementation(
            const FDataIndexerPrimaryKey& PrimaryKey, const FInstancedStruct& RowEntity) const override;

        UFUNCTION()
        static FGuid BuildTypeIndex(const FInstancedStruct& RowEntity);

        UFUNCTION()
        static FGuid BuildRarityIndex(const FInstancedStruct& RowEntity);

        UFUNCTION()
        static FGuid BuildTypeAndRarityIndex(const FInstancedStruct& RowEntity);
    };
    ```

    ```cpp title="ItemSchema.cpp"
    #include "ItemSchema.h"
    #include "ItemTypes.h"

    UItemSchema::UItemSchema()
    {
        RowStruct = FItemRow::StaticStruct();

        RegisterFunction_BuildIndex(ByTypeIndex(),          GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeIndex));
        RegisterFunction_BuildIndex(ByRarityIndex(),        GET_FUNCTION_NAME_CHECKED(ThisClass, BuildRarityIndex));
        RegisterFunction_BuildIndex(ByTypeAndRarityIndex(), GET_FUNCTION_NAME_CHECKED(ThisClass, BuildTypeAndRarityIndex));
    }

    FGuid UItemSchema::BuildTypeIndex(const FInstancedStruct& RowEntity)
    {
        if (const FItemRow* Row = RowEntity.GetPtr<const FItemRow>())
        {
            return FGuid(static_cast<uint32>(Row->Type), 0, 0, 0);
        }
        return {};
    }
    ```

    !!! tip "Blueprint スキーマ"
        プロトタイプ段階では `UDataIndexerSchema` を Blueprint でサブクラス化し、**Class Defaults → Build Index Functions** からインデックス関数を登録できます。サブシステムでコンパイル時アクセスが必要になったら C++ スキーマに移行してください。

---

## 2. Repository の作成と行のオーサリング

**Repository を作成**

1. 右クリック → **Miscellaneous → Data Asset**、`DataIndexerRepository` を選択
2. 名前を付け（例：`DI_Items`）、ダブルクリックで開く
3. **Details** パネルの **Schema Class** にスキーマ（例：`BP_ItemSchema` または `ItemSchema`）を設定

![Details パネルでスキーマをバインドした Repository](assets/images/repository-schema-binding.png)

!!! warning "スキーマのバインドは変更不可"
    行を追加する前に **Schema Class** を設定してください。後から変更すると既存の行データがすべて無効になります。

**行をオーサリング**

Repository アセットをダブルクリックすると Data View が開きます。

![Insert キーで行を追加](assets/images/data-view-add-row.png)

- **Insert** キーで行を追加 — GUID のプライマリキーが自動生成されます
- グリッドでインライン編集するか、行を選択して右側の **Selection Details** パネルで編集
- **保存** — 逆引きテーブルが自動再構築されます

エディタの詳細は [エディタガイド](editor-guide/index.md) を参照してください。

---

## 3. クエリ例

=== ":material-puzzle: エディタ & Blueprint"

    **特定の行を参照する**

    Actor またはコンポーネントに `FDataIndexerRowHandle` 変数を宣言します。Details パネルで Repository の特定の行を設定します。

    Blueprint グラフでは **Get Row** ノードを使います：

    1. 行ハンドル変数からドラッグして **Get Row** を検索
    2. ノードの Details パネルで **Schema Class** にスキーマを設定
    3. `Row` 出力ピンが構造体型に解決されます — ロジックに接続して使います

    **全行を反復する**

    DataIndexer 関数ライブラリの **Get All Primary Keys** に Repository を渡すと全プライマリキーが取得できます。各キーに対して **Get Row** を呼び出して行データを取得します。

    インデックスによるフィルタリングは [Blueprint API → 関数ライブラリ](api-reference/function-library.md) を参照してください。

=== ":material-language-cpp: C++"

    `UEngineSubsystem`（またはシングルトンにアクセスできる任意のクラス）でリポジトリアクセスをラップします：

    ```cpp title="GameDataSubsystem.cpp"
    // プライマリキーで直接取得
    const FItemRow* UGameDataSubsystem::GetItem(const FDataIndexerPrimaryKey& Key) const
    {
        const UGameDataSettings* Settings = UGameDataSettings::Get();
        if (!Settings || !Settings->ItemRepository) return nullptr;

        return FItemInterface::FindRow(*Settings->ItemRepository, Key);
    }

    // 順引き — 指定タイプのアイテムをすべて取得
    TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByType(EItemType Type) const
    {
        const UGameDataSettings* Settings = UGameDataSettings::Get();
        if (!Settings || !Settings->ItemRepository) return {};

        FItemRow Query;
        Query.Type = Type;
        return FItemInterface::GetPrimaryKeys(*Settings->ItemRepository, UItemSchema::ByTypeIndex(), Query);
    }

    // 複合インデックス — タイプとレアリティの同時指定
    TArray<FDataIndexerPrimaryKey> UGameDataSubsystem::GetItemsByTypeAndRarity(EItemType Type, EItemRarity Rarity) const
    {
        const UGameDataSettings* Settings = UGameDataSettings::Get();
        if (!Settings || !Settings->ItemRepository) return {};

        FItemRow Query;
        Query.Type   = Type;
        Query.Rarity = Rarity;
        return FItemInterface::GetPrimaryKeys(*Settings->ItemRepository, UItemSchema::ByTypeAndRarityIndex(), Query);
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
