---
status: draft
order: 20
tier: standard
screen: required
agent: any
created_by: task-create
---

# 選んだ方向性で docs サイト (トップ + 全ページの基調) を作り直す

## Goal
`t261025-4983` の canvas でユーザーが選んだ案を、docs サイトのトップ (overview) と全ページの基調
(色・タイポ・サイドバー・本文) に反映する。en / ja、light / dark の 4 通りで、選んだ案と同じ見た目になる。

## Context
- 起点の調査と「テンプレっぽさの要因」1〜6 は `t261025-4983` の `Context` にある。
- 変更対象になるファイル (2026-10-25 時点):
  - `docs/overrides/overview.html` (en / ja 両方を `is_ja` で持つ)、`docs/docs/assets/stylesheets/home.css`。
  - `docs/docs/assets/stylesheets/tokens.css` / `extra.css` / `c-variant.css`、`docs/overrides/partials/*.html`、`docs/mkdocs.yml` の `theme.palette` / `theme.font`。
  - `docs/docs/assets/javascripts/overview-tabs.js` (トップのタブ)。
- 動画: ユーザーが撮る予定 (2026-10-25)。置き方の既定は、`docs/docs/assets/` に置いた mp4 を
  `<video autoplay muted loop playsinline poster="…">` で埋める形。GitHub Pages は 1 ファイル 100MB が上限なので、
  目安 10MB 以下に圧縮する。動画が無い時点では poster 画像 (既存スクショ) だけで成立させる。
- 昇格レビュー時に埋める:
  - 選ばれた案 (A / B / C) と、canvas から写す寸法・色の値 (値ごとに基準 — 外形か内側か — を書く)。
  - 動画を待つか、poster だけで先に出すか。

## Scope
- 選んだ案の配色・書体・余白を `tokens.css` のトークンに落とし、全ページに効かせる。
- トップ `overview.html` + `home.css` を選んだ案の構成に作り直す (en / ja 両方)。
- 動画枠を入れる。動画ファイルが無ければ poster 画像で表示する。

## Out of Scope
- ドキュメント本文 (`*.md`) の文章の変更。
- 動画の撮影・編集。
- MkDocs / Material のバージョン更新。

## Preconditions
- `t261025-4983` が done で、ユーザーが案を 1 つ選んでいる。
- (任意) ユーザーが動画ファイルを用意している。

## Steps
1. 昇格レビューで選ばれた案の値を `tokens.css` に入れる。
2. `extra.css` / `c-variant.css` / partials の、新トークンと食い違うハードコード値を置き換える。
3. `overview.html` と `home.css` を選んだ案の構成に書き直す。en と ja の両ブロックを同じ構成にする。
4. 動画枠を入れる。
5. ビルドし、Verification のスクリーンショットを撮る。

## Acceptance Criteria
- トップに、選んだ案で捨てた要因 (1〜6 のうち 4 つ以上) が残っていない。
- en / ja、light / dark の 4 通りでトップと `concepts/schema` が canvas の選んだ案と一致する。
- 幅 375px でトップに横スクロールが出ない。
- 動画が無い状態でも hero が poster 画像で成立している。
- (寸法表は昇格レビュー時に canvas から書く)

## Tests
- `cd docs; python -m mkdocs build` がエラー無しで終わる。

## Verification
- ビルド結果を `python -m http.server` で配信し、トップと `concepts/schema` を
  en / ja × light / dark × 幅 1440px / 375px でスクリーンショットに撮り、canvas の選んだ案と並べて Execution Log に残す。

## Execution Log
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
`t261025-4983` の canvas でユーザーが案を選ぶのを待つ。動画を待つかもその時点で決める。
