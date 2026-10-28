---
status: verify_pending
order: 70
tier: standard
screen: required
agent: any
created_by: task-create
---

# Schema asset editor の Column Layout: unlock を undo 可能にし、UDS 変更にツリーを追随させる

## Goal
Schema の Column Layout パネルで、NotOverridable のロック解除がトランザクション下で行われ (undo 可)、UDS (User Defined Struct) の
フィールド追加・改名・削除・ロック切替後もツリーが最新になる。

## Context
監査 `.agents/audit-2026-10-26.md` の B-C6, B-C7。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_Schema.cpp`、行番号は 2026-10-26 時点。

- B-C6: `:123-127` unlock 経路が `VarDesc->MetaData` を直接消して `OnStructureChanged` を呼ぶ。`ModifyStructData` もトランザクションも無く、
  生きた `FProperty` の meta も消さない。`:125` のコメント (SetMetaData は空値を扱えない、の趣旨) は誤り:
  `FStructureEditorUtils::SetMetaData` は空 `Value` でキー削除 + `Property->RemoveMetaData` + `OnStructureChanged` をトランザクション下で行う
  (エンジン `StructureEditorUtils.cpp:930-958`、2026-10-26 実測)。分岐を `SetMetaData(UDS, Guid, Key, FString())` に置換。
- B-C7: `:617` ツリーを 1 回だけ構築、`:47, :66` で名前・子を捕捉。`FStructureEditorUtils::INotifyOnStructChanged` を購読しない。
  パネルを開いたまま UDS を編集するとツリーが古いまま。ロック切替 (`:121`) 自体が UDS を再コンパイルするのに、パネルは再コンパイル前のツリーを持ち続ける。
  `FSchemaDetails` (名前は実コードに合わせる) でリスナーを実装し、`PostChange` で `PropertyUtilities->ForceRefresh()`。
- 既存テスト `DataIndexerNotOverridableTests.cpp` (5 件) はメタデータの読み書きを覆う。GUI の追随は覆わない。
- 関連 memory: NotOverridable は Schema Column Layout の lock icon に移設済み (`RegisterVariableCustomization` は UDS に届かない)。

## Scope
- unlock 分岐を `FStructureEditorUtils::SetMetaData(..., FString())` に置換し、誤ったコメントを消す。
- `INotifyOnStructChanged` を実装し、対象 UDS の `PreChange` / `PostChange` で `ForceRefresh`。登録解除をデストラクタで行う。

## Out of Scope
- Column Layout の array/map/set 展開 (設計上非対応、memory 済)。
- NotOverridable の意味論・保存形式。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. `D:\Apps\Epic Games\UE_5.7\Engine\Source\Editor\UnrealEd\Private\Kismet2\StructureEditorUtils.cpp` の `SetMetaData` が空 `Value` で
   キー削除するか確認 (5.8 では `:930-958`)。同じなら unlock 分岐を `SetMetaData` に置換。違えば `UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)` で分岐し、
   5.7 側は `ModifyStructData` + トランザクション下で `VarDesc->MetaData.Remove` + `Property->RemoveMetaData`。lock 側が既に `SetMetaData` なら同じ呼び出しに揃える。
2. `FSchemaDetails` に `FStructureEditorUtils::INotifyOnStructChanged` を継承、`FStructureEditorUtils::FStructEditorManager::Get().AddListener(this)` /
   `RemoveListener` をライフサイクルに合わせて呼ぶ。`PostChange` で RowStruct と一致するときだけ `ForceRefresh`。
3. `DataIndexerEd.NotOverridable` を回し、GUI で Verification。

## Acceptance Criteria
- `rg 'MetaData.Remove|MetaData\.Empty' Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_Schema.cpp` が 0 件 (現在 1 件以上)。
- ロック解除 → Ctrl+Z でロックが戻る (現在: 戻らない)。
- Schema パネルを開いたまま UDS にフィールドを足すと Column Layout ツリーに即出る (現在: 閉じて開き直すまで出ない)。

## Tests
- `DataIndexerNotOverridableTests.cpp` に行: unlock 後に `FProperty::HasMetaData(NotOverridable)` が false (現在: `VarDesc` だけ消えて
  `FProperty` には残る → 修正前コードで落ちる)。
- quick: `DataIndexerEd.NotOverridable`。

## Verification
- Undo 後の lock icon と、UDS 編集後のツリーのスクリーンショット各 1 枚。 (残: 両方。UE MCP 停止中)

## Execution Log
- 2026-10-28 前提: 作業ブランチ `fix/audit-2026-10` (両 repo、main から)。`release/v1.0.2` は main にマージ済み (先行タスクと同じ)。
- Step 1: `SetMetaData` の空 `Value` 分岐は 5.7 (`StructureEditorUtils.cpp:978-1007`) と 5.8 (`:930-958`) で同一
  (Transaction + `ModifyStructData` + `VarDesc->MetaData.Remove` + `Property->RemoveMetaData` + `OnStructureChanged`)。版分岐なし。
  `SetFieldNotOverridable` を `SetMetaData( UDS, Guid, Key, bEnabled ? TEXT( "true" ) : FString() )` の 1 呼び出しに統一し、誤コメントを削除。
- Step 2: `FSchemaDetails` が `FStructureEditorUtils::INotifyOnStructChanged` を継承。登録/解除は基底
  `InnerListenerType` の ctor/dtor が `AddListener`/`RemoveListener` を呼ぶ (`ListenerManager.h`、5.7/5.8 同一) ので明示呼び出しは書かない。
  `CustomizeDetails` で RowStruct (UDS のときだけ) と `IPropertyUtilities` を weak で保持し、`PostChange` で一致時のみ更新。
- 既定 (可逆): `ForceRefresh()` ではなく `RequestForceRefresh()` を使う。根拠: `PostChange` は `OnStructureChanged` の中から同期で呼ばれ、
  ロック切替ではその呼び出し元がツリー内チェックボックスのハンドラ。同期 `ForceRefresh` は実行中ウィジェットを破棄する
  (`IPropertyUtilities.h:27` の注記も同趣旨)。観測結果は同じ (次 tick で再構築)。変える費用: 1 行。
  `PreChange` は何もしない (Steps 2 は PostChange のみ指定)。
- Tests 行の差し替え: 指定の「unlock 後に `FProperty::HasMetaData` が false」は修正前コードでも通る。
  `OnStructureChanged` の再コンパイル (`UserDefinedStructureCompilerUtils.cpp:351` `AppendMetaData(VarDesc.MetaData)`) が FProperty を作り直すため。
  修正前ビルドで実測: その assert は緑、`Undo restores the lock` だけ赤。よって判別する行は undo:
  lock → unlock → `GEditor->UndoTransaction()` → lock が戻る。`HasMetaData` は副 assert として残す。Acceptance Criteria は不変。
  テストから届くよう `IsFieldNotOverridable` / `SetFieldNotOverridable` を匿名 namespace から出し、`DataIndexerDetailCustomization_Schema.h` に宣言
  (本番の呼び手はツリーのチェックボックス。同じ関数をそのまま叩くので境界は本物)。
- 修正前 (before-fix, ヘルパ公開+テストのみ): `DataIndexerEd.NotOverridable` 6 Success / 1 Fail
  (`UnlockIsUndoable`: `Expected 'Undo restores the lock' to be true`、`DataIndexerNotOverridableTests.cpp(293)`)。
- 修正後 (after-fix): `Automation RunTests DataIndexerEd.NotOverridable+DataIndexerEd.Schema+DataIndexerEd.SchemaLayout` → 16 Success / 0 Fail、EXIT CODE 0。
  Editor ターゲット Build.bat 2 回とも `Result: Succeeded`。full は run 終端。runtime 構造体に触れないので Game ターゲットは不要。
- 軽微 fold: `FSchemaStructNode` の NOTE コメント (「パネルは古いツリーを持ち続ける」) を「deferred refresh まで」に更新。対象ファイルは Scope 内、2 行。
- 既知の副作用 (GUI 検証で確認): ロック切替のたびに `SSchemaStructTreeWidget` が再構築され、ツリーの展開状態が初期に戻る。
- 範囲外の観察: RowStruct が内包する別 UDS を編集しても `PostChange` の `Changed` はその UDS なのでツリーは更新しない (Scope は「対象 UDS」)。
- Self-review (stat): submodule 3 files (`DataIndexerDetailCustomization_Schema.cpp/.h`, `DataIndexerNotOverridableTests.cpp`)。Scope 外パスなし。
- Critic r1 (task-critic, verbatim):
  ```
  PASS
  NOTE: DataIndexerDetailCustomization_Schema.cpp:122 — AC1 は満たされる。このファイルで `MetaData.Remove|MetaData\.Empty` は 0 件。unlock 経路は `SetMetaData(..., FString())` だけ。
  NOTE: DataIndexerDetailCustomization_Schema.cpp:631 — AC2 (Ctrl+Z でロックが戻る) と AC3 (UDS にフィールドを足すとツリーに即出る) は画面でしか確かめられない。この diff からは判定できない。AC2 は自動テストの `UnlockIsUndoable` (DataIndexerNotOverridableTests.cpp:293) が裏付ける。GUI のスクリーンショット 2 枚は未取得で、タスクの Blockers に記録済み。
  NOTE: DataIndexerDetailCustomization_Schema.cpp:637 — Scope は `ForceRefresh` を指定しているが、実装は `RequestForceRefresh` (遅延) を使う。根拠は Execution Log にある。観測できる振る舞いは同じ。
  NOTE: DataIndexerDetailCustomization_Schema.h:103 — Scope は「登録解除をデストラクタで」と書く。実装は明示的に呼ばず、基底 `INotifyOnStructChanged` (InnerListenerType) の ctor/dtor に任せる。基底が実際にそうするかは確かめていない。根拠は作者の Execution Log (`ListenerManager.h`) だけ。
  NOTE: DataIndexerDetailCustomization_Schema.cpp:107 — `IsFieldNotOverridable` / `SetFieldNotOverridable` を匿名 namespace の外へ出し、ヘッダで公開した。今の呼び手は本番のチェックボックス (:233, :238) とテストの 2 つ。参照ゼロの定義は無い。
  ```
  回答 (NOTE 4): `E:/UE/UE_5.8/Engine/Source/Editor/UnrealEd/Public/Kismet2/ListenerManager.h` の `InnerListenerType()` が `AddListener(this)`、`~InnerListenerType()` が `RemoveListener(this)` を呼ぶことを本文で確認済み。5.7 の同ファイル先頭 90 行は 5.8 と差分なし。
- 検証待ち: UE MCP (127.0.0.1:8001) がこの run で停止中。GUI スクリーンショット 2 枚は取れない。
- verify sweep 2026-10-28 16:55 (環境不可、verify_pending 継続): エディタ (PID 9372) は HEAD のビルドで起動中、UE MCP に接続できた。ユーザーが作業中だった (最終入力 0.8 秒前、前面は Chrome リモート デスクトップ)。dispatch の条件は「ユーザーの作業を妨げない」。SlateInspectorToolset の Click / Hover / Drag は実カーソルを動かす (`SetCursorPos`)。2 枚とも Schema エディタを開き、lock icon のクリック・Ctrl+Z・UDS へのフィールド追加を要するので、未実施のまま残した。

## Summary
unlock を `FStructureEditorUtils::SetMetaData(..., FString())` に置換し、トランザクション下で key 削除 (undo 可) にした。
`FSchemaDetails` が UDS 変更を購読し、RowStruct の再コンパイル後 (undo 含む) に詳細パネルを再構築する。回帰テスト `UnlockIsUndoable` 追加。GUI 検証待ち。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_Schema.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Customizations/DataIndexerDetailCustomization_Schema.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerNotOverridableTests.cpp`

## Blockers
検証待ち: GUI の Verification 2 枚 (Undo 後の lock icon、UDS にフィールド追加後のツリー)。UE MCP 停止中のため未実施。
