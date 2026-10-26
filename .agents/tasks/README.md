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

対象コードは submodule `Plugins/DataIndexer` (UE 5.8 plugin、Fab 配布)。
この repo (`DataIndexerProject`) はサンプルプロジェクト + docs + タスク置き場。

### リポジトリとブランチ (2 repo 構成)

- 作業ブランチは **両 repo とも `main` から切る** (例 `fix/audit-2026-10`)。`release/*` には触らない。
- 前提: `release/v1.0.2` が両 repo の `main` にマージ済みであること。未マージなら実装に入らず
  `blocked` にして報告する (ユーザー決定 2026-10-26)。
- コミットは 2 段: (1) `Plugins/DataIndexer` 内で plugin のソースをコミット、
  (2) 親 repo で submodule pointer を bump (`chore: update DataIndexer submodule`) + タスクファイル。
  submodule を dirty のまま親をコミットしない。どちらも push しない。
- ソースの `file:line` は起票時点 (2026-10-26、`release/v1.0.2`)。ずれていたら関数名で探す。

### コマンド

エンジン: `E:\UE\UE_5.8`。`Build.bat` はエディタを閉じてから実行する (開いたままだと DLL ロックで失敗)。

- **quick** (タスクごと。task-execute `executor.md` Quality gate):

  ```powershell
  & 'E:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat' DataIndexerProjectEditor Win64 Development -Project='E:\workspace\DataIndexerProject\DataIndexerProject.uproject' -WaitMutex
  & 'E:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\workspace\DataIndexerProject\DataIndexerProject.uproject' -ExecCmds="Automation RunTests DataIndexerEd.<Group>; Quit" -unattended -nullrhi -nop4 -nosplash -log
  ```

  `<Group>` はタスクが触った自動テストの群 (`Compiler` / `EditorData` / `EditorFunctionLibrary` /
  `Logger` / `Migration` / `NotOverridable` / `Repository` / `Schema` / `SchemaLayout` / `Serializer` /
  `View` / `ViewModel`)。結果は `Saved/Logs/DataIndexerProject.log` の `Automation Test Succeeded/Failed` 行で確認する。

- **full** (run 終端と push 前に1回): quick と同じビルド + `Automation RunTests DataIndexerEd` (全群)。
- ソース変更後は Editor ターゲットのビルドを成功させる。runtime 構造体のサイズ・`WITH_EDITORONLY_DATA`
  に触るタスクは Game ターゲット (`DataIndexerProject Win64 Development`) もビルドする。
- 関連する自動テストを実行し、失敗を残さない。群の一覧は `rg 'IMPLEMENT_.*AUTOMATION_TEST' Plugins/DataIndexer/Source` で確認する。
- 自動テストの追加・変更は `~/.claude/CLAUDE.md` の Tests 4 問を先に答える。既存テーブルに行を足せるなら足す。
- **UE 5.7 も配布対象** (`Plugins/DataIndexer/Script/ArchivePlugin.bat` が 5.7 と 5.8 を両方ビルドする)。エンジン API に依存する変更は
  `D:\Apps\Epic Games\UE_5.7\Engine\Source` で同じ挙動かを読んで確認し、差があれば `UE_VERSION_NEWER_THAN_OR_EQUAL(5,8,0)` で分岐する
  (`AssetDefinition_DataIndexerRepository.cpp:37-47` に前例)。5.7 でのビルドは release 時の `ArchivePlugin.bat` が検出する。

### 証跡

- `screen: required` のタスクは GUI 操作の証跡をスクリーンショットで残す。UE MCP server
  (`http://127.0.0.1:8001/mcp`、エディタ起動が要る) か `windows-screenshot` スキルで撮る。
- perf 項目は静的解析の仮説が元。Acceptance Criteria は「構造が変わったこと」で書く。
  計測値は添えられれば添える (`stat` / `TRACE_CPUPROFILER_EVENT_SCOPE`)。必須ではない。

## KNOWLEDGE.md の目次

<!-- 1行1トピック: 「- <見出し> — 一言」。KNOWLEDGE.md の見出しと一致させる。 -->

- 監査 2026-10-26 — `.agents/audit-2026-10-26.md` の読み方と「問題なし」と判断した点
