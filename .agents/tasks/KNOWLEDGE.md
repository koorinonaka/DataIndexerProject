# KNOWLEDGE

## 監査 2026-10-26

- 一覧は `.agents/audit-2026-10-26.md`。ID は `B-` (bug) / `P-` (perf) / `R-` (refactor) + 領域
  (`R` runtime / `V` Views / `C` Customizations / `E` Editor core)。横断テーマは `X1`〜`X7`。
- 静的解析のみ。perf は仮説。エンジン側の根拠は UE 5.8 ソース (`E:\UE\UE_5.8`) 実測。
- 「問題なし」と確認した点は §4。再監査するときはそこを飛ばす。
- `ExpandedStruct` の array/map/set 非対応は設計どおり (bug ではない)。
- `task-dup-check.js` は `.agents/tasks` が無いと exit 2。初回起票 (2026-10-26) は corpus 0 で通過。
- R-R11 (監査): runtime ヘッダ `DataIndexerRepository.h` が editor クラス 9 個を friend にしている。解消は設計変更 (accessor の新設) なので t261026-a134 では見送った。

## UE MCP GUI ハーネス

2026-10-28 verify sweep (ALICE、UE 5.8、MCP 127.0.0.1:8001) で実測。スクリプトは `evidence/_harness/`。

- MCP の Slate `Click` / `PressKey` で modal (FMessageDialog・保存確認) を開くと game thread が止まり、MCP 呼び出しは 300 秒で切れて再接続になる。modal を開く操作は実マウス (`click.ps1`) で行う。MCP 呼び出しが modal を開くときは、先に `bgyes.ps1` を background で走らせて青いボタンを押させる。
- 撮影は `cap.ps1` (PrintWindow、エディタが背面でも撮れる)。メニュー・トースト・別ウィンドウは `-Desktop` で撮る。Slate の pos はプライマリ画面の座標と同じ (最大化ウィンドウ、枠 -8)。
- Data View のセル範囲選択: Slate `Drag` も素の Click+Shift+Click も行選択になる。Ctrl+Click (セルモード開始) → Shift+Click で矩形になる。セル中心が表示外 (横スクロールの外) だと Click は外れる。
- 実 OS 入力 (`SetCursorPos`+`mouse_event`) はヘッダ境界のドラッグ・メニュー・ダイアログに効く。`keybd_event` の Ctrl/Shift は Slate に届かなかった (2 回) ので、修飾付きクリックは Slate `Click` の `modifiers` を使う。
- Slate の ref はタブ切替・ビュー再構築で失効する。`Snapshot` を取り直す。`Observe` でサブツリーを監視すると子の ref が付く。
- コンソールは状態バーの Cmd テキストボックスへ Slate `Type` (submit) で打てる。`obj list class=Blueprint` → `LogsToolset.GetLogEntries` で「ロード済み BP」を数えられる (エンジンに LoadPackage の行ログは無い)。
- K2Node の検証は exec に繋がっていないと孤立ノードとして刈られ、`compile_blueprint` で走らない。BeginPlay から繋ぐ。
- 非表示の関数ピンの `DefaultObject` は、ノードを選んで Ctrl+C した T3D (クリップボード) で読める。
- `AssetTools.delete` は Reference Viewer などで掴まれた資産を消せないことがある (戻り値 true でも `exists` が true)。
- (2026-10-28 2 回目の sweep) 一時ログカウンタの代わり: SlateInspector の ref はウィジェット実体ごとに振られる。Snapshot の ref の一致で「作り直されていない」を数えられる。使う前に、静止で 2 回同じ・Undo (全再生成) で全部変わる、を確かめる。
- Python の逃げ道: 状態バーの Cmd 欄へ `py <文>` を `Type` (submit) すると実行され、`print` は `LogPython` に出る。Cmd 欄の ref は前面のタブで変わるので、毎回 Snapshot で `textbox … pos=3xx,10xx` を探す。`GetLogEntries` は数秒遅れることがある。「行が無い」と結論する前に読み直す。
- `Type` は欄の中身を置き換える (追記ではない)。MCP の `PressKey Ctrl+Z` は Data View で効かなかった (同じ時点の `TRANSACTION UNDO` は効いた)。Undo は Cmd 欄の `TRANSACTION UNDO` で行う。
- Undo の検証の前に `py print(unreal.SystemLibrary.end_transaction())` を打つ。1 以上なら開いたままのトランザクションが残っている (どの MCP ツール由来かは不明)。残っていると `TRANSACTION UNDO` が黙って何もしない。閉じると、それまでの操作が 1 つの塊になる。0 を確かめてから測る。
- アセットピッカーは現在値までスクロールして開く。先頭の数件だけ見ると、絞り込まれたように見える。比べるときは現在値が None のピッカーを使う。
- `_harness/dragcap.ps1`: 実マウスのドラッグを押したまま CopyFromScreen で撮ってから離す (ドラッグのデコレータ・フィルのプレビュー用)。`_harness/t3dpins.ps1`: ノードを選んで MCP `PressKey Ctrl+C` した後のクリップボード T3D から、ピンごとの `bIsReference` / `bIsConst` / LinkedTo 数 / 型を出す。`_harness/cursor.ps1`: 実カーソルを置いて OS カーソルの形 (SIZENS など) を返す。
- Data View の列ドラッグは、前に非表示のデータ列があると挿入位置が常に 0 になる (既存の不具合)。列の並べ替えを検証するときは全列を表示する。列の表示や並びは Saved/Config の SchemaLayouts に即時保存されるので、終わったら UI で戻す。
- このプロジェクトのスキーマには Repository 型・RowHandle 型の列が無い。Data View のセルの Repository ピッカー (PTC_Repository) を GUI で見るには、そういう列を持つ一時 UDS とスキーマが要る。
- (2026-10-28 3 回目の sweep) `end_transaction()` が 1 を返す件の一因: ProgrammaticToolset の `execute_tool_script` は実行中ずっとトランザクションを開いている。スクリプトの中から打った `TRANSACTION UNDO` は黙って何もしない。UNDO / REDO と `end_transaction` の確認は、スクリプトの外で SlateInspector `Type` を直接呼んで打つ (直接なら詳細パネルのペースト直後も 0 だった、2/2)。
- UDS に足した新しいフィールドは Schema の Column Layout で未チェックなので、Data View の列にならない (開き直しても出ない)。Data View が UDS の変更に追随するかを見るときは、チェック済みのフィールドを struct エディタで改名する (列が消える)。
- Repository の Schema Class ピッカーは RowStruct が同じ schema しか出さない。RowStruct の違う schema へ変えるには、Schema Class 行を右クリック → ペースト (クリップボードに `/Game/…/X.X_C`)。リセット確認は modal なので、先に `bgyes.ps1` を background で走らせる。
- Python で Schema CDO の `row_struct` を `set_editor_property` すると「Changing RowStruct will reset …」の modal が出る。`py` を `Type` する前に `bgyes.ps1` を background で走らせる。済んだら残った `bgyes.ps1` を止める (Data View の青い UI を誤クリックしうる)。
- UDS の型ピッカー: 型コンボを実マウスで開くと、検索欄が別ウィンドウの textbox として Snapshot に出る。Slate `Type` で絞り込み、候補を実マウスで押す。
- Schema BP のエディタを閉じた直後の `EditorAssetLibrary.delete_asset` は「使用中」で失敗することがある。`unreal.SystemLibrary.collect_garbage()` の後に再試行すると消える。
- (2026-10-28 t261028-12a6) 「再起動直後の読み込み時コンパイル」は GUI なしで再現できる。`UnrealEditor-Cmd … -ExecutePythonScript=<py> -nullrhi -unattended` の新しいプロセスで BP を `load_asset` すると、読み込み時のコンパイルの `LogBlueprint` エラーがログに出る。`DI_ItemRarity` / `DI_ItemType` は起動時に `VM_InventoryRowCard` 経由で読み込まれるので、未読み込みの Repository が要るときは複製する。K2Node の UPROPERTY (Repository 等) は `unreal.ObjectIterator(unreal.K2Node)` で見つけて `set_editor_property` で変えられる (PostEditChange が走る)。スクリプトは `evidence/t261028-12a6/`。
- (2026-10-29 sweep) `mcp__unreal__*` が繋がらないセッションでは `_harness/mcp.py` で HTTP から直接叩く (`ts <Toolset.tool> <json>`、`cmd '<console>'` は Cmd 欄へ Type)。urllib は `Connection: close` を送り、tools/call の SSE 本文が空で返る。http.client で送る。ObjectTools `set_properties` の `values` は JSON 文字列。BlueprintTools の `graph`/`blueprint`/`node` は `{"refPath": ...}`。
