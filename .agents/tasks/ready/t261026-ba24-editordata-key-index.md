---
status: ready
order: 100
tier: standard
screen: none
agent: any
created_by: task-create
---

# EditorData にキー→index マップを持たせ、Subsystem キャッシュをオブジェクト同一性キーにする (X1)

## Goal
`UDataIndexerEditorData` の行ルックアップが O(1) になり、`UDataIndexerEditorSubsystem` のコンパイル済みキャッシュが Reload / rename で
古いコピーを返さなくなる。no-op な再コンパイルをハッシュで省く。

## Context
監査 `.agents/audit-2026-10-26.md` の X1 = P-E1, P-E3, P-E4, B-E5, B-E6, R-E3, R-E6, R-E15。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexerEd/Private/` 相対、行番号は 2026-10-26 時点。

- P-E1: `DataIndexerEditorData.cpp:188, 202, 317, 427` 行ルックアップが全部 `FindByPredicate` 線形。キー→index マップなし。
  `DataIndexerCompiler.cpp:106` は index × entry ごとに `FindRow` → O(k·n²)。Views は セルごと `GetRowMemory` / `FindRow`
  (`Views/DataIndexerViewTypes.cpp:158-161`, `Views/SDataIndexerViewRow.cpp:74,99,114`)。変更入口は `PostEditChangeProperty` / `PostLoad` /
  `PostEditUndo` (t261026-21a2 が足す) + `SetRows` (t261026-25fc が足す) の 4 つ。
- B-E6: `DataIndexerEditorData.cpp:133` `Get(Repository)->FindRow(...)` を null 未確認。祖先の EditorData が null だと
  `UDataIndexerRepository::GetDisplayName` (名前解決のたび) でクラッシュ。`DataIndexerCompiler.cpp:67` / `DataIndexerEditorValidator.cpp:37` は同ケースをガード済み。
- R-E3: `DataIndexerEditorData.cpp:424, 472, 479` `UpdateRow` / `DeleteRows` / `SetRowsCommentOut` がキー一致確認前に `FScopedRowsModifier` を開く →
  ミスでも Modify + dirty + キャッシュ無効化 + broadcast。
- R-E15: `DataIndexerEditorData.cpp:384-420` `MoveRow` が 1 行 ±1 のためにキー配列 + TMap + 全体 StableSort。`RemoveAt + Insert`。
- B-E5 / P-E4: `DataIndexerEditorSubsystem.cpp:40, 62, 67, 78, 84` `CleanRepositories` が `FSoftObjectPath` キー。Reload / SCC sync 後も古い
  コンパイル済みコピー (旧インスタンスを指す `EntryOwners` / `ParentRepositories`) を返す。rename で孤児化。閉じたアセットのコピーも永久保持。
  lookup ごとに `FSoftObjectPath` を 2〜3 個構築。`FObjectKey` にし、Content Browser の Reload で発火するデリゲート
  (`FCoreUObjectDelegates::OnPackageReloaded`。`OnObjectsReplaced` は reinstancing 用なので実装者が Reload で実際に発火する方を確認) と
  Asset Registry の `OnAssetRemoved` / `OnAssetRenamed` で該当 entry を落とす。
- P-E3 / R-E6: `DataIndexerCompiler.cpp:85-124` 行×index ごとに BP VM 呼び出しを game thread 同期で、`GetOrCreateRepository` を呼んだ経路の中で実行。
  `MarkRepositoryDirty` が undo / コメント切替含む全 `PostEditChangeProperty` で無効化。`ComputeHash` (`DataIndexerEditorData.cpp:143`,
  `DataIndexer/Public/DataIndexerEditorInterface.h:89`) は存在するが呼び手ゼロ (`rg` 2026-10-26)。advisor 判断: 削除でなく使う →
  Subsystem がコンパイル時のハッシュを entry に持ち、`MarkRepositoryDirty` は「dirty 候補」にするだけ、`GetOrCreateRepository` でハッシュ一致なら再利用。

## Scope
- `UDataIndexerEditorData` に `TMap<FDataIndexerPrimaryKey, int32> RowIndexByKey` (Transient、UPROPERTY 不要) を持たせ、4 つの変更入口で再構築。
  `FindRow` / `GetRowMemory` / `UpdateRow` / `DeleteRows` / `SetRowsCommentOut` / `InsertRow` / `MoveRow` がそれを使う。
- B-E6 の null ガード。R-E3 の検査順序。R-E15 の `RemoveAt + Insert`。
- Subsystem のキーを `FObjectKey` に、リロード/削除/改名で entry 除去、ハッシュによる no-op recompile skip。

## Out of Scope
- Views 側のマップ (t261026-dd8a)。Views は EditorData の `FindRow` を呼ぶだけなので本タスクで速くなる。
- Compiler の非同期化。
- `NotifyRowsChanged` の統合 (t261026-21a2)。`SetRows` の新設 (t261026-25fc)。両方 done の前提で、マップ再構築をそこに足す。

## Preconditions
- t261026-21a2 (PostEditUndo) と t261026-25fc (SetRows) が done。
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。

## Steps
1. `DataIndexerEditorData.h/.cpp` に `RowIndexByKey` と `RebuildRowIndex()` を追加。`PostLoad` / `NotifyRowsChanged` / `SetRows` / `PostEditUndo` から呼ぶ。
2. `FindByPredicate` / `IndexOfByPredicate` でキー検索している箇所をマップ参照に置換 (`rg 'ByPredicate' DataIndexerEditorData.cpp` で全部拾う)。
3. B-E6 → `Get(Repository)` の null ガード (`Compiler.cpp:67` と同形)。
4. R-E3 → 3 関数で先に一致判定、無ければ早期 return。R-E15 → `MoveRow` を `RemoveAt + Insert`。
5. `DataIndexerEditorSubsystem` のキーを `FObjectKey` に、`OnPackageReloaded` (要確認) / `OnAssetRemoved` / `OnAssetRenamed` を購読 (Subsystem の
   `Deinitialize` で解除)。entry に `uint32 SourceHash` を持ち、`GetOrCreateRepository` で `ComputeHash()` 一致なら dirty を解除して再利用。
6. `DataIndexerEd.EditorData` + `DataIndexerEd.Compiler` + `DataIndexerEd.Repository` を回す。

## Acceptance Criteria
- `rg 'ByPredicate' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorData.cpp` の行検索用途が 0 件 (現在 4 件以上)。
- `UpdateRow` に存在しないキーを渡しても `OnEditorRowsUpdated` が発火せず package も dirty にならない (現在: 両方起きる)。
- 祖先の EditorData が null の Repository で `GetDisplayName` が落ちない (現在: クラッシュ)。
- `rg 'FSoftObjectPath' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorSubsystem.cpp` が 0 件 (現在 1 件以上)。
- 行を変えずに `MarkRepositoryDirty` → `GetOrCreateRepository` を呼ぶと `CallBuildIndex` が呼ばれない (現在: 毎回呼ばれる)。
- `ComputeHash` の呼び手が 1 件以上 (現在 0)。

## Tests
- `DataIndexerEditorFunctionLibraryTests.cpp` か `DataIndexerEd.EditorData` 群に行: (a) 1000 行で `FindRow` が index 経由
  (マップの `Num()` == 行数)、(b) 存在しないキーの `UpdateRow` で broadcast 回数 0、(c) 祖先 EditorData null で `FindRepository` が nullptr。
  修正前コードで落ちることを確かめる (c はクラッシュ)。
- `DataIndexerCompilerTests.cpp` に行: no-op dirty → recompile skip (fixture の builder 呼び出し回数を数える)。
- quick: `DataIndexerEd.EditorData` + `DataIndexerEd.Compiler` + `DataIndexerEd.Repository`。

## Verification
- quick の log。GUI 不要。Content Browser の Reload 後の挙動は t261026-dd8a の GUI 確認に合流。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
