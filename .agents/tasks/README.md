# AIタスク運用

このディレクトリは、後でAIエージェントが実行する作業を管理する。

ワークフローの正(状態遷移・実行ルール・タスクファイル形式・完了要件)は
`task-create` / `task-execute` スキル(`SKILL.md` と `executor.md`)にある。この README は
project 固有情報だけを持ち、ワークフロー仕様を重複記載しない。Claude 以外のエージェントが
作成・実行する場合も、両スキルを読んで従う。

<!-- tasks-readme-format: v7 -->
<!-- 上のマーカーはスキルが旧形式READMEの移行判定に使う。削除・変更しない。 -->
<!-- v3: 品質基準は quick / full の2コマンド。知見は KNOWLEDGE.md へ分離し、ここには目次だけ。 -->
<!-- v4: ルート README に ## ストリーム 表を追加。ストリームは独立した置場+独立した order。 -->
<!-- v5: タスクIDはファイル名先頭の t<YYMMDD>-<hex4>。実行順は frontmatter order:。参照は <名前>/<ID>。 -->
<!-- v6: cancelled 状態を追加(ユーザーだけが決める・ファイルは消さない)。verify_pending の意味を配置に明記。 -->
<!-- v7: cancelled の置き場を ready/ から done/ へ。ready/ にはまだ動きうるタスクだけを置く。 -->

## 配置

状態の正は frontmatter の `status:` で、ディレクトリではない。

- `draft/`: 未承認タスク。実行禁止。
- `ready/`: 実行対象。`ready` / `in_progress` / `verify_pending` / `blocked` をここに置く。
  - `verify_pending`: 実装とテストは完了し `quick` も通っている。残りは環境依存の
    `Verification` (ロック画面・実機操作など) だけ。完了ではない。
- `done/`: 完了タスクと、取りやめたタスク（`status: cancelled`）。
  - `cancelled`: ユーザーだけが決める。理由と決定者を `Blockers` に残し、ファイルは消さずに
    `done/` へ移す。`done/` に在っても完了ではない —— 状態の正は `status:` で、実行対象からも
    依存の成立からも外れる。
- `KNOWLEDGE.md`: この project の知見(落とし穴・実測値・過去の事故)。毎回は読まない。
  下の目次で該当トピックがあるときだけ、その節を読む。

## ストリーム

<!-- ルート README のみ。Tool / Plugin ごとに独立したタスク置場(ストリーム)を持つ場合にここへ登録する。
     パスは repo ルート相対。各ストリームは draft/ ready/ done/ README.md を持ち、order: の採番は独立。
     task-execute <名前> で実行、他ストリームのタスク参照は <名前>/<ID>。空なら root のみ。 -->

| 名前 | パス | 用途 |
|---|---|---|

## Project固有の品質基準

<!-- project に合わせて追記する: ビルドコマンド、証跡ルールなど。
     コマンドは quick / full の2つを必ず書く。同じ1本でよい project は両方に同じものを書く。
     このディレクトリがストリームなら、先頭に「ルート .agents/tasks/README.md の品質基準を全て継承する」と書き、
     ストリーム固有の追加ルール(quick / full を含む)だけを続ける。 -->

- **quick** (タスクごと。task-execute `executor.md` Quality gate):
  - docs サイト (`docs/`) を触ったタスク: `cd docs; python -m mkdocs build -d <scratchpad>/site`
    がエラー無しで終わる。見た目の変更は、ビルド結果を `python -m http.server` で配信し、
    ブラウザで en / ja・light / dark・幅 375px / 1440px のスクリーンショットを証跡に残す。
  - C++ (`Plugins/DataIndexer`) を触ったタスク: エディタターゲットのビルドが通る。
- **full** (run 終端と push 前に1回): quick と同じ。
- ソース変更後は project のビルドを成功させる。
- 関連する自動テストを実行し、失敗を残さない。

## KNOWLEDGE.md の目次

<!-- 1行1トピック: 「- <見出し> — 一言」。KNOWLEDGE.md の見出しと一致させる。 -->

- (なし)
