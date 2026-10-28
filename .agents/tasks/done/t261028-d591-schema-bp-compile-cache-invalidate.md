---
status: done
order: 220
tier: standard
screen: none
agent: any
created_by: task-create
---

# schema Blueprint の再コンパイルで、Subsystem のコンパイル済み Repository キャッシュを捨てる

## Goal
schema Blueprint の BuildIndex 本体を変えてコンパイルすると、次の `GetOrCreateRepository` が Repository を再コンパイルする。
アセットを開き直さなくても BuildIndex の変更が索引に反映される。

## Context
- 出どころ: t261026-ba24 の critic r1 NOTE 2 (「no-op skip は ComputeHash が compile 入力を網羅する前提。schema BP の BuildIndex 本体は含まない」)。
  ユーザー決定 2026-10-28「BP コンパイル時のキャッシュ無効化は新タスク」。
- 現状 (2026-10-28 時点、submodule `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorSubsystem.cpp`):
  - `CleanRepositories` は `TMap<FObjectKey, FCompiledRepository{Repository, SourceHash, bDirty}>`。キーは元 Repository の `FObjectKey`。
  - `GetOrCreateRepository`: entry があり `bDirty` でなければそのまま返す。dirty なら `UDataIndexerEditorData::ComputeHash()` を取り、
    一致なら古いコピーを再利用する。
  - `ComputeHash` (`DataIndexerEditorData.cpp`) が混ぜるのは、階層の EditorData の CRC、`Schema->GetClass()`、各 Index の `IndexKey` だけ。
    BP を再コンパイルしても generated class のオブジェクトは同じなので、ハッシュは変わらない。だから `bDirty` を立てるだけでは再コンパイルされない。
    **entry を除去する必要がある。**
  - entry 除去の既存関数は `RemoveRepositoriesIn( const UPackage* )` (Reload / 削除 / 改名で使う)。購読は `Initialize`、解除は `Deinitialize`。
  - Compiler は `Repository.GetSchema()` の `CallBuildIndex` で BuildIndex を呼ぶ (`DataIndexerCompiler.cpp`)。schema は `Repository->SchemaClass` から引く。
- engine の購読口 (5.8 / 5.7 とも `Editor/UnrealEd/Classes/Editor/EditorEngine.h` に同じ宣言、両方 `DECLARE_TS_MULTICAST_DELEGATE`):
  - `GEditor->OnBlueprintPreCompile()` — 引数 `UBlueprint*`。どの BP かが分かる。
  - `GEditor->OnBlueprintCompiled()` — 引数なし。コンパイル後。
- 判断 (起票時): 除去の範囲は「schema BP がコンパイルされたらキャッシュ全体を空にする」にする。理由: schema BP のコンパイルは稀で、
  再コンパイルは次のアクセスまで遅延する。どの entry がその schema に依存するか (親 Repository の schema を含む) を辿るコードは足さない。
  schema BP の判定は `Blueprint->GeneratedClass` (無ければ `ParentClass`) が `UDataIndexerSchema` の子かで行う。
- 除去のタイミング: PreCompile で schema BP かを記録し、`OnBlueprintCompiled` で空にする。コンパイル中の再インスタンス化の途中で
  `GetOrCreateRepository` が呼ばれて古い BuildIndex のコピーを作り直す窓を残さないため。実装者が PreCompile だけで足りると確かめたら、そちらでよい。
- 既存テストの型: `DataIndexerCompilerTests.cpp` の `Compiler.NoOpDirtySkipsRecompile` が `UDataIndexerBuildIndexSchemaTests::MatchingCallCount` (fixture の static カウンタ) で
  builder の呼び出し回数を数える。BP を作ってコンパイルする前例は `DataIndexerSearchableNameTests.cpp` (`FKismetEditorUtilities::CreateBlueprint` / `CompileBlueprint`)。
- 重複チェック通過 (23 件対象。最も近い t261026-ba24 で dup 0.25 / conflict 0.27)。

## Scope
- `DataIndexerEditorSubsystem.h/.cpp`: BP コンパイルの購読 (`Initialize` / `Deinitialize`) と、schema BP のときにキャッシュを空にする処理。
  `FailedCompileNotifiedRepositories` も一緒に空にする (コンパイル後に失敗通知をもう一度出せるように)。
- 自動テスト 1 件。

## Out of Scope
- `ComputeHash` に BP のバイトコードを混ぜる案。
- 開いている Data View / グラフピンの再描画。キャッシュの次の取得で新しい索引になることだけを保証する。
- 非 schema BP のコンパイル。

## Preconditions
- t261026-ba24 が done (`FObjectKey` キーの `CleanRepositories` と `RemoveRepositoriesIn`)。

## Steps
1. `UDataIndexerEditorSubsystem` に BP コンパイルの購読を足す (`GEditor` が null の場合 = commandlet 起動を考えて null を確かめる)。
2. PreCompile の BP が schema BP なら印を立て、`OnBlueprintCompiled` で印があれば `CleanRepositories` と `FailedCompileNotifiedRepositories` を空にして印を下ろす。
3. Tests を足し、修正前の木で落ちることを確かめてから 1-2 を入れる。
4. quick (`DataIndexerEd.Compiler` + `DataIndexerEd.Repository`) を回す。

## Acceptance Criteria
- `UDataIndexerBuildIndexSchemaTests` を親にした schema BP を `SchemaClass` に持つ Repository で、`GetOrCreateRepository` → `FKismetEditorUtilities::CompileBlueprint` →
  `GetOrCreateRepository` の 2 回目が builder を再び呼び、1 回目と別のコピーを返す (現在: builder は呼ばれず同じコピー)。
- `UDataIndexerEditorSubsystem` が BP コンパイルのイベントを `Initialize` で購読し、`Deinitialize` で外す (現在: 購読なし)。
- 対照: schema でない BP のコンパイルではキャッシュが残る (Tests の表の 2 行目。今の木でも真なので、回帰の検出用)。

## Tests
自動テスト 1 件 (`DataIndexerEd.Compiler.<名前>`、`DataIndexerCompilerTests.cpp`)。Tests 4 問:
1. 守る契約: schema BP のコンパイル後、キャッシュ済みの Repository は再コンパイルされる。
2. 落ちる回帰: 購読を消す / schema BP の判定を誤る。
3. 既存の `NoOpDirtySkipsRecompile` は行の変更だけを扱い、BP コンパイルの経路を通らない。
4. 本番に無い口は要らない。本物の `FKismetEditorUtilities::CompileBlueprint` が `GEditor` のイベントを流す境界で検査する。

- 表 2 行: (schema BP をコンパイル → 別コピー、builder 呼び出しが増える) / (schema でない BP をコンパイル → 同じコピー、増えない)。
- BP は `FKismetEditorUtilities::CreateBlueprint( UDataIndexerBuildIndexSchemaTests::StaticClass(), ... )` で作る (前例 `DataIndexerSearchableNameTests.cpp`)。
  BuildIndex は native 親のものを使うので、BP のグラフは編集しない。
- 修正前の木で schema BP の行が「同じコピー」で落ちることを確かめる。
- quick: ビルド + `Automation RunTests DataIndexerEd.Compiler+DataIndexerEd.Repository`。

## Verification
- 自動テストの結果 (`Saved/Logs/DataIndexerProject.log` の `Automation Test Succeeded/Failed`) だけで閉じる。GUI は要らない。

## Execution Log
- 2026-10-28 dispatch 23/23。前提 t261026-ba24 は `done/` で `status: done`。両 repo とも `fix/audit-2026-10`、着手時 clean。
- Step 3 (テスト先行): `DataIndexerEd.Compiler.SchemaBlueprintCompileDropsCache` を追加。schema BP は `FKismetEditorUtilities::CreateBlueprint( UDataIndexerIndexedSchemaTests::StaticClass(), … )`
  (fixture は `NotBlueprintable` だが CreateBlueprint はそれで拒否しなかった)。表 2 行: 非 schema BP (`UObject` 親) → 同じコピー・builder 増えない / schema BP → 別コピー・builder 増える。
  BP は保存せず `ObjectTools::DeleteObjects` で後始末。
- 修正前の木 (テストだけ追加) でビルド 23s → `Found 1 automation tests`、Result={Fail}。失敗は schema 行の 2 assert だけ
  (`Schema blueprint: builder runs again` / `Schema blueprint: new copy`、cpp(270)(271))。非 schema 行は通過 —— ハッシュが BP コンパイルで変わらず、キャッシュが残ることを実測で確認。
- Step 1-2: `UDataIndexerEditorSubsystem` に `GEditor->OnBlueprintPreCompile()` / `OnBlueprintCompiled()` の購読を追加 (`Initialize` で `GEditor` null 確認の上で購読、`Deinitialize` で解除)。
  PreCompile で `Blueprint->ParentClass->IsChildOf<UDataIndexerSchema>()` なら印、`OnBlueprintCompiled` で印があれば `CleanRepositories` と `FailedCompileNotifiedRepositories` を空にして印を下ろす。
- 5.7 確認: `D:\Apps\Epic Games\UE_5.7\Engine\Source\Editor\Kismet\Private\BlueprintCompilationManager.cpp` も同じ箇所で `BroadcastBlueprintPreCompile(BP)` (1309) と `BroadcastBlueprintCompiled()` (440 / 1892) を呼ぶ。分岐不要。
- 既定 (可逆): schema BP の判定は `ParentClass` だけで行う (task Context の「`GeneratedClass` (無ければ `ParentClass`)」から簡略化)。理由: `ParentClass` は BP の直接の親で、`IsChildOf` が BP 親の鎖も辿るので結果は同じ。PreCompile 時点では `GeneratedClass` が未生成の新規 BP もある。戻す費用は 1 行。
- 既定 (可逆): 除去は Context どおり `OnBlueprintCompiled` 側。PreCompile だけで足りるかは確かめていない (Context の既定のまま)。
- quick: `Build.bat DataIndexerProjectEditor Win64 Development` 成功 (25s、C1001 なし) → `Automation RunTests DataIndexerEd.Compiler+DataIndexerEd.Repository` = Found 9 / Success 9 / Fail 0 (新テスト含む)。full は run 終端。
- ビルド 2 回 (修正前 1、修正後 1)。
- 自己レビュー: `git diff --stat` = `DataIndexerEditorSubsystem.cpp` +37 / `.h` +8 / `DataIndexerCompilerTests.cpp` +80、計 3 file。すべて Scope 内。Scope 外のパスなし。
- critic r1:
  ```
  PASS
  NOTE: E:\workspace\DataIndexerProject\Plugins\DataIndexer\Source\DataIndexerEd\Private\Tests\Core\DataIndexerCompilerTests.cpp:222 — AC1 says the schema BP is parented to `UDataIndexerBuildIndexSchemaTests`. The test parents it to `UDataIndexerIndexedSchemaTests` instead. That class is a direct subclass of `UDataIndexerBuildIndexSchemaTests` (DataIndexerTestTypes.h:276), and it registers the matching builder. So the BP still descends from the class AC1 names, and the builder counter is still the one the AC refers to. AC1 holds through inheritance. The fixture is `NotBlueprintable`, and the author reports that `CreateBlueprint` accepts it anyway.
  NOTE: E:\workspace\DataIndexerProject\Plugins\DataIndexer\Source\DataIndexerEd\Private\DataIndexerEditorSubsystem.cpp:50 — The schema BP check uses only `ParentClass`, not the Context default (`GeneratedClass`, falling back to `ParentClass`). The Execution Log records this as a reversible choice. `IsChildOf` follows the parent chain, so the result is the same.
  NOTE: E:\workspace\DataIndexerProject\Plugins\DataIndexer\Source\DataIndexerEd\Private\DataIndexerEditorSubsystem.cpp:60 — If a schema BP fails to compile partway, `OnBlueprintCompiled` may never run. The flag then stays set until the next compile of any BP. That compile clears the cache, which costs only a recompile. No defect is visible in the diff.
  NOTE: Kind 3/5 sweep: the diff removes no code identifiers. The new handlers, handles and flag are each bound or used in the diff (cpp:23-24, 35-36, 52, 60-62; h:83-84, 92-93, 96). Out of Scope is not touched: there is no `ComputeHash` change, no view redraw, and no reaction to non-schema BPs. No project CLAUDE.md exists.
  ```

## Summary
schema BP のコンパイル (PreCompile で判定 → Compiled で実行) で Subsystem の Repository キャッシュと失敗通知済み集合を空にした。次の `GetOrCreateRepository` が新しい BuildIndex で再コンパイルする。回帰テスト 1 件 (2 行の表) は修正前に schema 行だけが落ちることを確認済み。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorSubsystem.h`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerEditorSubsystem.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerCompilerTests.cpp`

## Blockers
None
