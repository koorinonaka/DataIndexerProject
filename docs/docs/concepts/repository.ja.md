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

    UPROPERTY に `meta = (NotOverridable)` を付加します。Override 行では、このメタが付いたプロパティは読み取り専用になります。

    ```cpp
    UPROPERTY(EditDefaultsOnly, Category = DataIndexer, meta = (NotOverridable))
    FString InheritedReadOnlyField;
    ```


=== "Blueprint"

    !!! warning "未実装"
        Blueprint からの `NotOverridable` 設定は現在対応していません。今後のリリースで UI を追加予定です。
## Schemaによるピッカーの絞り込み

クラスが `UDataIndexerRepository` UPROPERTY を持ち、特定のSchemaのRepositoryのみに制限したい場合は `meta = (Schema = "...")` を追加します。UPROPERTY のほかに UFUNCTION も指定できます。エディタはスキーマを解決し、アセットピッカーを対応するRepositoryのみに絞り込みます。

=== "C++"

    C++ では、アセットパスではなく同クラス上の **UPROPERTY 名** または **UFUNCTION 名** を指定します。

    **UFUNCTION で指定する場合**

    ```cpp
    UPROPERTY(EditDefaultsOnly, Category = DataIndexer, meta = (Schema = "GetSchema"))
    TObjectPtr<UDataIndexerRepository> MyRepository;

    UFUNCTION()
    UDataIndexerSchema* GetSchema() const;
    ```

    **UPROPERTY で指定する場合**

    ```cpp
    UPROPERTY(EditDefaultsOnly, Category = DataIndexer, meta = (Schema = "MySchema"))
    TObjectPtr<UDataIndexerRepository> MyRepository;

    UPROPERTY(EditDefaultsOnly, Category = DataIndexer)
    TObjectPtr<UDataIndexerSchema> MySchema;
    ```


=== "Blueprint"

    Blueprint 変数の **Details** パネルを開きます。`UDataIndexerRepository` 変数を選択すると **Schema** ピッカーが表示されます。Schema アセットを選択すると、アセットピッカーが一致するRepositoryのみに絞り込まれます。ピッカーをクリアするとすべてのRepositoryが表示されます。
## シリアライズ

バイナリアセット形式はカスタムの `Serialize` オーバーライドで `LocalEntries`・`EntryOwners`・`ReverseLookups` をシリアライズします。`ReverseLookups` は保存時にSchemaの `BuildIndexFunctions` から再構築されます。

JSON エクスポートはRepositoryの内容を人間可読かつ差分確認に適した形式で出力します。[JSON サポート](../editor-guide/json-support.md) を参照してください。
