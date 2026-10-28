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
