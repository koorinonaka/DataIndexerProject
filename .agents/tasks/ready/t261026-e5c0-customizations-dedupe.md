---
status: ready
order: 150
tier: standard
screen: required
agent: any
created_by: task-create
---

# Customizations の重複排除・毎フレーム同期ロードの除去・meta キー集約

## Goal
GraphPin と PropertyTypeCustomization のペアで重複する Schema フィルタ・コンボ選択肢・表示テキスト構築が 1 箇所になり、
details の毎フレーム `LoadObject` / グローバル関数検索 / ハンドル鎖走査が消え、meta キーのリテラルが `DataIndexerMetaDataKeys.h` に集まる。

## Context
監査 `.agents/audit-2026-10-26.md` の R-C3, R-C4, R-C6, R-C7, R-C8, R-C9, R-C11 (残り), P-C3, P-C4, P-C5, P-C6, P-C8, P-C9, P-C10, B-C12。
重複チェック通過 (0件対象)。パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` 相対、行番号は 2026-10-26 時点。

- R-C3: `DataIndexerGraphPin_Repository.cpp:23-56,110-125` ≡ `DataIndexerPropertyTypeCustomization_Repository.cpp:18-100,148-165` Schema meta →
  期待スキーマクラス解決と `ShouldFilterAsset` (AssetRegistry `SchemaClass` タグ + `TryFindTypeSlow`)。
  `DataIndexer::SchemaFilter::{ResolveExpectedSchemaClass(const FString& MetaPath, TConstArrayView<UObject*> Outers), ShouldFilterRepositoryAsset(const FAssetData&, const UClass*)}` に。
- P-C6: `PTC_Repository.cpp:155`, `GraphPin_Repository.cpp:120` フィルタ 1 パス・アセットごとに `TryFindTypeSlow`。`PTC_Repository.cpp:141`
  `GetNewAssetFactoriesForClasses` が `CustomizeHeader` ごとに全 factory 走査。`GraphPin_Repository.cpp:49` ピン Construct で `LoadObject<UClass>` 同期ロード。
  パス→UClass と factory リストをメモ化 (置き場は R-C3 の `SchemaFilter`)。
- R-C4: `GraphPin_IndexKey.cpp:73-137` ≡ `PTC_IndexKey.cpp:69-128`。`GraphPin_PrimaryKey.cpp:133-153` ≡ `PTC_PrimaryKeyOptions.cpp:56-81`。
  `IndexKeyDisplayText(const UDataIndexerRepository*, const FString&)`, `BuildIndexKeyOptions(const UDataIndexerRepository*)`,
  `BuildPrimaryKeyOptions(const UDataIndexerRepository&, TFunctionRef<bool(const FDataIndexerPrimaryKey&)>)` に。
- P-C5: `PTC_PrimaryKeyOptions.cpp:36` (`:68-78` のループ内) 主キー 1 件ごとに `FindObjectSafe<UFunction>`。ループ外へ。
- R-C6: `DataIndexerVariableCustomization_RepositorySchema.cpp:46-77` vs `PTC_Schema.cpp:52-81` スキーマクラス用 `IClassViewerFilter` が 2 つ、
  除外条件 (Abstract/Deprecated/NewerVersionExists/SKEL vs RowStruct) が食い違う。`RequiredRowStruct` optional の 1 クラスに。
- R-C7: `DataIndexerDetailCustomization_EditorRow.cpp:245`, `PTC_NamedStruct.cpp:11` (`"ShowOnlyInnerProperties"`×2)、`DataIndexerRowFunctionBinding.cpp:308`
  (`"AllowFunctionLibraries"`)、`PTC_Schema.cpp:24` (`AssetRegistryTagKeys::RowStructure` を meta キーとして流用)、`PTC_PrimaryKeyOptions.cpp:25`
  (`FScopedFilterContext::FunctionName` = `"FilterFunction"` が runtime の Repository ヘッダにある)。`DataIndexerMetaDataKeys.h` に集約。
- R-C8: `DataIndexerVariableCustomization_PrimaryKey.cpp:289-302` `HandleSelectNone/Variable/Function` は全部 `SetStoredMetaValue` 転送 → 1 つに。
  `:417-443` `BrowseToSelection` の `SMyBlueprint::SelectItemByName` ブロック 2 重。
- R-C9: `DataIndexerDetailCustomization_EditorRow.cpp:254` TODO。監査評価: instanced 登録は `Views/SDataIndexerSelectionDetails.cpp:95-110` のみで
  同型はグローバル layout も持つ → 現状実害なし。advisor 判断: TODO を「許容する制限」に書き換え、実害が出る条件 (per-view だけに登録した
  `ShowOnlyInnerProperties` 構造体) を明記。実装はしない。
- R-C11 (残り): `DataIndexerPrimaryKeyRepositoryResolver.cpp:19` 前方宣言重複、`GraphPin_PrimaryKey.h:24` `DisplayStrings` 未参照
  (`rg` 2026-10-26: 宣言のみ)、`VariableCustomization_RepositorySchema.cpp:227-230, 264-267` (+ `.h:28-29`) no-op / 未バインド。
- P-C3: `DetailCustomization_EditorRow.cpp:754-777 → :167-223, 36-64, 69-87, 105-127` `HandleIsResetToDefaultVisible` が表示行ごとに毎 tick poll
  (エンジン `SResetToDefaultPropertyEditor.cpp:103`) → `GetChildHandle` 確保つきハンドル鎖走査 + 親ごと線形行走査 + パス再構築。
  source Repository と行メモリを `CustomizeDetails` で 1 回解決して保持、行ごとはパス走査だけに。
- P-C4: `VariableCustomization_RepositorySchema.cpp:207` (Text_Lambda), `:240` (CanBrowseTo) 毎フレーム `LoadObject<UClass>`。古いパスだと毎フレーム
  失敗同期ロード + 警告ログ。`CustomizeDetails` / `SetStoredMetaValue` でキャッシュ。
- P-C8: `DetailCustomization_EditorRow.cpp:256` `IsCustomizedStruct` (モジュール検索) がプロパティごと 2 回 + 祖先ごと。ローカル `TMap<const UScriptStruct*, bool>`。
- P-C9: `VariableCustomization_PrimaryKey.cpp:616-623` 毎フレーム線形走査 + `GetName() == StoredValue` 文字列比較。
- P-C10: `DataIndexerRowFunctionBinding.cpp:262,274` `ResolveMember<UFunction>` をバインディングウィジェットごと毎フレーム。バインド済みなら安いので、
  未バインド時だけ結果 (null) をキャッシュして毎フレームの GUID 線形検索を避ける。
- B-C12: `PTC_PrimaryKey.cpp:123`, `PTC_IndexKey.cpp:59` `GetParentHandle()->GetValueData` を null 未確認。Handle 構造体をルートにした
  structure details view で親ハンドルが無い。

## Scope
上の全項目。R-C9 はコメント書き換えのみ。

## Out of Scope
- Resolver の分割 (t261026-de66、done 済み前提)。`ResolveSiblingRepository` はそちらが作ったものを使う。
- K2Node (t261026-dfba)。
- `DataIndexerDetailCustomization_EditorRow.cpp:322-451` `FInstancedStructProvider` (エンジン private クラスのコピー、再利用不可、許容)。

## Preconditions
- t261026-de66 が done (`GraphPin_IndexKey.cpp` / `PTC_PrimaryKeyOptions.cpp` を両方が触る)。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. R-C7 → `DataIndexerMetaDataKeys.h` に定数を足し、リテラルを置換。`"FilterFunction"` は runtime ヘッダの定数を参照する形に。
2. R-C3 + P-C6 → `DataIndexerSchemaFilter.h/.cpp` を新設、メモ化を内蔵。GraphPin / PTC の Repository 2 ファイルから呼ぶ。
3. R-C4 + P-C5 → `DataIndexerKeyOptions.h/.cpp` (名前は実装者が決める) に 3 関数。4 ウィジェットから呼ぶ。
4. R-C6 → `FDataIndexerSchemaClassFilter` 1 クラス。
5. P-C3 / P-C8 → EditorRow details のキャッシュ。P-C4 / P-C9 → VariableCustomization のキャッシュ。P-C10 → 未バインド結果のキャッシュ。
6. R-C8 / R-C11 → 重複と死にコード削除。B-C12 → null ガード。R-C9 → コメント。
7. GUI で Verification。

## Acceptance Criteria
- `rg '"ShowOnlyInnerProperties"|"AllowFunctionLibraries"' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 3)。
- `rg 'TryFindTypeSlow' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` が 1 件 (`SchemaFilter` 内、現在 2)。
- `rg 'LoadObject<UClass>' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerVariableCustomization_RepositorySchema.cpp` の
  呼び出しが lambda 内に無い (現在 2 箇所 lambda 内)。
- `rg 'IClassViewerFilter' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/` の派生クラスが 1 つ (現在 2)。
- `rg 'DisplayStrings' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 1)。
- `rg 'TODO' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_EditorRow.cpp` が 0 件 (現在 1)。
- 1000 行 Repository の行を選択して details を開いたとき、`HandleIsResetToDefaultVisible` 1 回あたり `GetChildHandle` が呼ばれない (カウンタ)。

## Tests
- 自動テストなし (details / Slate)。Tests 4 問: 既存テーブルなし。
- quick: ビルド + `DataIndexerEd.SchemaLayout` (回帰確認)。

## Verification
- Repository ピッカー (ピンと details) が同じ候補を出すスクリーンショット。Schema クラスピッカー 2 箇所が同じ候補を出すスクリーンショット。
- Schema Filter 付き BP 変数 details を開いて静止 → log に `LoadObject` 警告が出ない。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
