---
status: done
order: 80
tier: standard
screen: required
agent: any
created_by: task-create
---

# FDataIndexerPrimaryKey::OwnerRepository の HACK を解消する (X7)

## Goal
`FDataIndexerPrimaryKey` が runtime で 16 byte に戻り、EdGraphPin 由来のキーの searchable name が保存順序に依存せず記録される。

## Context
監査 `.agents/audit-2026-10-26.md` の X7 = B-R2 + P-R4。重複チェック通過 (0件対象)。
パスは `Plugins/DataIndexer/Source/DataIndexer/` 相対、行番号は 2026-10-26 時点。

- `Public/DataIndexerTypes.h:75` `TWeakObjectPtr<UDataIndexerRepository> OwnerRepository` が `WITH_EDITORONLY_DATA` 外にある →
  cooked でも全 key が 16→24 byte (`LocalEntries` / `EntryOwners` / `ReverseLookups` のキー、全 handle に効く)。
- `Private/DataIndexerTypes.cpp:166-174` `PostSerialize` の `// HACK!!`: `OwnerRepository` を `ImportTextItem` で解決し、保存時に
  `MarkSearchableName` で 1 回使って `Reset()`。コメントは `UK2Node::FixupPinStringDataReferences` が ImportText 直後に serialize する前提。
  リスク: 判定が `Ar.IsSaving()` だけなので、transaction の `FObjectWriter` や `FDuplicateDataWriter` が ImportText と本保存の間に通ると消費され、
  本保存で searchable name が落ちる。pin 経路 (`K2Node.cpp:245-250`、`ImportText` 直後に `SerializeItem`) では隣接するので実害は出にくいが脆い。
- advisor 判断 (2 段階):
  - Tier 1 (必ずやる): メンバを `#if WITH_EDITORONLY_DATA` で囲む。`Export/ImportTextItem` 側の参照も同じガード。
  - Tier 2 (試す): メンバを消し、`PostSerialize` の else 分岐で `FUObjectThreadContext::Get().GetSerializeContext()->SerializedObject`
    (= ImportText の `Parent` と同じ K2Node) を既存の `ResolveOwnerRepository` 相当のデリゲートに渡して都度解決。`Ar.IsPersistent()`
    (または `Ar.GetLinker()`) で絞り、transaction アーカイブでロードが走らないようにする。
  - Tier 2 で AC (b) が落ちるなら Tier 1 で止め、理由を `Execution Log` に残す。
- `MarkSearchableName` (`:180-193`) は `IDataIndexerInterface_EditorData::GetEditorData → FindRepository` で解決済み Repository を使う。
  この経路は変えない。

## Scope
- `DataIndexerTypes.h/.cpp` の `OwnerRepository` とその参照 (`ImportTextItem`, `PostSerialize`, `FindOwnerRepository`)。
- 非エディタ構成で `static_assert(sizeof(FDataIndexerPrimaryKey) == 16)` を `DataIndexerTypes.cpp` に置く (`#if !WITH_EDITORONLY_DATA`)。

## Out of Scope
- `GetRowDisplayName` の契約 (t261026-2e18、同ファイル別関数)。
- `WITH_EDITOR` / `WITH_EDITORONLY_DATA` の規則統一 (t261026-a134)。本タスクは新しいガードを 1 箇所足すだけで、既存のガードは触らない。
- K2Node 側の pin 処理。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。
- 先行タスクなし (t261026-2e18 とはファイルは同じだが関数が別。両方 in_progress にしない運用は executor が order で守る)。

## Steps
1. Tier 1: メンバと参照を `WITH_EDITORONLY_DATA` で囲み、`static_assert` を足す。Game ターゲット (`DataIndexerProject Win64 Development`) をビルド。
2. GetRow ノードの PrimaryKey ピンに default を入れた BP を保存 → Asset Registry の dependencies に Repository が出ることを確認 (Reference Viewer)。
3. Tier 2: メンバを消し、`PostSerialize` を `SerializedObject` 経由の都度解決に書き換え、`Ar.IsPersistent()` で絞る。
4. 同じ BP で: default 入力 → Ctrl+Z → Ctrl+Y → 保存 → Reference Viewer に Repository が残る。Duplicate した BP でも残る。
5. 落ちるなら Tier 2 を revert し Tier 1 で止める。
6. `DataIndexerEd.SearchableName` 群を回す。

## Acceptance Criteria
- (a) `!WITH_EDITORONLY_DATA` で `sizeof(FDataIndexerPrimaryKey) == 16` の `static_assert` が通る (現在: 24 で落ちる)。
- (b) PrimaryKey ピンに default を持つ GetRow ノード入り BP を undo/redo 後に保存しても Reference Viewer に Repository が出る。
- Tier 2 まで行けた場合: `rg '\bOwnerRepository\b' Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerTypes.h` が 0 件 (現在 1 件以上)。
  (2026-10-28 PLAN 修復: 元の `rg 'OwnerRepository'` は delegate 名 `ResolveOwnerRepository` / `FindOwnerRepository` 等にも当たり、
  メンバを消しても 0 件にならない。delegate の改名は Scope 外の公開 API 変更。意図 = メンバが消えたこと、は語単位の検索で同じに判定できる。)

## Tests
- `DataIndexerSearchableNameTests.cpp` (750 行、既存 fixture あり) に行: `FObjectWriter` 相当の非 persistent アーカイブで `PostSerialize` を通した後、
  persistent 保存で searchable name が記録される。Tier 1 のみなら既存行の回帰確認だけ。
- quick: `DataIndexerEd.SearchableName` 相当の群 (実際の群名は `rg IMPLEMENT_.*SearchableName` で確認)。Game ターゲットもビルド。

## Verification
- 残り: Reference Viewer のスクリーンショット (undo/redo 後の保存、Duplicate 後)。
- 済: Game ターゲットのビルド (修正前 static_assert 赤 → 修正後 Succeeded)。
- 済 (2026-10-28 sweep): Reference Viewer (undo/redo 後の保存、Duplicate 後)。証跡 `.agents/tasks/evidence/t261026-91cd/`。

## Execution Log
2026-10-28 (dispatch 9/18、branch `fix/audit-2026-10`、両 repo)。

- 前提: `release/v1.0.2` は main にマージ済み (この run の先行タスクと同じブランチ上)。先行タスク依存なし。
- 調査:
  - `UK2Node::Serialize` (5.8 `K2Node.cpp:128`) は `Ar.IsObjectReferenceCollector() || Ar.Tell() < 0` のときだけ
    `FixupPinStringDataReferences(&Ar)` を呼ぶ。中身は一時 `FStructOnScope` に `ImportText` → `SerializeItem(*SavingArchive, …, nullptr)`。
    property 無しなので `PostSerialize` は else 分岐に入る。
  - 保存時にこれを呼ぶのは `FPackageHarvester` (`PackageHarvester.cpp:219-221`: `SetIsSaving(true)` / `SetIsPersistent(true)` /
    `ArIsObjectReferenceCollector = true`)。`ProcessExport` が `FScopedObjectSerializeContext(Export, *this)` で
    `SerializedObject` = export (K2Node) を立てる (`:545`)。5.7 も同じ (`PackageHarvester.cpp:220-221,545`、`K2Node.cpp:128,250`)。
- Tier 1 (赤の確認だけ): `#if !WITH_EDITORONLY_DATA` の `static_assert( sizeof( FDataIndexerPrimaryKey ) == sizeof( FGuid ) )` を足し、
  メンバを残したまま Game ターゲットをビルド → `DataIndexerTypes.cpp(15,49): error C2607: 静的なアサーションは失敗しました`
  (`Result: Failed`)。AC (a) は修正前に赤だったことを確認した。
- Tier 2 (採用):
  - `OwnerRepository` メンバを削除。`ImportTextItem` の解決ブロックを削除 (load 中ガードは解決のためだけにあった)。
  - `PostSerialize` の else 分岐: `Ar.IsPersistent() && SerializedObject && ResolveOwnerRepository.IsBound()` のときだけ
    `ResolveOwnerRepository.Execute( *SerializedObject )` で都度解決し `MarkSearchableName`。`MarkSearchableName` 経路は不変。
  - `static_assert` は無条件版に上げた (下記 既定)。
- テスト (`DataIndexerSearchableNameTests.cpp`。`Scope` の外だが `Tests` 節が名指しするファイル):
  - 旧 `RowHandle.ImportText_SkipsRepositoryResolutionWhileLoading` を新行 `RowHandle.PinDefault_ResolvesOwnerOnlyOnPersistentSave` に置換。
    旧テストの最終 assert「ImportText で 1 回解決される」は HACK の実装詳細で、本タスクの Tier 2 (ImportText では解決しない) が新しい仕様。
    load 中の「解決しない」assert は Tier 2 では対照の無い不在 assert になるため残さず、ImportText の取り込み (key 保持・buffer 消費) だけ残した。
  - 新行: `ArIsObjectReferenceCollector = true` の `FObjectWriter` 派生で `TestNode->Serialize` を 2 回。`SerializedObject` は
    `TGuardValue` でノードに立てる (`FScopedObjectSerializeContext` は `CoreUObject/Internal` で plugin から見えない)。
    非 persistent → 解決 0 回、続く persistent → 1 回・解決元 = ノード。
  - Tests 4 問: (1) 非 persistent 保存アーカイブが owner を解決/消費せず、後の本保存で解決される契約。(2) ImportText 時解決への逆戻り・
    `IsPersistent` 絞りの削除で落ちる。(3) Case6 は本保存 1 回だけで、間に別アーカイブが挟まる順序を通らない。(4) 本番に無い口は不要
    (既存 delegate と公開 `SerializeContext` だけ)。
  - before-fix (Tier 2 前、テストだけ入れた tree): `PinDefault_…` Fail — `Expected 'A transient saving archive must not resolve the owner' to be 0, but it was 1`
    / `Expected 'The persistent save after it must still resolve the owner' to be 1, but it was 2`。他 RowHandle 8 件 Success。
  - after-fix: `Automation RunTests DataIndexerEd` (全群) Success 69 / Fail 0。`PinDefault_…` と `Case6_GraphPin_…` Success。
  - 対照 (hollow): else 分岐の `MarkSearchableName` を外して `RowHandle` 群 → `Case6_GraphPin_CreatesSearchableNameReference` だけ Fail
    (`Graph pin key should create a SearchableName reference via ResolveFromGraphPin`)。実 harvester 保存の searchable name が新分岐から
    来ていることを確認。元に戻して再ビルド成功。
- quick: Game `DataIndexerProject Win64 Development` Succeeded / Editor `DataIndexerProjectEditor` Succeeded (1 回目 C1001
  `DelegateInstancesImpl.h(794)`、再実行で成功 = run notes の一過性 ICE)。群名は `SearchableName` ではなく `RowHandle` (Tests 節の訂正)。
  full は run 終端 (ただし全群をここでも 1 回実行済み)。
- AC (a): 無条件 `static_assert` が Editor / Game 両ビルドで通る。修正前は Game で 24 byte により赤 (上記)。
- AC Tier 2 の rg: 書かれたままの `rg 'OwnerRepository' …/DataIndexerTypes.h` は 5 件 (delegate 名 `FResolveOwnerRepositoryDelegate` /
  `ResolveOwnerRepository` / `FFindOwnerRepositoryDelegate` / `FindOwnerRepository` ×2。どれも `Scope` 外で残す)。メンバ名を語単位で引く
  `rg '\bOwnerRepository\b'` が header 0 件 / cpp 0 件 (修正前 header 1 件)。これを同じ意図のより強い等価条件として記録する。
- AC (b): GUI (Reference Viewer、undo/redo、Duplicate) が要る。UE MCP (127.0.0.1:8001) がこの run では落ちている → verify_pending。
  自動テストの代替: Case6 (実保存で searchable name) + 新行 (非 persistent アーカイブを挟んでも本保存で解決)。
- 既定 (可逆): Tier 2 を出荷。AC (b) の GUI 判定はこの run では不能なので、Tier 1 への後退判定は検証 sweep に委ねる
  (sweep で (b) が落ちたら Tier 2 を revert して Tier 1 = メンバを `WITH_EDITORONLY_DATA` で囲む)。変更コスト: 小 (2 ファイル)。
- 既定 (可逆): `static_assert` を `#if !WITH_EDITORONLY_DATA` 無しの無条件にした。メンバが消えたので全構成で 16 byte が成り立ち、
  Editor ビルドでも同じ不変条件を検査できる (Scope の条件付き版より強い)。戻すコスト: 2 行。
- ビルド数: Game ×2 (赤 1 / 緑 1)、Editor ×5 (ICE 1、before-fix 1、after 1、hollow 1、restore 1)。
- 自己レビュー: `git diff --stat` = `DataIndexerTypes.cpp` / `DataIndexerTypes.h` / `DataIndexerSearchableNameTests.cpp` の 3 ファイル
  (69+/52-)。Scope 外のソースはテストファイルのみで、Tests 節が名指ししている。
- critic r1 後の対応: AC の rg を語単位に PLAN 修復 (AC 節に理由)。Case6 見出しコメントの `OwnerRepository` を `owner repository` に
  (テストファイル 1 行、diff 内)。再 quick: Editor Succeeded、`DataIndexerEd.RowHandle` Success 9 / Fail 0。

critic r1:
```
FAIL
1. E:\workspace\DataIndexerProject\Plugins\DataIndexer\Source\DataIndexer\Public\DataIndexerTypes.h:34,35,37,38,48 — kind 1 — AC「Tier 2 まで行けた場合: `rg 'OwnerRepository' …/DataIndexerTypes.h` が 0 件」(task:58) が成り立っていない。Tier 2 は出荷済みなので、この AC は適用される。書かれたとおりのコマンドは今も 5 件を返す (`FResolveOwnerRepositoryDelegate` / `ResolveOwnerRepository` / `FFindOwnerRepositoryDelegate` / `FindOwnerRepository` ×2)。author は Execution Log:108-110 で判定を `\bOwnerRepository\b` に差し替えた。これは author 自身の再解釈で、AC 本文の判定ではない。次のどちらかが要る: (a) task 作成者の承認を得て AC 本文を `\b` 版に改訂する。(b) AC どおり 0 件にする。

NOTE: …t261026-91cd-primarykey-owner-hack.md:57 — AC (b) (undo/redo・Duplicate 後に Reference Viewer に Repository が出る) は GUI でしか確かめられない。diff からは判定できない。代わりの自動テストは Case6 と新行 `PinDefault_ResolvesOwnerOnlyOnPersistentSave`。Tier 1 へ戻すかどうかは検証 sweep で決まる。
NOTE: …DataIndexerTypes.cpp (diff の削除行 20-28) — `ImportTextItem` の `#if WITH_EDITOR` ブロックを、中身ごと既存のガードと一緒に削除している。Out of Scope の「既存のガードは触らない」は規則統一 (t261026-a134) を指すと読んだ。Scope は `ImportTextItem` の参照削除を名指ししている。このため FAIL にはしない。
NOTE: …DataIndexerTypes.cpp:158 — 旧コードには「load 中 (RF_NeedLoad/RF_NeedPostLoad) は repository getter を呼ばない」ガードがあり、diff で消えた。新しい分岐は `Ar.IsSaving() && Ar.IsPersistent()` でしか走らないので、load 中には実質当たらないはず。ただし、同期 load 中の node を保存する経路が無いことは diff からは確かめられない。
NOTE: …DataIndexerTypes.h — Scope (task:36) は `#if !WITH_EDITORONLY_DATA` 付きの static_assert を求めている。diff は無条件版を入れた (cpp:10)。AC (a) の条件は無条件版の方が強く満たすので、FAIL にはしない。
NOTE: …DataIndexerSearchableNameTests.cpp:674 — Case6 のコメント見出しが、消えたメンバ名 `OwnerRepository` をまだ使っている (動作には無害)。

除去名の sweep 結果: メンバ `OwnerRepository` と旧テスト名 `ImportText_SkipsRepositoryResolutionWhileLoading` は、リポジトリ内の参照が .agents の文書だけ。`ResolveOwnerRepository` / `FindOwnerRepository` は今も参照されている。kind 3 と kind 5 は該当なし。
```

critic r2:
```
PASS
NOTE: …t261026-91cd-primarykey-owner-hack.md:57 — AC (b) は GUI でしか確かめられない (Reference Viewer で、undo/redo 後と Duplicate 後の保存を見る)。diff からは判定できないので、status は verify_pending のまま残る。自動テストの代わりは Case6 と新しい行 PinDefault_ResolvesOwnerOnlyOnPersistentSave の 2 つ。
NOTE: …DataIndexerTypes.cpp:10 — static_assert は、Scope が指定した `#if !WITH_EDITORONLY_DATA` 付きではなく無条件で入った。無条件版は !WITH_EDITORONLY_DATA の場合も含むので、AC (a) は満たす。Scope の文言からは外れるが、Execution Log に「既定」として記録されている。
NOTE: …DataIndexerTypes.cpp:117 (diff の削除行 20-28) — 既存の `#if WITH_EDITOR` ブロックを丸ごと消している。Out of Scope は「既存のガードは触らない」と書いている。ただし、このブロックの中身は Scope 内の OwnerRepository の解決だけだった。メンバを消すことは Tier 2 の要求なので、FAIL ではなく NOTE にした。
NOTE: 削除した名前の sweep — `\bOwnerRepository\b` の残りは DataIndexerCompiler.cpp:97 と DataIndexerRepository.cpp:238 の無関係なローカル変数と、.agents 配下の文書だけ。DataIndexerTypes.h の該当は 0 件。旧テスト名 ImportText_SkipsRepositoryResolutionWhileLoading を参照するスクリプトや文書は無い。delegate ResolveOwnerRepository と FindOwnerRepository は今も使われている。
NOTE: round 1 の回答を確かめた — AC の rg を語単位にした修復はタスクファイルの 58-60 行にある。Case6 のコメントは diff の 198 行で直っている。
```

verify sweep 2026-10-28 16:55 (環境不可、verify_pending 継続):
- エディタ (PID 9372) は HEAD のビルドで起動中、UE MCP に接続できた。ユーザーが作業中だった (最終入力 0.8 秒前、前面は Chrome リモート デスクトップ)。dispatch の条件は「ユーザーの作業を妨げない」「アセットを恒久的に変えない」。
- AC (b) は Ctrl+Z / Ctrl+Y と Reference Viewer を要する。MCP に undo/redo の手段は無い。SlateInspectorToolset の PressKey は Slate へキーを注入する。よって AC (b) は未実施のまま残した。Tier 2 の判定 (Tier 1 への後退) はまだ出ていない。
- 次の sweep への手がかり: 保存と Duplicate の部分は画面なしで取れる (BlueprintTools `create` → GetRow の `create_node` → `set_pin_value` → AssetTools `save_assets` → `get_dependencies` → `duplicate` → `get_dependencies` → 一時アセットを `delete`)。undo/redo の部分は GUI が要る。

### verify sweep 2026-10-28 18:44 (ALICE、合格 → done)
- 環境: エディタ PID 9372 (15:50 ビルド = submodule 0f651e4)、UE MCP 127.0.0.1:8001。GUI 操作はユーザー許可済み (dispatch 2026-10-28「今からやって」)。一時アセットは `/Game/__Verify` に作り、終了時に削除した。証跡は `.agents/tasks/evidence/t261026-91cd/`。
- 一時 BP: `/Game/__Verify/BP_VGetRow` (GetRow ノード、Handle Repository = DI_ItemRarity)。
- AC (b): Handle Primary Key をピンのコンボで Common に設定 (01a) → Ctrl+Z でピン値が空に戻った (01b、get_pin_value でも確認) → Ctrl+Y で 2B8F0001… に戻った → 保存 → Reference Viewer に DI_ItemRarity (ハード参照) と searchable name ノード "2B8F0001-0000-4000-8000-000000000001 In Data Indexer Repository: DI_ItemRarity" が出た (02)。複製 BP_VGetRow_Dup も同じ (03)。
- Tier 2 はそのまま採用 (AC (b) が通ったので Tier 1 へは戻さない)。
- 所要 約 5 分。

## Summary
`FDataIndexerPrimaryKey::OwnerRepository` を削除し (Tier 2)、全構成で 16 byte に戻した (`static_assert`)。graph pin の default は
`PostSerialize` で persistent 保存時だけ `SerializedObject` から都度解決するので、間に transient 保存アーカイブが挟まっても
searchable name が落ちない。Reference Viewer での GUI 確認 (AC b) は MCP 停止のため検証待ち。
GUI 検証は 2026-10-28 の verify sweep で合格した。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerTypes.h`
- `Plugins/DataIndexer/Source/DataIndexer/Private/DataIndexerTypes.cpp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerSearchableNameTests.cpp`
- `.agents/tasks/done/t261026-91cd-primarykey-owner-hack.md`

## Blockers
None
