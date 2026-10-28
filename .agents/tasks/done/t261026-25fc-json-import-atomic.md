---
status: done
order: 20
tier: standard
screen: none
agent: any
created_by: task-create
---

# JSON import を原子化し、失敗理由を返し、round-trip の非対称を直す (X5)

## Goal
JSON import (`ImportAsJSON` / `ImportFromDataTableJSON` / `ImportRowsFromJSON` / Reimport) が、途中失敗で
アセットを壊さず、トランザクションを cancel し、失敗理由を呼び手に返す。export → import の round-trip で
FText / `Name` 列 / 重複 PrimaryKey が黙って落ちない。

## Context
監査 `.agents/audit-2026-10-26.md` の X5 = B-E2, B-E3, B-E9, B-E10, B-E11, B-E12, B-E13, B-E16, R-E1, R-E12, R-E14, P-E2, P-E5。
重複チェック通過 (0件対象)。パスは `Plugins/DataIndexer/Source/` 相対、行番号は 2026-10-26 時点。

- B-E2: `DataIndexerEd/Private/DataIndexerJsonSupport.cpp:94` `ImportAsJSON` が live `EditorRows` を
  `JsonArrayStringToUStruct` に渡す。エンジン `JsonObjectConverter.h:393-414` は `SetNum(N)` してから in-place 変換し、
  最初の不正要素で `false`。途中失敗で N 個の default 行が残る。既存の `DataIndexerImportAtomicityTests.cpp` は
  `ImportFromDataTableJSON` だけを覆う。
- B-E3: `:252`, `:469`, `DataIndexerEditorFunctionLibrary.cpp:44` の 3 呼び手が失敗時に `FScopedTransaction` を
  `Cancel()` しない (`rg '\.Cancel\(' DataIndexerJsonSupport.cpp` 0 件、2026-10-26 確認)。`OutFailReason` は `:95` で nullptr。
- R-E1: 「transaction + ImportAsJSON + `FindFProperty("EditorRows")` + NotifyPostChange」が 3 写し (`:262-265`, `:476-478`,
  `EditorFunctionLibrary.cpp:49-50`)。2 つは `TEXT("EditorRows")` リテラル、1 つは `GET_MEMBER_NAME_CHECKED`。
- R-E12: `Public/DataIndexerEditorData.h:82-83` の `friend class FDataIndexerSerializer` が modifier を迂回して
  `EditorRows` を書く元。`friend FDataIndexerCompiler` は `GetRows()` が public なので不要。
- P-E2: `:349-407` `ImportFromDataTableJSON` が `Empty()` → 行ごと `InsertRow` → 行ごとに `FScopedRowsModifier`
  (Modify + `MarkRepositoryDirty` + 全 toolkit の `NotifyAssetChangedExternally`)。
- P-E5: `:161-167` + `:94` でファイル全体を 2 回パース (形式判定 + 本 import)。
- B-E9: export は `FText::ToString()` (`:63`)、import は `FTextStringHelper::ReadFromBuffer` (`:123`、エンジン
  `JsonObjectConverter.cpp:819-828`) → localized text の namespace/key が落ちる。直すと export の FText 値の文字列形式が
  `NSLOCTEXT("ns","key","text")` 形になり、旧 export との VCS diff が FText 列で全行変わる (docs は VCS diff を売りにしている)。
  旧ファイルの import は引き続き通る。**ユーザー決定 2026-10-26 (昇格レビュー): 変える。** docs の json-support に形式変更を 1 行足す。
- B-E10: `:43` `ImportRowEntityFromJson` が `JsonObjectToUStruct` の戻り値を無視し non-strict。行レベル (`:95` strict) と非対称。
- B-E11: `:378` `RowNameProperty == NAME_None` でも `Name` キーを剥がす。
- B-E12: `DataIndexerEditorData.cpp:447` keyed `InsertRow` が重複未検査。`:364` でパース不能 PrimaryKey を無言で新 GUID。
- B-E13: `DataIndexerRepositoryFactory.cpp:19-21` `bEditorImport = true` + `Formats "json"` だが `FactoryCreateFile/Text` 未実装。
  docs (`docs/docs/editor-guide/json-support.md:3,43-62`) は Content Browser コンテキストメニューの Import JSON と toolbar の
  Reimport しか約束していない → advisor 判断: `bEditorImport = false` にして `Formats` を外す (実装しない)。
- B-E16: `:459` `UE_LOG(LogTemp, …)`。他は `LogDataIndexer`。
- R-E14: `:384-408` 手動 `TArray<uint8>` + `InitializeStruct/DestroyStruct`。`:42` は既に `FInstancedStruct`。

## Scope
- `ImportAsJSON` はローカル `TArray<FDataIndexerEditorRow>` に deserialize → 成功時だけ `UDataIndexerEditorData::SetRows(TArray&&)`
  (新設、`FScopedRowsModifier` 経由) で swap。失敗時は `OutFailReason` を埋めて `false`。
- 3 呼び手は `FScopedTransaction` を持ち、失敗で `Cancel()`。Content Browser 経路は `FMessageDialog` か notification で理由を出す。
  `ImportRowsFromJSON` (BP 関数) は `OutFailReason` を返す引数を足す。
- `ImportFromDataTableJSON` もローカル配列を組んでから `SetRows` 1 回。重複 PrimaryKey とパース不能 PrimaryKey は失敗理由に含める。
- `friend class FDataIndexerSerializer` / `friend class FDataIndexerCompiler` を `DataIndexerEditorData.h` から外す。
- JSON パースを 1 回にし、`TArray<TSharedPtr<FJsonValue>>` を下へ渡す。
- FText は `WriteTextAsComplexString` 相当で export し、import と対称にする。
- `ImportRowEntityFromJson` は戻り値を見て strict (`EJsonObjectConversionFlags` で未知キーを失敗に) にする。
- `Name` キーの剥がしは `RowNameProperty != NAME_None` のときだけ。
- Factory の import 主張を外す。`LogTemp` を `LogDataIndexer` に。`:384-408` を `FInstancedStruct` に。

## Out of Scope
- 行ルックアップのマップ化 (t261026-ba24)。`SetRows` の中でマップを組むのはそちらが足す。
- Undo 通知経路 (t261026-21a2)。`SetRows` は同タスクが作る `NotifyRowsChanged()` を呼ぶ想定。先に走っていなければ
  `NotifyPostChange` を呼ぶ。
- JSON の形式仕様そのものの変更 (キー名・配列構造)。

## Preconditions
- `release/v1.0.2` が両 repo の `main` にマージ済み。作業ブランチは `main` から。
- t261026-21a2 が先に done なら `NotifyRowsChanged()` を使う。必須ではない。

## Steps
1. `DataIndexerEditorData.h/.cpp` に `bool SetRows(TArray<FDataIndexerEditorRow>&& InRows)` を追加 (`FScopedRowsModifier` 内で `EditorRows = MoveTemp`)。
   friend 2 行を削除し、ビルドエラーになった箇所を `SetRows` / public getter に置換する。
2. `DataIndexerJsonSupport.cpp` の `ImportAsJSON` をローカル配列方式に書き換え、`OutFailReason` (FText*) を必須引数にする。
   `ImportRowEntityFromJson` を strict にし、`FText` の export/import を対称にする。`Name` 剥がしの条件を直す。
3. `ImportFromDataTableJSON` をローカル配列 + 重複/不正 PrimaryKey 検出 + `SetRows` 1 回に書き換える。
   `:384-408` を `FInstancedStruct` に置換する。
4. 形式判定の JSON パース結果を本 import に渡す (2 回目のパースを消す)。
5. 3 呼び手 (`:252`, `:469`, `EditorFunctionLibrary.cpp:44`) を共通の `ImportWithTransaction(...)` にまとめ、失敗で
   `Transaction.Cancel()` + 理由表示。`ImportRowsFromJSON` の BP シグネチャに `FText& OutFailReason` を足す。
6. `DataIndexerRepositoryFactory.cpp` の `bEditorImport` / `Formats` を外す。`:459` の `LogTemp` を直す。
7. テスト (Tests 参照) を足し、`DataIndexerEd.Serializer` + `DataIndexerEd.EditorFunctionLibrary` を回す。

## Acceptance Criteria
- `rg 'friend class' Plugins/DataIndexer/Source/DataIndexerEd/Public/DataIndexerEditorData.h` が 0 件 (現在 2)。
- `rg 'Transaction.Cancel\(\)' Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerJsonSupport.cpp` が 1 件以上 (現在 0)。
- 3 要素目が不正な JSON を `ImportAsJSON` に渡すと `false` + 非空の `OutFailReason`、`EditorRows` は import 前のまま (現在: default 3 行)。
- 同じ PrimaryKey を 2 行持つ DataTable JSON の import が `false` で理由に重複キーを含む (現在: 2 行入る)。
- `NSLOCTEXT` 付き FText を export → import すると namespace/key が保持される (現在: 落ちる)。
- `rg 'LogTemp' Plugins/DataIndexer/Source/DataIndexerEd/` が 0 件 (現在 1)。
- Content Browser の Import ダイアログのフィルタに DataIndexer の json が出ない。

## Tests
- `DataIndexerImportAtomicityTests.cpp` に行を足す: `ImportAsJSON` 途中失敗 → 行数と内容が不変。修正前コードで落ちることを確かめる。
- `DataIndexerSerializerTests.cpp` に行を足す: FText round-trip、`Name` プロパティ持ち struct の round-trip、重複 PrimaryKey 拒否。
- `DataIndexerEditorFunctionLibraryTests.cpp`: `ImportRowsFromJSON` 失敗時に `OutFailReason` 非空。
- quick: `DataIndexerEd.Serializer` + `DataIndexerEd.EditorFunctionLibrary`。

## Verification
- quick の log で `Automation Test Succeeded` を確認。GUI 不要。
- Reimport 失敗時に notification が出ることは log の `LogDataIndexer` 行で確認 (スクリーンショット不要)。

## Execution Log
- 2026-10-28 10:09 着手。前提: `release/v1.0.2` は両 repo の `main` にマージ済み (`git branch --contains release/v1.0.2` に main)。前提 t261026-21a2 は verify_pending の上に構築 (`NotifyRowsChanged()` は private のまま。`SetRows` は `FScopedRowsModifier` 経由で `PostEditChangeProperty` → `NotifyRowsChanged()` に届く)。
- 既知の赤 `DataIndexerEd.Migration.FailedImportKeepsExistingRows` の原因: エンジン `FJsonValue::AsObject()` (`JsonValue.cpp:78-89`) は非 object に対して static の空 object (有効なポインタ) を返し、LogJson Error を出す。旧コードの事前検査 `AsObject().IsValid()` は `42` を通し、default 行が入って `true` を返していた。新コードは `Type == EJson::Object` で判定し、この test は緑になった。
- 修正前の赤 (build 1 = テストだけ追加、実装は旧のまま。23.55 s): `Automation RunTests DataIndexerEd.Serializer+DataIndexerEd.Migration+DataIndexerEd.EditorFunctionLibrary` → 18 件中 13 Success / 5 Fail。Fail: FailedImportKeepsExistingRows (既知)、FailedNativeImportKeepsExistingRows (2 ケースとも行数 3。AC の「現在: default 3 行」を再現)、TextRoundTripKeepsLocalizationKey (namespace/key が "")、DataTableNameKeptWithoutBinding (Name が None)、DataTableRejectsBadPrimaryKeys (重複: true で 2 行、不正キー: true で 1 行)。
- 修正後 (build 3。23.32 s。build 2 は Toolset の呼び手エラーと MSVC C1001 で失敗、再ビルドで C1001 は再発せず): `Automation RunTests DataIndexerEd.Serializer+DataIndexerEd.Migration+DataIndexerEd.EditorFunctionLibrary+DataIndexerEd.Compiler+DataIndexerEd.EditorData` → 24 Success / 0 Fail。Compiler / EditorData 群は friend 削除と `SetRows` 追加のため足した。親 repo の `DataIndexerProject.Migration.CharacterDataTable` (`ImportFromDataTableJSON` の呼び手) → 1 Success。full は run 終端。
- Verification: 失敗時のログ行を確認。`LogDataIndexer: Warning: JSON import into 'DataIndexerRepositoryTests_8' failed: Element 0 is not a JSON object.` (EFL テストが通る `ImportWithTransaction` は Reimport と同じ関数)。Reimport の notification 表示そのものは GUI で未確認 (UE MCP 停止中)。
- AC「Import ダイアログのフィルタに出ない」: GUI ではなくコードで確認した。エンジン `AssetTools.cpp:6505-6516` (`ImportAssetsWithDialog`) は CDO の `bEditorImport` が真の factory だけをフィルタに入れる。`UDataIndexerRepositoryFactory` は `bEditorImport = true` と `Formats.Add` を消したので `bEditorImport` は既定の false になり、`Formats` も空。
- 「strict」の意味: Scope の「`EJsonObjectConversionFlags` で未知キーを失敗に」はエンジンに無い。`EJsonObjectConversionFlags` は SkipStandardizeCase / WriteTextAsComplexString / SuppressClassNameForPersistentObject の 3 つだけ。`bStrictMode` は「JSON に無いフィールドを失敗にする」(`JsonObjectConverter.cpp:1341-1352`)。行レベル (`:95` 旧、`bStrictMode=true`) と対称にするため、`ImportRowEntityFromJson` も `bStrictMode=true`。export が書かない `CPF_Transient | CPF_Deprecated` は import でも skip する。DataTable 経路は strict にしない (フラットな DataTable JSON は全フィールドを持たない)。
- エンジンの import callback は false を返すと「未処理」として既定の FInstancedStruct import へ落ちる (`JsonObjectConverter.cpp:584-591`)。RowEntity の失敗は lambda が捕まえたフラグと理由で返し、callback 自体は true を返す。
- 既定 (可逆): `ImportFromDataTableJSON` は `FText* OutFailReason = nullptr` を末尾に足し、必須にしなかった。理由: 親 repo `Source/DataIndexerProject/Tests/CharacterDataTableMigrationTests.cpp:225` が 3 引数で呼ぶ (Scope 外)。必須にするなら呼び手 1 か所を直すだけ。
- 既定 (可逆): `SetRows` は `void`。Steps の `bool` に失敗の場合が無いため。重複キーの検査は JSON 層の `CheckUniqueKeys` に置き、native / DataTable の両経路で使う。
- 既定 (可逆): 失敗の表示。`ImportFromFile` は `FMessageDialog`、Reimport は notification、`ImportWithTransaction` は毎回 `LogDataIndexer` Warning を出す。BP 経路は `OutFailReason` を返すだけ。
- Scope 外の source: 親 repo `Plugins/DataIndexerToolset/.../DataIndexerToolset.cpp:89` が `ImportAsJSON` を 2 引数で呼んでいてビルドが壊れた。この run の前例 c58b690 (sample を新シグネチャに合わせる別コミット) に倣い、別コミット `fix: update DataIndexerToolset to new ImportAsJSON signature` で `FailReason` を渡してログに出す 2 行を直した。Toolset の `NotifyRowsChanged()` (`NotifyPostChange` を呼ぶ) は `SetRows` の通知と重なるが無害なので触っていない。
- fold (軽微、source 以外): docs `docs/docs/editor-guide/json-support.md` / `.ja.md` に FText の形式変更 (Context のユーザー決定) と、失敗時は取り消して既存行を残す旨を各 2 行。ja は yomiyasu の run/verify を通した。`mkdocs build` はエラー無し。
- 差分の自己確認: `git diff --stat` は Scope の 5 ファイル + テスト 3 + fixture 1 + EFL 2 + Compiler/Factory。Scope 外は Toolset 1 ファイル (別コミット) と docs 2 ファイル (fold)。
- critic round 1: PASS
  - NOTE: DataIndexerRepositoryFactory.cpp:19 — AC「Import ダイアログのフィルタに DataIndexer の json が出ない」は GUI でしか確かめられない。diff は `bEditorImport = true` と `Formats.Add` を消している。author はエンジンの `AssetTools.cpp` を読んでコードで確かめた。画面ではまだ確かめていない。
  - NOTE: DataIndexerJsonSupport.cpp:603-608 — Reimport が失敗したときの notification が出るかは GUI でしか確かめられない。Execution Log も、画面では確かめていないと書いている。
  - NOTE: DataIndexerToolset.cpp:93 — 書き直した UE_LOG はまだ `LogTemp` を使っている。AC の `rg 'LogTemp'` は DataIndexerEd しか見ないので、AC 違反にはならない。
  - NOTE: DataIndexerJsonSupport.cpp:607 — `Repository->AssetImportData->Update` は null を確かめずに呼んでいる。この挙動は diff の前から同じ。`ImportFromFile` のほうは null を確かめている。
  - NOTE: docs/docs/changelog.md:22 — changelog は `ImportRowsFromJSON` を引数 2 つの形で説明している。BP シグネチャには `OutFailReason` が増えた。この 1 行は doc の文で、コードを動かすものではない。直すかどうかは任意。
  - NOTE: sweep の結果 — 消えた呼び出しの先は全部まだ参照がある。diff が足した定義には全部呼び手がある。引数 2 つの `ImportAsJSON` の呼び手は repo のどこにも残っていない。`ImportFromDataTableJSON` の 3 引数呼び出しは既定値 nullptr でそのまま通る。
- critic NOTE への対応: Reimport の `AssetImportData` は関数冒頭 (`!Repository->AssetImportData` で Failed) で null を除いているので追加の検査は不要。changelog:22 は過去の版 (1.0.x) の記録なので書き換えない。Toolset の `LogTemp` は Scope 外のため触っていない。
- time: build 3 回 (23.55 s / 20.44 s 失敗 / 23.32 s)。

## Summary
JSON import 4 経路 (`ImportAsJSON` / `ImportFromDataTableJSON` / `ImportRowsFromJSON` / Reimport) はローカル配列に全行を組み、成功時だけ `UDataIndexerEditorData::SetRows` で 1 回入れ替える。失敗時は `FScopedTransaction::Cancel()` し、理由を `OutFailReason` で返す。`ImportFromFile` はダイアログ、Reimport は notification で理由を出す。JSON のパースは 1 回。FText は complex string で export し、namespace/key が往復する。DataTable 経路は重複 / 不正な PrimaryKey を拒否し、`Name` は束縛時だけ剥がす。friend 2 行、`LogTemp`、Factory の import 主張、手動の `TArray<uint8>` を消した。既知の赤 `FailedImportKeepsExistingRows` は緑になった。

## Changed Files
- `Plugins/DataIndexer/Source/DataIndexerEd/Public/DataIndexerEditorData.h` / `Private/DataIndexerEditorData.cpp` — friend 削除、`SetRows` 追加
- `Plugins/DataIndexer/Source/DataIndexerEd/Public/DataIndexerJsonSupport.h` / `Private/DataIndexerJsonSupport.cpp` — 原子化、理由、`ImportWithTransaction`、パース 1 回、FText、strict、`Name`、`LogTemp`
- `Plugins/DataIndexer/Source/DataIndexerEd/Public/DataIndexerEditorFunctionLibrary.h` / `Private/DataIndexerEditorFunctionLibrary.cpp` — `ImportRowsFromJSON` に `FText& OutFailReason`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerCompiler.cpp` — `GetRows()`
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/DataIndexerRepositoryFactory.cpp` — `bEditorImport` / `Formats` 削除
- `Plugins/DataIndexer/Source/DataIndexerEd/Private/Tests/Core/DataIndexerImportAtomicityTests.cpp` / `DataIndexerSerializerTests.cpp` / `DataIndexerEditorFunctionLibraryTests.cpp`、`Tests/Fixtures/DataIndexerTestTypes.h` (`FDataIndexerTextRow`)
- `Plugins/DataIndexerToolset/Source/DataIndexerToolset/Private/DataIndexerToolset.cpp` — 新シグネチャ (別コミット、Scope 外)
- `docs/docs/editor-guide/json-support.md` / `json-support.ja.md` — fold

## Blockers
None
