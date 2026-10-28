---
status: done
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
- 2026-10-28 完了: quick の log で全項目を確認 (Execution Log)。Reload の GUI 挙動は t261026-dd8a へ合流のまま。

## Execution Log
- 2026-10-28 前提: t261026-25fc は done。t261026-21a2 は verify_pending (検証未完) の上に構築。両 repo `fix/audit-2026-10`。
- 実装 (submodule `Source/DataIndexerEd/`):
  - `DataIndexerEditorData.h/.cpp`: `mutable TMap<FPrimaryKey,int32> RowIndexByKey` (UPROPERTY なし) + `RebuildRowIndex()` + `FindRowIndex()`。
    再構築は `PostLoad` (新設 override) と `NotifyRowsChanged` の先頭 (broadcast より前)。`SetRows` / `PostEditChangeProperty` / `PostEditUndo` /
    `NotifyPostChange` / 全 mutator (`FScopedRowsModifier`) はすべて `NotifyRowsChanged` を通るので、呼び出しはこの 2 か所。
    `FindRowIndex` はヒット先のキーを照合し、別行を指していたら `ensure` + 再構築、件数ずれ (PostLoad を通らない複製) は黙って再構築。
  - `ByPredicate` 6 か所 (GetRowMemory / FindRow / GetRowDisplayName / UpdateRow / InsertRow の GUID 重複検査 / InsertRow の BeforeKey) をマップ参照に置換 → 0 件。
  - B-E6: `FindRepository` のラムダで `Get(Repository)` null なら false (Compiler.cpp:67 と同形)。
  - R-E3: `UpdateRow` / `DeleteRows` / `SetRowsCommentOut` はキー不一致なら modifier を開かず return。R-E15: `MoveRow` は行を `RemoveAt + Insert`。
  - `DataIndexerEditorSubsystem.h/.cpp`: `TMap<FObjectKey, FCompiledRepository{Repository, SourceHash, bDirty}>`。FObjectKey は UPROPERTY に置けないため
    `AddReferencedObjects` でコピーを GC から守る。`FailedCompileNotifiedRepositories` も `TSet<FObjectKey>`。`MarkRepositoryDirty` は削除せず bDirty を立てる。
    `GetOrCreateRepository` は dirty なら再入ガード内で `ComputeHash()` → 一致なら再利用、不一致なら再コンパイルし同じハッシュを保存。コンパイル失敗時は entry を消して nullptr (従来と同じ)。
  - Reload: `FCoreUObjectDelegates::OnPackageReloaded` を購読。Content Browser の Reload は `UPackageTools::ReloadPackages` (PackageTools.cpp:638) → `::ReloadPackages`
    (PackageReload.cpp:670-824) が全 phase を broadcast することを確認。fix-up 前の `PrePackageFixup` で `GetOldPackage()` に属する source / 階層を持つ entry を除去。
    `IAssetRegistry::OnAssetRemoved` / `OnAssetRenamed` も同じ除去 (未解決になった key も掃除)。購読は `Initialize`、解除は `Deinitialize`。
    3 デリゲートとも UE 5.7 に同じ宣言あり (UObjectGlobals.h:3347, IAssetRegistry.h:930/937) → 版分岐なし。AssetRegistry は Engine の public 依存で届くので Build.cs 変更なし。
- 既定で進めて記録:
  - `ComputeHash` に `Schema->GetClass()` を混ぜた。根拠: 行が同じでも schema を差し替えると索引の作り方が変わり、ハッシュ一致で古いコピーを再利用してしまう。戻すコストは 1 行。
  - `OnAssetRenamed` でも entry を落とす (Steps どおり)。FObjectKey は rename で変わらないので冗長で、再コンパイルが 1 回増えるだけ。
  - Tests (a) はマップの `Num()` でなく振る舞いで検査 (挿入・先頭削除・移動・BeforeKey 挿入・`SetRows` 1000 行の後に全キーの `GetRowMemory` / `FindRow` が自分の行を返す)。
    `Num()` を見るにはテスト専用の口が要る (Tests 4 問の 4)。
  - no-op recompile の計数は fixture `UDataIndexerBuildIndexSchemaTests::MatchingCallCount` (テスト型の static カウンタ)。
- テスト (Tests 4 問は各テストのコメントに要約):
  - 新規 `Tests/Core/DataIndexerEditorDataRowIndexTests.cpp`: `EditorData.RowLookupFollowsMutations` (a)、`EditorData.MissingKeyIsNoOp` (b、3 関数の表 + 既存キーの対照。
    transient package は dirty にならないので `CreatePackage` の実パッケージ)、`EditorData.FindRepositoryNullAncestorEditorData` (c、自キーの対照つき)。
  - `DataIndexerCompilerTests.cpp`: `Compiler.NoOpDirtySkipsRecompile` (MarkRepositoryDirty 後の再取得で builder 呼び出し 1→1・同一コピー、対照として UpdateRow 後は 2・別コピー)。
- before-fix (テストだけ先に入れ、旧コードでビルド 24s → 3 件指定で実行): `MissingKeyIsNoOp` 3 関数 × (broadcast 1 / dirty) で赤、`NoOpDirtySkipsRecompile` は
  「No-op dirty does not run the builder: expected 1, was 2」「reuses the cached copy」で赤。対照 (既存キーで dirty になる) は旧コードでも緑 = 計器は効いている。
  (c) は旧コードだと null deref で UnrealEditor-Cmd ごと落ち他の結果も消えるため実行しなかった。(a) は旧コードの線形探索が正しいので旧コードでは緑 (設計上)。
  (a) の対照: `NotifyRowsChanged` の `RebuildRowIndex()` を抜いてビルド → `RowLookupFollowsMutations` が `ensure(!FoundIndex)` 「row index points at another row」で Fail。戻して再ビルド。
- after-fix quick: `Build.bat DataIndexerProjectEditor Win64 Development` Succeeded (25s / 23s)。
  `Automation RunTests DataIndexerEd.EditorData+DataIndexerEd.Compiler+DataIndexerEd.Repository` → Found 13、13 Success、EXIT CODE 0。
  追加で `Automation RunTests DataIndexerEd` (全群) → Found 73、73 Success、EXIT CODE 0 (前タスク後 69 + 新規 4)。full は run 終端。
  ログ冒頭の `LogAutomationTest: Error: Condition failed` 19 行は起動時 (frame 0) のもので、修正前の実行でも同数 19。
- Game ターゲット: DataIndexerEd は editor-only で runtime 構造体・`WITH_EDITORONLY_DATA` に触れていないためビルド不要。公開シグネチャ変更なしで Toolset / sample も同じビルドで通過。
- 自己レビュー (stat): submodule 7 ファイル (EditorData.h/.cpp, Subsystem.h/.cpp, CompilerTests, TestTypes.h, 新規 RowIndexTests) — すべて Scope と Tests の範囲。親は task ファイルと submodule pointer のみ。

- critic r1 (task-critic):
  ```
  PASS
  NOTE: DataIndexerEditorDataRowIndexTests.cpp — Tests (a) の Num() 照合は振る舞い検査に置換 (Execution Log に理由)。Tests は AC ではないので FAIL ではない。
  NOTE: DataIndexerEditorSubsystem.cpp:102 — no-op skip は ComputeHash が compile 入力を網羅する前提。schema BP の BuildIndex 本体は含まない。変更前も同じ (新しい穴ではない) がテストなし。
  NOTE: DataIndexerEditorSubsystem.cpp:176 — Reload / asset remove / rename での entry 除去にテストなし。Reload の GUI は t261026-dd8a に合流。
  NOTE: DataIndexerEditorData.cpp:233 — マップ件数と行数がずれると FindRowIndex は黙って再構築。同一キーが 2 行あるとずれ続け、ミスのたびに全再構築になる。
  Checks: ByPredicate 0 / FSoftObjectPath 0 / ComputeHash 呼び手 1 / GetDisplayName→FindRepository の null ガード / UpdateRow 早期 return / 旧 container の外部参照なし / 新関数に呼び手あり / 失敗時・再入時 nullptr 維持。
  ```
  NOTE 4 の扱い: 重複キーは `SetRows` の呼び手が検証する前提 (header コメント) で、正しさは保たれ性能だけが落ちる。KNOWLEDGE 行にも task にもしない (要ユーザー判断 として報告)。

### ユーザー決定 2026-10-28
- 重複キー時の再構築は放置 (critic r1 NOTE 4)。コードもタスクも足さない。
- BP コンパイル時のキャッシュ無効化は新タスク (critic r1 NOTE 2、schema BP の BuildIndex 本体が `ComputeHash` に入らない件):
  t261028-d591 (`ready/t261028-d591-schema-bp-compile-cache-invalidate.md`)。このタスクの status は done のまま。

## Summary
`UDataIndexerEditorData` にキー→index マップを持たせ、行検索 6 か所を O(1) にした。キー不一致の `UpdateRow` / `DeleteRows` / `SetRowsCommentOut` は何もしない。
祖先の EditorData が null でも `FindRepository` は落ちない。`MoveRow` は 1 行の移動。Subsystem のキャッシュは `FObjectKey` キーになり、Reload / 削除 / 改名で entry を落とす。
`MarkRepositoryDirty` 後の取得は `ComputeHash` が一致すれば再コンパイルしない。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Public/DataIndexerEditorData.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorData.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorSubsystem.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorSubsystem.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerEditorDataRowIndexTests.cpp` (新規)
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerCompilerTests.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Fixtures/DataIndexerTestTypes.h`
- `.agents/tasks/done/t261026-ba24-editordata-key-index.md`

## Blockers
None
