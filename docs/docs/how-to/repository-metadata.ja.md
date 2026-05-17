---
title: リポジトリメタデータ
---

# リポジトリメタデータ

DataIndexer はエディタでのリポジトリプロパティの動作を制御するメタデータキーをいくつか提供しています。このページでは最もよく使われる 2 つ、UPROPERTY ピッカーの **スキーマフィルタリング** と、親子階層における **NotOverridable** フィールドロックを説明します。

## スキーマフィルタリング { #schema-filtering }

クラスが `UDataIndexerRepository` の UPROPERTY を持つ場合、デフォルトではアセットピッカーにスキーマに関わらずプロジェクト内のすべてのリポジトリが表示されます。`meta = (Schema = "...")` を追加すると、特定のスキーマを使用するリポジトリのみに絞り込めます。

これにより、アイテムスロットにクエストリポジトリを誤って割り当てたり、ショップにキャラクターリポジトリを設定してしまうミスを防げます。

### C++

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataIndexer",
    meta = (Schema = "/Game/DataIndexer/BP_ItemSchema.BP_ItemSchema"))
TObjectPtr<UDataIndexerRepository> ItemRepository;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DataIndexer",
    meta = (Schema = "/Game/DataIndexer/BP_QuestSchema.BP_QuestSchema"))
TObjectPtr<UDataIndexerRepository> QuestRepository;
```

meta の値は**スキーマ Blueprint** のアセットパスです（リポジトリではありません）。アセットを右クリックして **Copy Reference** で取得できる形式と同じです。

!!! note "パスの形式"
    パスは `/Game/` から始まる絶対コンテンツパスでなければなりません。プラグインコンテンツは `/PluginName/` です。

### Blueprint

1. 変数を持つ Blueprint を開く
2. **My Blueprint** パネルで `UDataIndexerRepository` 変数を選択する
3. **Details** パネルの **Variable** セクションで **Schema** ピッカーを探す
4. スキーマアセットを選択する — ピッカーが一致するリポジトリのみに絞り込まれる

ピッカーをクリアすると絞り込みが解除されます。

### GameMode の例

複数のデータドメインを 1 つの GameMode にまとめる典型的な構成:

```cpp
UCLASS()
class AMyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Data",
        meta = (Schema = "/Game/DataIndexer/BP_ItemSchema.BP_ItemSchema"))
    TObjectPtr<UDataIndexerRepository> ItemRepository;

    UPROPERTY(EditDefaultsOnly, Category = "Data",
        meta = (Schema = "/Game/DataIndexer/BP_CharacterSchema.BP_CharacterSchema"))
    TObjectPtr<UDataIndexerRepository> CharacterRepository;

    UPROPERTY(EditDefaultsOnly, Category = "Data",
        meta = (Schema = "/Game/DataIndexer/BP_QuestSchema.BP_QuestSchema"))
    TObjectPtr<UDataIndexerRepository> QuestRepository;
};
```

各 UPROPERTY のピッカーは対応するスキーマのリポジトリのみを表示します。デザイナーが誤った型を割り当てることができなくなります。

---

## NotOverridable { #notoverridable }

子リポジトリが親行をオーバーライドすると、デフォルトでは子の Data View ですべてのフィールドが編集可能になります。構造体フィールドに `NotOverridable` メタデータを付加すると、そのフィールドはロックされ、子が親行を編集する際に読み取り専用になります。

地域やショップごとに変更すべきでないベースステータス（基本コスト・攻撃力・レアリティ）などに有効です。

### Blueprint（User Defined Struct）

1. 行構造体の **User Defined Struct** エディタを開く
2. ロックしたい変数を選択する
3. **Details** パネルで **Not Overridable** を有効にする

これで、その行をオーバーライドした子リポジトリではこのフィールドが読み取り専用になります。

### C++（ネイティブ構造体）

ネイティブ構造体の場合は、エディタ初期化時に `FStructureEditorUtils::SetMetaData` を使用します。`UDataIndexerSchema` サブクラスや専用のエディタサブシステムで行うのが適切です。

```cpp
#if WITH_EDITOR
void LockBaseStatFields(UUserDefinedStruct* ItemStruct)
{
    for (FGuid VariableGuid : FStructureEditorUtils::GetVarGuids(ItemStruct))
    {
        FName VarName = FStructureEditorUtils::GetVariableFriendlyNameForProperty(
            ItemStruct, FStructureEditorUtils::GetVarDescription(ItemStruct, VariableGuid));

        if (VarName == GET_MEMBER_NAME_CHECKED(FItemRow, BaseValue) ||
            VarName == GET_MEMBER_NAME_CHECKED(FItemRow, Rarity))
        {
            FStructureEditorUtils::SetMetaData(
                ItemStruct, VariableGuid,
                DataIndexer::MetaDataKeys::NotOverridable, TEXT("true"));
        }
    }
}
#endif
```

### プロパティカスタマイズでのフラグ確認

カスタムの `IPropertyTypeCustomization` を書く場合、このフラグを読み取って読み取り専用表示を強制できます。

```cpp
void FMyRowCustomization::CustomizeChildren(
    TSharedRef<IPropertyHandle> StructHandle,
    IDetailChildrenBuilder& Builder,
    IPropertyTypeCustomizationUtils& Utils)
{
    uint32 NumChildren = 0;
    StructHandle->GetNumChildren(NumChildren);

    for (uint32 i = 0; i < NumChildren; ++i)
    {
        TSharedPtr<IPropertyHandle> Child = StructHandle->GetChildHandle(i);
        if (Child->GetProperty()->HasMetaData(DataIndexer::MetaDataKeys::NotOverridable))
        {
            Child->MarkHiddenByCustomization();
        }
        else
        {
            Builder.AddProperty(Child.ToSharedRef());
        }
    }
}
```

### 例: ベースステータスをロックしたアイテムリポジトリ

シナリオ: グローバルアイテムリポジトリが `BaseValue` と `Rarity` を定義している。地域ショップのリポジトリはアイテムをオーバーライドできる（例: 割引フラグの変更）が、ベースステータスは変更不可にしたい。

1. `FItemRow` UDStruct の `BaseValue` と `Rarity` に `NotOverridable` を追加する
2. `DI_AllItems` をグローバルリポジトリとして作成する
3. `DI_AllItems` を親に持つ `DI_ShopA` を作成する（[親リポジトリ階層](parent-hierarchy.md) を参照）
4. `DI_ShopA` で親由来の行を開く — `BaseValue` と `Rarity` がグレーアウトされている

階層の構築方法は [親リポジトリ階層](parent-hierarchy.md) を参照してください。
