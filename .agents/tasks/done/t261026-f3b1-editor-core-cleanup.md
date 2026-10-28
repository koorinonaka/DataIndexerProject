---
status: done
order: 170
tier: standard
screen: none
agent: any
created_by: task-create
---

# Editor core の死にコード・未使用 include・重複ブロック・static init の整理

## Goal
`DataIndexerEd` の top-level ファイルから、呼び手ゼロの関数、未使用の style 取得と include、重複したメニューオーバーレイ、
毎 open の toolbar 再登録、死んだバージョン分岐、不要な `SaveConfig`、非 inline の namespace-scope FName を消す。挙動は変えない。

## Context
監査 `.agents/audit-2026-10-26.md` の R-E4, R-E5, R-E7, R-E8, R-E9, R-E10, R-E11, R-E13, R-E16, R-E17, P-E6。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/` 相対、行番号は 2026-10-26 時点。`rg` で未使用を確認済み (2026-10-26)。

- R-E4: `DataIndexerEditorToolkit.cpp:479-485`, `.h:52` `HandleMessageTokenClicked` 未バインド。`Misc/UObjectToken.h` / `Logging/TokenizedMessage.h` include もそのためだけ。
- R-E5: `DataIndexerEditorToolkit.h:70-77`, `.cpp:117-122` `UDataIndexerToolMenuContext` を作るが `Toolkit` を読む者なし。
- R-E7: `DataIndexerLogger.cpp:19`, `.h:47` `AppendToLog` 未使用、`IMessageLogListing` include もろとも。
- R-E8: `DataIndexerEditorStyle.cpp:40-51` `ScrollBar` / `NoBorder` / `NormalFont` / `Button` / `NormalEditableTextBoxStyle` / `InvertedForeground` を取得して未使用。
- R-E9: `DataIndexerEditorSettings.cpp:5,9` `#include "FileHelpers.h"` と `using namespace DataIndexer;` 未使用。
- R-E10: `DataIndexerEditorToolkit.cpp:124-249` メニューオーバーレイ 2 分岐が "Schema:" ラベルブロックを重複、末尾 widget だけ違う。
- R-E11: `DataIndexerEditorToolkit.cpp:386-405` toolbar menu/section を toolkit open ごとに再登録 (同名 `AddEntry` 置換で動いているだけ、
  `ToolMenuSection.cpp:57-60`)。`FDataIndexerEdModule::StartupModule` で 1 回。
- R-E13: `AssetDefinition_DataIndexerSchema.cpp` は `UAssetDefinition` を含まず Content Browser メニュー拡張 2 つ。`AssetDefinition_DataIndexerRepository.cpp:216-228`
  と `FDelayedAutoRegisterHelper` + `FToolMenuOwnerScoped` boilerplate 重複、`UnregisterOwner` なし。`DataIndexerContentBrowserMenus.cpp` に改名し
  `StartupModule` / `ShutdownModule` で登録・解除。
- R-E16: `AssetDefinition_DataIndexerRepository.cpp:37-47` `UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)` の else 分岐。`.uplugin` / `.uproject` は 5.8 のみ
  (ただし `Script/ArchivePlugin.bat` は UE_5.7 でもビルドする → **else 分岐は残す**。監査の「死に」は誤り。コメントで 5.7 ビルド用と明記するだけ)。
- R-E17: `DataIndexerEditorToolkit.cpp:45-51` デストラクタで毎回 `UDataIndexerEditorSettings::SaveConfig()`。`SchemaLayouts` が変わる場所
  (`Views/DataIndexerSchemaLayout.cpp` の書き込み関数) で保存。
- P-E6: `DataIndexerEditorStyle.h:22` `DECLARE_STYLE_PROPERTY` が非 `inline` namespace-scope `const FName` → 18 個が include する全 TU で static init。
  `inline const` に (`DataIndexerMetaDataKeys.h:9` と同形)。

## Scope
上の全項目。R-E16 はコメント追記のみ。

## Out of Scope
- EditorData / JsonSupport / Subsystem / Compiler (先行タスクが全部触る)。
- Toolkit の GUI bug (t261026-6bf4、done 済み前提)。

## Preconditions
- t261026-6bf4 が done (`DataIndexerEditorToolkit.cpp` を両方が触る)。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. R-E4 / R-E5 / R-E7 / R-E8 / R-E9 → 削除。
2. P-E6 → `DECLARE_STYLE_PROPERTY` を `inline const` に。
3. R-E10 → ラベルブロックを lambda / 関数に。
4. R-E11 / R-E13 → 登録をモジュール起動時に移し、`ShutdownModule` で `UToolMenus::UnregisterOwner`。ファイル改名。
5. R-E17 → `SaveConfig` を `SchemaLayout` の書き込み関数へ。
6. R-E16 → コメント。
7. full を回す。

## Acceptance Criteria
- `rg 'HandleMessageTokenClicked|UDataIndexerToolMenuContext|AppendToLog|FileHelpers.h' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 各 1 件以上)。
- `rg 'inline const FName' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorStyle.h` が 1 件以上 (現在 0)。
- `rg 'FDelayedAutoRegisterHelper' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 2)。`rg 'UnregisterOwner'` が 1 件以上 (現在 0)。
- `rg 'SaveConfig' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp` が 0 件 (現在 1)。
- `AssetDefinition_DataIndexerSchema.cpp` が存在しない (改名済み)。
- 全自動テストが通り、Repository を開く → 閉じる → 開くで toolbar が 1 組。

## Tests
- 新規テスト行なし。full: `DataIndexerEd` 全群。

## Verification
- full の log: 済 (DataIndexerEd 75 Success / 0 Fail)。
- toolbar 1 組: 済 (2026-10-28 sweep、ALICE)。`/Game/GameData/DI_Items` を開く → 閉じる → 開くを 2 周。各回の toolbar は Reimport 1 + Insert / Copy / Paste / Duplicate / Delete の 5 個。証跡 `evidence/t261026-f3b1/reopen1-toolbar.png`、`reopen2-toolbar.png`。

## Execution Log
- 前提: t261026-6bf4 は verify_pending (検証未完) の上に構築。release/v1.0.2 は main マージ済み (ブランチ fix/audit-2026-10 は main から)。
- R-E4: `HandleMessageTokenClicked` 宣言・定義と、そのためだけの `Misc/UObjectToken.h` / `Logging/TokenizedMessage.h` include を Toolkit.cpp から削除。PCH の `Misc/UObjectToken.h` は他 TU 用に残す。
- R-E5: `UDataIndexerToolMenuContext` と `InitToolMenuContext` override を削除。header に UCLASS が無くなるため `DataIndexerEditorToolkit.generated.h` include と `ToolMenuContext.h` include も削除。
- R-E7: `FLogger::AppendToLog` と `IMessageLogListing` include / 前方宣言を削除。`Logging/TokenizedMessage.h` は `EMessageSeverity` に要るので header に残す。
- R-E8: Style の未使用 6 変数 (ScrollBar / NoBorder / NormalFont / Button / NormalEditableTextBoxStyle / InvertedForeground) を削除。
- R-E9: Settings.cpp の `FileHelpers.h` include と `using namespace DataIndexer;` を削除。
- P-E6: `DECLARE_STYLE_PROPERTY` を `inline const FName` に。
- R-E10: menu overlay を "Schema:" ラベル 1 スロットの box に作り、分岐ごとの末尾スロットを `AddSlot()` で足す形へ。スロット順・属性は不変。
- R-E11: toolbar 拡張を `FEditorToolkit::RegisterToolbar()` (static) に移し、`ExtendMenu("AssetEditor.DataIndexerEditor.ToolBar")` で 1 回だけ足す。名前は engine の `FAssetEditorToolkit::GetToolMenuToolbarName` と同じ (`AssetEditorToolkit.cpp:1319-1324`、非 simple editor なので `GetToolkitFName()`)。`GenerateToolbar` (`:1519-1523`) は未登録の拡張メニューを親付きで登録し直すので、起動時の拡張が先に在っても成立する。旧コードの `RegisterMenu(..., EMultiBoxType::ToolBar)` 分岐は `InitAssetEditor` 後に呼ばれ常に登録済みで死んでいた。`GetToolkitFName` は同じ定数 `ToolkitFName` を返す。
- R-E13: `AssetDefinition_DataIndexerSchema.cpp` を `git mv` で `DataIndexerContentBrowserMenus.cpp` に改名し、Repository 側の Content Browser メニュー (`MenuExtension_DataIndexerRepository`) もここへ移した。2 つの `FDelayedAutoRegisterHelper` を `DataIndexer::ContentBrowserMenus::Register()` 1 本に。`FDataIndexerEdModule::StartupModule` が `UToolMenus::RegisterStartupCallback(CreateRaw(this, &RegisterMenus))`、`RegisterMenus` は `FToolMenuOwnerScoped(this)` 下で ContentBrowserMenus と toolbar を登録。`ShutdownModule` で `UnRegisterStartupCallback(this)` + `UnregisterOwner(this)`。Repository.cpp から不要になった include 3 個 (ContentBrowserMenuContexts / SlateApplication / ToolMenus) を削除。
- R-E16: `#else` 分岐に「UE 5.7 ビルド用で死にではない」コメントを追記のみ。
- R-E17: Toolkit デストラクタの `SaveConfig` を削除 (デストラクタは `= default` で out-of-line に残す)。`FSchemaLayout` に private `Save()` を置き、`ResetLayout` / `SetColumnVisibility` / `SetVisibleDetailCategories` / `SetColumnOrder` で即保存。
- 既定 (R-E17 の幅): `SetColumnWidth` はグリップドラッグ中マウス移動ごとに呼ばれる (`SDataIndexerViewHeader.cpp` `OnWidthChanged` → `FViewModel::HandleColumnWidthChanged`) ので毎回 `SaveConfig` (ini 書き出し) はしない。`bWidthDirty` を立て、`~FSchemaLayout` (ViewModel が持つ。Toolkit を閉じる・schema 変更で解放) で保存する。旧挙動 (Toolkit を閉じたとき保存) と同じ時点。即保存へ変えるなら `SetColumnWidth` に `Save()` を 1 行足すだけ。
- 6bf4 からの申し送り: `AssetDefinition_DataIndexerRepository.cpp` の `LoadSelectedObjects` 2 箇所は `ExecuteExportAsJSON` / `ExecuteImportAsJSON` (メニュー実行時) の中で、メニュー構築時には読まない。修正不要。関数ごと `DataIndexerContentBrowserMenus.cpp` へ移した。
- ビルド: Editor target 3 回。1・2 回目 MSVC C1001 (内部コンパイラエラー、場所は `ContainerAllocationPolicies.h(687)` と `DataIndexerMigrationTests.cpp(15)` で毎回違う)、3 回目 無変更で `Result: Succeeded`。run notes の一過性 C1001 と同じと判断。Game target は editor モジュールしか触っていないので未ビルド。
- テスト: `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests DataIndexerEd; Quit"` → `Result={Success}` 75 / `Result={Fail` 0 / EXIT CODE 0。full は run 終端でも回る。
- 受け入れ grep: `HandleMessageTokenClicked|UDataIndexerToolMenuContext|AppendToLog|FileHelpers.h` 0 件 / `inline const FName` (Style.h) 1 件 / `FDelayedAutoRegisterHelper` 0 件 / `UnregisterOwner` 1 件 (DataIndexerEd.cpp:53) / Toolkit.cpp の `SaveConfig` 0 件 / `AssetDefinition_DataIndexerSchema.cpp` 不在。
- サブモジュール外の呼び手 (`Source/`, `Plugins/DataIndexerToolset`, `docs`) に削除シンボル・旧ファイル名の参照なし (rg 0 件)。
- 自己レビュー (stat): 12 ファイル +217/-282、全て Scope の項目に対応。Toolkit.cpp の 244 行は overlay の組み直しが大半。
- 検証待ち: toolbar 1 組の GUI 目視は UE MCP 停止で不可。
- critic r1:
  ```
  PASS
  NOTE: DataIndexerEditorToolkit.cpp:338 — the AC "open → close → open shows exactly one toolbar set" needs an on-screen check and is still unverified. When checking, confirm that the hardcoded "AssetEditor.%s.ToolBar" matches GetToolMenuToolbarName and that GenerateToolbar registers over the startup extension, so the Reimport and Data sections still appear.
  NOTE: DataIndexerSchemaLayout.cpp:59 — column-width changes are now saved only in ~FSchemaLayout (before: the toolkit destructor). This assumes the ViewModel releases the layout on close or schema change. No other UDataIndexerEditorSettings writer loses its save.
  NOTE: DataIndexerSchemaLayoutTests.cpp:49 — the test writes the user ini twice (SetColumnVisibility, ResetLayout). The final state has no test entry, so this is not a failure.
  NOTE: BugReport.md:304 — this doc still mentions HandleMessageTokenClicked. No code depends on it.
  Sweeps: no removed names remain except that BugReport.md line; AssetDefinition_DataIndexerSchema has no references; LOCTEXT "AssetTypeActions" keys are unchanged; every new definition has a caller; Out of Scope modules are untouched; the R-E16 #else is kept.
  ```
- 2026-10-28 verify sweep (ALICE、UE MCP 127.0.0.1:8001、ユーザー許可の GUI 操作):
  - 手順: MCP `OpenEditorForAsset /Game/GameData/DI_Items` → 状態バー Cmd 欄で `py …AssetEditorSubsystem.close_all_editors_for_asset(…)` → `GetOpenAssets` で閉じたことを確認 → 再度 `OpenEditorForAsset`。これを 2 周した。
  - 1 周目: `cap.ps1` の撮影で toolbar は Reimport / Insert / Copy / Paste / Duplicate / Delete が 1 組 (`reopen1-toolbar.png`)。
  - 2 周目: SlateInspector Snapshot で y=50–95 の button を数えた。`保存, ブラウズ, Reimport, Insert, Copy, Paste, Duplicate, Delete` で、Reimport 1 件・Insert 1 件 (`reopen2-toolbar.png`)。
  - critic NOTE (起動時の拡張と `GenerateToolbar` の両立) は、再オープン後も Reimport と Data の 5 ボタンが出ることで答えが出た。
  - 観察 (範囲外・既存): toolbar 右上の「Schema: <名前>」は Toolkit 生成時の静的テキストで、SchemaClass を変えても更新されない。f3b1 の親コミットでも `FText::FromName( ClassGeneratedBy->GetFName() )` の静的テキストだったので、本タスクの回帰ではない。

## Summary
Editor core の死にコード (R-E4/5/7/8/9)、style FName の static init (P-E6)、overlay 重複 (R-E10)、toolbar / Content Browser メニューの毎 open・個別 auto-register (R-E11/13 → モジュール起動時 1 回 + UnregisterOwner)、Toolkit 閉時の SaveConfig (R-E17 → SchemaLayout 書き込み側) を整理。R-E16 はコメントのみ。全 75 テスト通過。toolbar 1 組は 2026-10-28 の sweep で GUI 確認済み (開く → 閉じる → 開くを 2 周)。

## Changed Files
- Plugins/DataIndexer/Source/DataIndexerEd/Private/AssetDefinition_DataIndexerRepository.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/AssetDefinition_DataIndexerSchema.cpp → DataIndexerContentBrowserMenus.cpp (改名)
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerContentBrowserMenus.h (新規)
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEd.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorSettings.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorStyle.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorStyle.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorToolkit.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerLogger.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerLogger.h
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerSchemaLayout.cpp
- Plugins/DataIndexer/Source/DataIndexerEd/Private/Views/DataIndexerSchemaLayout.h

## Blockers
None
