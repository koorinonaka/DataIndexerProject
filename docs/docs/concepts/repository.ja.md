---
title: Repository
---

# Repository

`UDataIndexerRepository` は `UDataAsset` のサブクラスで、型付きの行コレクションを格納します。DataIndexer ワークフローの主要な成果物です。デザイナーはここで行をオーサリングし、ランタイムコードはここから行をクエリします。

## 親 Repository の合成

Repositoryは 1 つ以上の親Repositoryを参照できます。走査・クエリ時には階層が再帰的に辿られ、親の行は子を通して見えるようになります。

たとえば、全アイテムを持つグローバルRepositoryを親に置き、ショップ画面専用の子Repositoryにはそのサブセットだけを持たせる、といった構成が可能です。行は親側に一か所だけ存在し、子Repositoryは参照するだけなので重複が生じません。

!!! note "循環検出"
    `IncludesRepository(Repository)` で親チェーンの循環を検出します。エディタは循環参照を防ぎます。

### オーバーライドモードと `NotOverridable`

行が親Repositoryに由来し、子の Data View がその行を表示する場合、その行は*オーバーライドモード*に入ります。子で編集するとローカルオーバーライドが作成され、親のデータは変更されません。対応する **User Defined Struct** 変数に `NotOverridable` メタデータを付加することで、このモードで特定フィールドをロックできます。

=== "C++"

    ネイティブ C++ 構造体の場合は `const` またはアクセス制御を使用してください。UDStruct フィールドの場合、カスタマイズレイヤーがこのキーを管理します。

    ```cpp
    // オーバーライドモードで UDStruct 変数をロック
    FStructureEditorUtils::SetMetaData(
        UserDefinedStruct, VariableGuid,
        DataIndexer::MetaDataKeys::NotOverridable, TEXT("true"));

    // プロパティカスタマイズ内で確認
    if (PropertyAndParent.Property.HasMetaData(DataIndexer::MetaDataKeys::NotOverridable))
    {
        // プロパティを読み取り専用でレンダリング
    }
    ```


=== "Blueprint"

    !!! warning "未実装"
        Blueprint からの `NotOverridable` 設定は現在対応していません。今後のリリースで UI を追加予定です。
## Schemaによるピッカーの絞り込み

クラスが `UDataIndexerRepository` UPROPERTY を持ち、特定のSchemaのRepositoryのみに制限したい場合は `meta = (Schema = "AssetPath")` を追加します。エディタはパスを解決し、アセットピッカーを対応するRepositoryのみに絞り込みます。

=== "C++"

    ```cpp
    UPROPERTY(EditDefaultsOnly, Category = DataIndexer,
        meta = (Schema = "/Game/DataIndexer/DA_MySchema.DA_MySchema"))
    TObjectPtr<UDataIndexerRepository> MyRepository;
    ```


=== "Blueprint"

    Blueprint 変数の **Details** パネルを開きます。`UDataIndexerRepository` 変数を選択すると **Schema** ピッカーが表示されます。Schema アセットを選択すると、アセットピッカーが一致するRepositoryのみに絞り込まれます。ピッカーをクリアするとすべてのRepositoryが表示されます。
## シリアライズ

バイナリアセット形式はカスタムの `Serialize` オーバーライドで `LocalEntries`・`EntryOwners`・`ReverseLookups` をシリアライズします。`ReverseLookups` は保存時にSchemaの `BuildIndexFunctions` から再構築されます。

JSON エクスポートはRepositoryの内容を人間可読かつ差分確認に適した形式で出力します。[JSON サポート](../editor-guide/json-support.md) を参照してください。
