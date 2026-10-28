---
status: ready
order: 210
tier: standard
screen: none
agent: any
created_by: task-create
---

# RowEntity の JSON import を non-strict に戻し、未知キーだけを失敗にする

## Goal
row struct にフィールドを足しても、足す前に書き出した native JSON (`ExportAsJSON`) が `ImportAsJSON` で入る。
逆に、RowEntity の JSON object に row struct に無いキーがあれば、import は理由つきで失敗する。

## Context
- 出どころ: t261026-25fc の要ユーザー判断。ユーザー決定 2026-10-28「RowEntity は non-strict + 未知キー検査 (新タスクで実装)」。
- 現状 (2026-10-28 時点、submodule `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerJsonSupport.cpp`):
  - 無名 namespace の `ImportRowEntityFromJson` が `FJsonObjectConverter::JsonObjectToUStruct( ..., 0, CPF_Transient | CPF_Deprecated, true, &OutFailReason )`
    を呼ぶ。第 6 引数 `true` が engine の `bStrictMode`。
  - engine の `bStrictMode` は「row struct のフィールドが JSON に無いと失敗」(`JsonObjectConverter.cpp:1341-1352`、25fc の Execution Log)。
    未知キーは見ない。だから今は「フィールド追加で古い export が入らない」「未知キーは黙って捨てる」の両方が起きる。
  - 25fc の Execution Log: Scope の「`EJsonObjectConversionFlags` で未知キーを失敗に」はエンジンに無い。そこで `bStrictMode=true` を選んだ。
    今回はこれを独自検査に置き換える。
  - 呼び手は `FDataIndexerSerializer::ImportAsJSON` 内の `ImportCallback` lambda (RowEntity プロパティだけを横取りする)。
    callback が false を返すと engine 既定の `FInstancedStruct` import に落ちるので、失敗は `bEntityFailed` と理由で返す形を保つ。
  - export (`ExportRowEntityToJson`) は `EJsonObjectConversionFlags::SkipStandardizeCase` で書く。キーはプロパティ名そのまま。
    `CPF_Transient | CPF_Deprecated` のフィールドは書かない。
- 未知キーの定義: RowEntity の JSON object の**トップレベル**のキーのうち、`RowStruct` のどのプロパティ名にも一致しないもの。
  - 照合規則は engine の import と同じにする。実装前に `JsonObjectConverter.cpp` の `JsonAttributesToUStructWithContainer` を読み、
    プロパティ名の取り方 (`GetAuthoredName` か `GetName` か) と大小文字の扱いを合わせる。
  - Transient / Deprecated のプロパティ名は既知として扱う (古い export に、後で deprecated にしたフィールドが載っていても通す)。
  - ネストした struct の未知キーは見ない (Out of Scope)。
- 行レベル (`FDataIndexerEditorRow` 自身、`ImportAsJSON` の行ループで `bStrictMode=true`) は対象外。行の器は plugin が持つ型でユーザーが足さない。
- 既存テスト: `Private/Tests/Core/DataIndexerImportAtomicityTests.cpp` の `DataIndexerEd.Serializer.FailedNativeImportKeepsExistingRows` は
  `FCase` 表 (`Label` / `BadElement` / `ExpectedEngineError`) で「失敗しても既存行が残る」を検査している。未知キーの失敗はこの表に行を足せる。
- UE 5.7 も配布対象。`JsonObjectToUStruct` の引数と strict の意味が 5.7 (`D:\Apps\Epic Games\UE_5.7\Engine\Source`) で同じかを読んで確かめる。
- 重複チェック通過 (23 件対象。最も近い t261026-25fc で dup 0.25 / conflict 0.19)。

## Scope
- `DataIndexerJsonSupport.cpp` の `ImportRowEntityFromJson`: `bStrictMode` を false にし、未知キー検査を足す。理由文 (`FText`、`NSLOCTEXT`) にキー名を入れる。
- 関数上のコメント (「Strict like the row level ...」) を新しい規則に書き換える。
- 自動テストを 2 本分 (下の Tests)。

## Out of Scope
- 行レベル (`FDataIndexerEditorRow`) の strict。
- DataTable 経路 (`ImportFromDataTableJSON`) と `ImportRowsFromJSON` の規則。
- ネストした struct・配列要素の中の未知キー。
- 足したフィールドの既定値の扱い (struct の既定値のまま入る。engine の non-strict の挙動)。

## Preconditions
- t261026-25fc が done (`ImportRowEntityFromJson` と `OutFailReason` の経路がある)。

## Steps
1. `JsonObjectConverter.cpp` で import 側のプロパティ名の照合を読み、未知キー検査の照合規則を決める (5.7 も同じか確認)。
2. `ImportRowEntityFromJson` で `JsonObjectToUStruct` の前に、JSON object の各キーを `RowStruct` のプロパティ (`TFieldIterator<FProperty>`、継承分を含む) と照合する。
   一致しないキーがあれば、そのキー名を含む理由を `OutFailReason` に入れて false を返す。
3. `JsonObjectToUStruct` の `bStrictMode` を false にする。コメントを直す。
4. Tests を足して、修正前の木で落ちることを確かめてから修正を入れる。
5. quick (`DataIndexerEd.Serializer` + `DataIndexerEd.Migration`) を回す。

## Acceptance Criteria
- RowEntity から row struct のフィールドを 1 つ抜いた native JSON の `ImportAsJSON` が `true` を返し、抜いたフィールドは struct の既定値になる (現在: `false`)。
- RowEntity に row struct に無いキーを足した native JSON の `ImportAsJSON` が `false` を返し、`OutFailReason` にそのキー名が入り、既存行が残る (現在: `true` でキーを捨てる)。
- `ImportRowEntityFromJson` の `JsonObjectToUStruct` 呼び出しの `bStrictMode` 引数が `false`。

## Tests
自動テスト 2 件。Tests 4 問:
1. 守る契約: 古い export は row struct へのフィールド追加後も入る。未知キーは理由つきで拒否する。
2. 落ちる回帰: `bStrictMode` を true に戻す / 未知キー検査を消す。
3. 既存テストは「RowEntity が丸ごと無い」行しか持たず、フィールド単位の欠落も未知キーも通らない。未知キーは既存の `FCase` 表に行を足す。
4. 本番に無い口は要らない。`ExportAsJSON` → 文字列加工 → `ImportAsJSON` の本物の境界で検査する。

- `DataIndexerImportAtomicityTests.cpp` の `FailedNativeImportKeepsExistingRows` の `Cases` に 1 行: RowEntity に未知キー (例 `"NoSuchField"`) を足した要素。
  理由にキー名が入ることも検査する。
- 新規 1 件 (`DataIndexerEd.Serializer.<名前>`、`DataIndexerSerializerTests.cpp` に置く): `FDataIndexerAutomationRow` を export し、RowEntity から 1 フィールドのキーを消した JSON を
  import して `true`、値が既定値。row struct が 1 フィールドしか無くて消すと空になるなら、2 フィールド以上の既存 fixture 型を使う (`Tests/Fixtures/DataIndexerTestTypes.h`)。
- 修正前の木で、新規 1 件は「import が false」、表の追加行は「import should fail」で落ちることを確かめる。
- quick: ビルド + `Automation RunTests DataIndexerEd.Serializer+DataIndexerEd.Migration`。

## Verification
- 自動テストの結果 (`Saved/Logs/DataIndexerProject.log` の `Automation Test Succeeded/Failed`) だけで閉じる。GUI は要らない。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし
