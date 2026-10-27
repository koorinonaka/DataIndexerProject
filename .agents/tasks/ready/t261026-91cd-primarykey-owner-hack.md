---
status: ready
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
- Tier 2 まで行けた場合: `rg 'OwnerRepository' Plugins/DataIndexer/Source/DataIndexer/Public/DataIndexerTypes.h` が 0 件 (現在 1 件以上)。

## Tests
- `DataIndexerSearchableNameTests.cpp` (750 行、既存 fixture あり) に行: `FObjectWriter` 相当の非 persistent アーカイブで `PostSerialize` を通した後、
  persistent 保存で searchable name が記録される。Tier 1 のみなら既存行の回帰確認だけ。
- quick: `DataIndexerEd.SearchableName` 相当の群 (実際の群名は `rg IMPLEMENT_.*SearchableName` で確認)。Game ターゲットもビルド。

## Verification
- Reference Viewer のスクリーンショット (undo/redo 後の保存、Duplicate 後)。Game ターゲットのビルド log。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし。
