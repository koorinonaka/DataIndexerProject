---
status: ready
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
- Undo 後の lock icon と、UDS 編集後のツリーのスクリーンショット各 1 枚。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
