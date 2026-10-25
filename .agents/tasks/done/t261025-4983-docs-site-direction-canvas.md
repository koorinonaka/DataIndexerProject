---
status: done
order: 10
tier: standard
screen: required
kind: canvas
agent: any
created_by: task-create
---

# docs サイトの見た目の方向性を 3 案、Design canvas で比べる

## Goal
DataIndexer の docs サイト (トップの overview とドキュメント本文ページ) について、
「AI のテンプレっぽい」印象から抜ける見た目の方向性を 3 案作り、1 枚の Artifact Design canvas に
並べる。ユーザーがこの canvas を見て 1 案を選ぶ。選んだ案を後続タスク
`t261025-dbeb` (実装) が再現する。

## Context
- サイトは MkDocs Material + i18n (`docs_structure: suffix`)。設定は `docs/mkdocs.yml`。
  - トップ: `docs/docs/index.md` (`template: overview.html`) → `docs/overrides/overview.html`
    (約 590 行、en / ja を `is_ja` で切り替えて同一テンプレートに両方持つ) + `docs/docs/assets/stylesheets/home.css`。
  - 全ページの基調: `docs/docs/assets/stylesheets/tokens.css` / `extra.css` / `c-variant.css`、
    `docs/overrides/partials/` (nav / path / source / toc)。
  - 現在のトップは過去のデザインハンドオフ「Overview B 案」(`C:\Users\koori\Downloads\DataIndexer\design_handoff_overview\README.md`) が起点。
- ユーザーの評価 (2026-10-25): 「AI のテンプレっぽい感じでパッとしない」。
  2026-10-25 時点のビルドを目視して、テンプレっぽさの要因と判断したもの:
  1. hero 上の eyebrow ラベル「— UNREAL ENGINE PLUGIN」(線+大文字+字間)。
  2. 2 行の大見出し「Rename rows. / References hold.」+ 説明段落 + ボタン 2 つ + 小さな meta チップ (点滅ドット付き「UE 5.7+ ready」)。
  3. 各セクションが同じ型の繰り返し: 大文字 eyebrow (「WHY DATAINDEXER」「WHAT DATAINDEXER ADDS」) → 2 行見出し → リード文。
  4. 「01 / MAINTAINABILITY」形式の連番フィーチャー見出し。
  5. DataTable vs DataIndexer の ✕ / ✓ 比較表。
  6. 暗い背景に彩度の低い青 1 色のアクセント。どの SaaS LP とも見分けがつかない。
- 範囲はユーザーが決定済み (2026-10-25): **トップ + 全ページの基調** (色・タイポ・サイドバー・本文の見た目)。
- 方向性はユーザーが決定済み (2026-10-25): **先に方向性を複数案で比べてから実装する**。
  ユーザーは**動画を撮る**ことも検討している。各案に動画の置き場 (hero か、機能説明か) を入れる。
- 素材: 実エディタのスクショ・GIF が `docs/docs/assets/images/` に既にある
  (`editor-overview.png`、`cellop.gif`、`data-view-reference-viewer.gif`、`data-view-move-row.gif` など)。
  ユーザーの手元素材は `C:\Users\koori\Downloads\DataIndexer\Thumbnail\` にある。
- 3 案の軸 (どれも「テンプレっぽさの要因」1〜6 の少なくとも 4 つを使わない):
  - A 案: UE エディタの実画面が主役。hero は動画 / GIF を大きく置き、装飾的な LP 要素を削る。ツールらしい情報密度。
  - B 案: 独自のブランド表現。DataIndexer 固有の色・書体・図版モチーフ (例: GUID・行・参照線) を作る。
  - C 案: 実行者が A・B と明確に違う方向を 1 つ選ぶ (例: ドキュメント主体で LP をほぼ持たない構成)。
- 重複チェック通過 (0件対象 — この時点でタスク置き場は空)。

## Scope
- Artifact Design canvas を 1 枚作り、公開する。
- 各案に次の 3 つを描く。
  1. トップ hero から最初の 2 セクションまで (幅 1440px)。動画の置き場を示す枠を含む。
  2. ドキュメント本文ページ 1 枚 (例: `concepts/schema`) のヘッダ・サイドバー・本文・コードブロック・admonition。
  3. 各案の配色 (light / dark) と書体の見本。
- 各案に、上の要因 1〜6 のどれを捨てたかを 1 行で書く。
- 文言は en を使う。ja の見出しが崩れないことを 1 箇所だけ示す。

## Out of Scope
- `docs/` 配下のファイルの変更 (実装は `t261025-dbeb`)。
- 動画の撮影・編集。
- サイトの文章構成の書き直し (見出しの言い回しを変えるのは canvas 上の提案としてだけ可)。

## Preconditions
- なし。

## Steps
1. `python -m mkdocs build` で現状をビルドし、トップと `concepts/schema` のスクショを撮る (比較用の「現状」)。
2. canvas に「現状」+ A / B / C の 4 列を作る。
3. Scope の 3 つを各案に描く。実画像は `docs/docs/assets/images/` の実ファイルを使い、エディタ画面を CSS で描いた偽物にしない。
4. 各案に「捨てた要因」と「動画の置き場」を注記する。
5. canvas を公開し、URL をタスクの Summary に書く。

## Acceptance Criteria
- canvas が公開され、URL が Summary にある。
- 「現状」と A / B / C の 3 案が並んでいる。
- 各案に hero〜2 セクション・本文ページ・配色と書体の見本の 3 つがある。
- 各案が要因 1〜6 のうち 4 つ以上を使っていない。どれを捨てたかが注記されている。
- 各案に動画の置き場が描かれている。

## Tests
- なし (成果物はデザイン)。

## Verification
- canvas を light / dark で開き、スクリーンショットを Execution Log に残す。

## Execution Log
- 2026-10-25: メインスレッドが Design canvas を作成し、素材画像 10 枚を asset としてアップロードした。現状のスクショ 2 枚 (トップと concepts/schema、dark、1440px 幅) も含む。
- 2026-10-25〜26: canvas の中身を task-executor-canvas (max) が書いた。13 ファイル、所要 38 分。executor の自作 lint はエラー 0 件、テキストの最低コントラストは 4.73。
- 2026-10-26: メインスレッドが canvas.json と 12 枚のボードを公開した (version 3)。
- Verification は未了: ブラウザペインは claude.ai にサインインしていないので、canvas を開けなかった。
- 既定で進めて記録:
  - 本文ページはトップと逆のテーマで描いた (A は light、B と C は dark)。両方のテーマをページの大きさで見せるため。
  - テーマは props のトグルにせず固定にした。
  - ja 見出しは `lang="ja"` + `word-break: auto-phrase` + `text-wrap: balance` で組んだ。t261025-dbeb に渡す。
- 2026-10-26: ユーザーが canvas を見て決めた。トップは A、配色と本文ページは B、書体は IBM Plex、C は採らない。動画は poster で先に出す。
- 2026-10-26: A+B の統合案の行 (AB-Home / AB-Docs / AB-Spec) を task-executor-canvas が書いた (34 分)。メインスレッドが公開した (version 4)。

## Summary
canvas: https://claude.ai/artifact/8uPTrWirULD7DSRCRAPPYs

- 現状: スクショ 2 枚と凡例 (Main)。凡例には要因 1〜6、各案が捨てた要因、動画の置き場を載せた。
- A エディタが主役:
  - 色は UE の灰と、行の選択色の琥珀 #f8b800。書体は IBM Plex。
  - hero 全幅の 16:9 に動画を置く。
  - 1〜5 を捨てた。6 は暗い背景を残し、青を琥珀に替えた。
- B 台帳 (Ledger):
  - 色は紙・インク・キーの緑。書体は Archivo 系。
  - hero は「参照 → GUID キー → リネーム」の図版。
  - 2 つ目のセクションに動画を置く。
  - 1, 3, 4, 5, 6 を捨てた。
- C マニュアル:
  - LP を持たない。トップは本の扉と目次。色は白・インク・深紅 #9b1d33。書体は Source Serif。
  - タイトル横の Figure 1 に動画を置く。
  - 1〜6 すべてを捨てた。

## Changed Files
- repo の変更は無い。成果物は canvas (Artifact) だけ。

## Blockers
なし。Verification はユーザーが canvas を自分で開いて確かめ、統合案を承認した (2026-10-26)。ブラウザペインは未サインインのため、メインスレッドのスクリーンショットは無い。
