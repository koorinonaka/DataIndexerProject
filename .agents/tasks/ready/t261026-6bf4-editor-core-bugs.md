---
status: ready
order: 60
tier: standard
screen: required
agent: any
created_by: task-create
---

# Editor core の GUI 経路 bug 修正 (右クリック同期ロード・undo 不能なリセット・Validator・Schema 差し替え)

## Goal
Repository toolkit と AssetDefinition の GUI 経路で、右クリックごとの同期ロード、undo 不能な全行削除、無効化行で止まる保存、
SchemaClass 差し替え後の未追随、誤った tooltip、無駄な再構築を直す。

## Context
監査 `.agents/audit-2026-10-26.md` の B-E4, B-E7, B-E8, B-E14, B-E15, B-E17。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/` 相対、行番号は 2026-10-26 時点。

- B-E4: `AssetDefinition_DataIndexerSchema.cpp:51` `Blueprint_CreateDataIndexerRepository` が Blueprint 右クリックのたびに
  `LoadSelectedObjects<UBlueprint>()` → 未ロード BP を全部同期ロード。`AssetContext->SelectedAssets` の
  `FBlueprintTags::NativeParentClassPath` / `ParentClassPath` タグで判定する。
- B-E7: `AssetDefinition_DataIndexerRepository.cpp:79-81` 「全行を現スキーマにリセット」が `Modify()` + `EnsureStructureMatches` を
  `FScopedTransaction` なしで呼ぶ → 全行削除が undo 不能。
- B-E8: `DataIndexerEditorValidator.cpp:46` `CommentOut` 行を live 行と同じに検証 (`DataIndexerCompiler.cpp:42` と
  `DataIndexerNotOverridable.cpp:156` は非定義扱い)。`:36-41` Error 追加後に `NotValidated` を返す。`:69-72` Info を warning で報告。
- B-E14: `DataIndexerEditorToolkit.cpp:271-296` `OnStructChanged` / `Blueprint::OnChanged` を open 時のスキーマに bind。`:546` は
  `NotifyAssetChangedExternally` しか呼ばないので Asset Details で `SchemaClass` を変えても新スキーマに bind されず、RowStruct 不一致も再検査されない (`OpenAssets` だけが検査)。
- B-E15: `DataIndexerEditorCommands.cpp:33-36` MoveUp/Down の tooltip が "to top/bottom" (実際は ±1、`Toolkit.cpp:359-363`)。`:41` `DevOnlyRow` は存在しない (今は `CommentOut`)。
- B-E17: `DataIndexerEditorToolkit.cpp:450` `ExecuteReimport` がダイアログキャンセル / 失敗でも `NotifyAssetChangedExternally`。

## Scope
上の 6 項目。B-E14 は `SchemaClass` 変更を検知して delegate を bind し直し、RowStruct 不一致なら `OpenAssets` と同じ警告を出す。

## Out of Scope
- `FindRepository` の null ガード B-E6 (t261026-ba24 が EditorData と一緒に直す)。
- JSON import 系 (t261026-25fc)。B-E17 は import 結果の bool を見るだけで、import 本体には触らない。
- Toolkit の死にコード・重複ブロック (t261026-f3b1)。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. B-E7 → `FScopedTransaction` で囲む。
2. B-E4 → `SelectedAssets` のタグ判定に置換。未ロード BP が多い Content Browser で右クリックし、log に `LoadPackage` が出ないことを確認。
3. B-E8 → Validator で `CommentOut` 行を skip、Error 後は `Invalid` を返す、Info は `AddInfo` 系で報告。
4. B-E14 → Toolkit に `HandleSchemaClassChanged` を足す (`:546` の経路から呼ぶ)。古いスキーマの delegate を unbind、新スキーマに bind、
   RowStruct 不一致なら `OpenAssets` と同じ警告を出す。
5. B-E15 → tooltip 文言。
6. B-E17 → import の戻り値が false / キャンセルなら通知を出さない。
7. `DataIndexerEd.NotOverridable` (Validator 周辺) を回し、GUI で Verification。

## Acceptance Criteria
- `rg 'LoadSelectedObjects' Plugins/DataIndexer/Source/DataIndexerEd/Private/AssetDefinition_DataIndexerSchema.cpp` が 0 件 (現在 3)。
- 「全行リセット」の後に Ctrl+Z で行が戻る (現在: 戻らない)。
- 無効な値を持つ `CommentOut` 行だけの Repository が Validate で Valid (現在: Invalid)。
- Asset Details で `SchemaClass` を RowStruct 違いに変えると警告が出る (現在: 出ない)。
- `rg 'DevOnlyRow|to top|to bottom' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorCommands.cpp` が 0 件 (現在 3)。

## Tests
- `DataIndexerRowValidationTests.cpp` に行: `CommentOut` 行の無効値が Validator 結果に出ない。修正前コードで落ちることを確かめる。
- quick: `DataIndexerEd.NotOverridable` + `DataIndexerEd.Compiler`。

## Verification
- AC の GUI 項目 3 つ (Undo 後の表、Validate 結果、SchemaClass 変更後の警告) のスクリーンショット。右クリックは log 抜粋。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
