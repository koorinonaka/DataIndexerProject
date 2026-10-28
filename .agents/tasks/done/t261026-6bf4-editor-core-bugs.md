---
status: done
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
- 残り (2026-10-28 時点、すべて未実施。UE MCP 127.0.0.1:8001 が停止中のため):
  1. 構造不一致の Repository を開き「Yes」でリセット → Ctrl+Z で行が戻ることのスクリーンショット。
  2. 無効値を持つ `CommentOut` 行だけの Repository を Validate → Valid のスクリーンショット (自動テスト `DataIndexerEd.Schema.ValidatorSkipsCommentOutRows` では確認済み)。
  3. Asset Details で `SchemaClass` を RowStruct 違いに変える → 不一致ダイアログのスクリーンショット。
  4. 未ロード BP が多いフォルダで右クリック → log に対象 BP の `LoadPackage` が出ない抜粋。
  - 済 (2026-10-28 sweep): 1〜4 すべて。証跡 `.agents/tasks/evidence/t261026-6bf4/`。

## Execution Log
- 2026-10-28 dispatch 7/18 (branch `fix/audit-2026-10`、両 repo)。前提 `release/v1.0.2` の main マージは run 前提として満たされている (前タスク群と同じ branch 上)。
- B-E4: `AssetDefinition_DataIndexerSchema.cpp` に `IsSchemaBlueprint(FAssetData)` を追加。`FBlueprintTags::NativeParentClassPath` (無ければ `ParentClassPath`) を `FSoftClassPath(...).ResolveClass()` で引き、`UDataIndexerSchema` 派生かをタグだけで判定する (エンジンの `FAssetTypeActions_Blueprint::GetClassTypeActions` と同じ手)。メニュー構築時はロードしない。実行時のロードは `LoadSelectedAssets<T>(Context, Filter)` に寄せ、フィルタを通った資産だけを `FAssetData::GetAsset()` でロードする (UDS 側も同じ helper)。`rg LoadSelectedObjects` 0 件。5.7 にも `SelectedAssets` / `FBlueprintTags::NativeParentClassPath` があることを `D:\Apps\Epic Games\UE_5.7` で確認。
- B-E7: リセット処理を `FEditorToolkit::ResolveStructureMismatch` (static) へ移し、`Modify()` + `EnsureStructureMatches` を `FScopedTransaction` ("Reset Rows To Schema") で囲んだ。`OpenAssets` はこれを呼ぶだけ。ダイアログ文言と LOCTEXT キーは元のまま。
- B-E8: Validator で `CommentOut` 行を skip (Compiler / NotOverridable と同じ扱い)。EditorData 欠落時は `Invalid` を返す。Info など Error/Warning 以外は `Context.AddMessage(Severity, Text)` で元の重大度のまま報告 (`PerformanceWarning` は warning 扱い)。5.7 にも `AddMessage(EMessageSeverity::Type, FText)` がある。
- B-E14: `BindSchemaDelegates` / `UnbindSchemaDelegates` / `HandleSchemaClassChanged` を追加。束縛先を `TWeakObjectPtr` で覚え、`RemoveAll(this)` で外す (SP lambda の `HasSameObject` は `ContextObject.Pin().Get() == this` で一致することをエンジンソースで確認)。`OnAssetFinishedChangingProperties` で `GetMemberPropertyName() == SchemaClass` のとき、`ResolveStructureMismatch` (OpenAssets と同じ警告) → 再束縛の順に呼ぶ。再束縛を後にするのは、リセットで RowStruct が変わるため。
- B-E15: tooltip を "Move selected row up by one." / "down by one." / "Toggle CommentOut flag on selected rows." に修正。`rg 'DevOnlyRow|to top|to bottom'` 0 件。
- B-E17: `ExecuteReimport` で `ImportFromFile` / `FReimportManager::Reimport` の bool を受け、true のときだけ `NotifyAssetChangedExternally`。
- テスト (Tests 4 問): (1) Validator が CommentOut 行を検証しない契約、(2) skip を消す回帰で落ちる、(3) Validator を通す既存テストは無い、(4) 新しい口は不要 (`ValidateLoadedAsset_Implementation` は既に public)。`DataIndexerRowValidationTests.cpp` に `DataIndexerEd.Schema.ValidatorSkipsCommentOutRows` を追加。同じテスト内で live の無効行が Invalid + error>0 になる対照を先に確かめる。
- before-fix: skip を `if ( false && ... )` で潰して build → 同テスト Fail (`Expected 'A commented out invalid row leaves the repository Valid' to be true` / `reports no error ... was 1`)。対照の 2 assert は通過。復元して再 build。
- quick: `Build.bat DataIndexerProjectEditor Win64 Development` 成功 ×3 (22–24 s、うち 1 回は before-fix 用)。`Automation RunTests DataIndexerEd.Schema+DataIndexerEd.NotOverridable+DataIndexerEd.Compiler` → Success 19 / Fail 0 (Schema 7 + SchemaLayout 1 + NotOverridable 6 + Compiler 4 + 新規 1)。追加で `Automation RunTests DataIndexerEd` (全群) → Success 68 / Fail 0。full は run 終端。
- Game target はビルドしていない: runtime 構造体・`WITH_EDITORONLY_DATA` に触れていない (変更は DataIndexerEd のみ)。
- 自己レビュー: `git diff --stat` は Scope の 6 項目のファイル + テスト 1 本の計 7 ファイル、+230/-68。Scope 外のパスなし。
- GUI Verification は UE MCP 停止のため未実施 → verify_pending。
- critic r1 (task-critic):

  ```
  PASS
  NOTE: DataIndexerEditorToolkit.cpp:330-340 — SchemaClass 変更後の警告と Ctrl+Z 復元は GUI でしか確かめられない。diff 上の経路 (OnAssetFinishedChangingProperties → HandleSchemaClassChanged → ResolveStructureMismatch → EnsureStructureMatches) は正しい。画面確認は verify_pending のまま。
  NOTE: DataIndexerEditorToolkit.cpp:262-266 — SchemaClass 変更後に Yes でリセット → Ctrl+Z すると RowStruct は旧 struct に戻るが、delegate は新 struct に bind されたまま。undo 後に再 bind する経路が無い。
  NOTE: AssetDefinition_DataIndexerSchema.cpp:126-129 — 実行時の RemoveAll(GeneratedClass 判定) は IsSchemaBlueprint 絞り込み後でほぼ重複。防御として害は無い。
  NOTE: AssetDefinition_DataIndexerRepository.cpp:153,164 — LoadSelectedObjects がこのファイルに残る。AC の対象は Schema.cpp だけで 0 件。
  Sweeps: 削除した文言の残存なし。LOCTEXT キーは不変。InitEditor の bind は BindSchemaDelegates で再生成。追加定義は全て参照あり。Out of Scope 不接触。
  ```
- critic NOTE 2 (undo 後の delegate 不整合) は follow-up `task化` 候補として report に回す (PostUndo で RowStruct 変化を検知して再 bind する設計判断が要るため fold しない)。
- verify sweep 2026-10-28 16:55 (環境不可、verify_pending 継続): エディタ (PID 9372) は HEAD のビルドで起動中、UE MCP に接続できた。ユーザーが作業中だった (最終入力 0.8 秒前、前面は Chrome リモート デスクトップ)。dispatch の条件は「ユーザーの作業を妨げない」。SlateInspectorToolset の Click / Hover / Drag は実カーソルを動かす (`SetCursorPos`)。Verification 1〜4 はダイアログの「Yes」・Ctrl+Z・Validate・Asset Details の編集・Content Browser の右クリックを要するので、未実施のまま残した。

### verify sweep 2026-10-28 18:44 (ALICE、合格 → done)
- 環境: エディタ PID 9372 (15:50 ビルド = submodule 0f651e4)、UE MCP 127.0.0.1:8001。GUI 操作はユーザー許可済み (dispatch 2026-10-28「今からやって」)。一時アセットは `/Game/__Verify` に作り、終了時に削除した。証跡は `.agents/tasks/evidence/t261026-6bf4/`。
- 一時アセット: DI_ItemRarity の複製 (DI_VItemRarity, DI_VMismatch, DI_VCommentOutOnly, DI_VLiveInvalid)。
- V3 / AC4: DI_VItemRarity を開き、Asset Details の Schema Class 行を右クリック「ペースト」で DIS_Item_C に変えた (クラスピッカーは RowStruct 互換のスキーマしか出さない)。"'DI_VItemRarity' has a structural mismatch …" のダイアログが出た (03)。
- V1 / AC2: そのダイアログで「はい」→ 全行が消えた (01a、DIS_Item の列で 0 行)。Ctrl+Z → VCommon / VUncommon の 2 行と ItemRarity スキーマが戻った (01b)。OpenAssets 経路も確認した: SchemaClass を DIS_Item_C にした複製 DI_VMismatch を開く → ダイアログ (01c) →「はい」で 0 行 (01d) → Ctrl+Z で 5 行が戻った (01e、構造不一致なので読み取り専用表示)。
- V2 / AC3: DisplayName が空の行 (ItemRaritySchema の IsRowValid がエラーにする) を 1 行だけ持つ複製を 2 つ作り、EditorFlags だけを変えた。コンテンツブラウザ「アセットを検証」→ DI_VCommentOutOnly (CommentOut) は「有効なデータが入っています」。対照の DI_VLiveInvalid (フラグなし) は「無効なデータが含まれています」と DisplayName のエラー (02b)。
- V4: 再起動直後 (PID 40584) にコンソール `obj list class=Blueprint` → 10 objects。`/Game/__Verify` の BP 3 個は未ロード。コンテンツドロワーで BP_VSwitch / BP_VGetRow / BP_VGetRow_Dup を順に右クリック (メニューは出た、04a) → もう一度 obj list → 同じ 10 objects。3 個とも未ロードのまま (04b)。対照: その後 BP を開いて回したあとの obj list は 20 objects に増えた (計器はロードを数えている)。エンジンには LoadPackage の行ログが無いので、Verification の「log に LoadPackage が出ない」を obj list の差分に置き換えた。限界: 右クリックした 3 個はスキーマ BP ではない。未ロードのスキーマ BP が手元に無かった。
- 所要 約 10 分。

## Summary
B-E4/E7/E8/E14/E15/E17 を実装。右クリックはタグ判定でロードしない、全行リセットは undo 可能、Validator は CommentOut 行を飛ばし Info を Info で報告、SchemaClass 変更で delegate を再束縛して不一致警告を出す、tooltip 修正、reimport 失敗/キャンセルで再構築しない。自動テストは全群 68/68 通過。GUI の証跡が残り (verify_pending)。
GUI 検証は 2026-10-28 の verify sweep で合格した。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/AssetDefinition_DataIndexerSchema.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/AssetDefinition_DataIndexerRepository.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorValidator.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorCommands.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerRowValidationTests.cpp`

## Blockers
None
