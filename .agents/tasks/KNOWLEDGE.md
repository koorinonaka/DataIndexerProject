# KNOWLEDGE

## 監査 2026-10-26

- 一覧は `.agents/audit-2026-10-26.md`。ID は `B-` (bug) / `P-` (perf) / `R-` (refactor) + 領域
  (`R` runtime / `V` Views / `C` Customizations / `E` Editor core)。横断テーマは `X1`〜`X7`。
- 静的解析のみ。perf は仮説。エンジン側の根拠は UE 5.8 ソース (`E:\UE\UE_5.8`) 実測。
- 「問題なし」と確認した点は §4。再監査するときはそこを飛ばす。
- `ExpandedStruct` の array/map/set 非対応は設計どおり (bug ではない)。
- `task-dup-check.js` は `.agents/tasks` が無いと exit 2。初回起票 (2026-10-26) は corpus 0 で通過。
