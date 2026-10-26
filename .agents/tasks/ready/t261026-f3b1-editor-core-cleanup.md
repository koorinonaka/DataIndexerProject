---
status: ready
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
- full の log。toolbar 1 組はエディタで目視するが証跡はスクリーンショット 1 枚で足りる (screen: none のまま、実行者が撮れれば添える)。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
