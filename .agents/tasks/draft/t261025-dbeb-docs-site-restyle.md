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
- ユーザーの決定 (2026-10-26、canvas https://claude.ai/artifact/8uPTrWirULD7DSRCRAPPYs を見て):
  - トップは A 案「エディタが主役」の構成にする。理由: 非プログラマにも届く。
  - 配色は B 案「台帳」の紙・インク・キーの緑にする。理由: プラグイン本体の緑基調のテーマと合う。
  - 本文ページ (concepts/schema) は B 案の形にする。
  - 書体は A 案の IBM Plex (Sans / Mono / Sans JP) にする。
  - C 案は採らない。理由: 無骨すぎる。
  - 動画は待たない。poster 画像で先に出し、撮れたら差し替える。
  - 実装の前に、A+B の統合案 (トップ・本文・配色) を canvas に足して確かめる。
- 統合案 (canvas の「AB — 統合案」の行、2026-10-26 に公開、version 4):
  - AB-Home: A のトップの構成・文言・画像 + B の配色 + IBM Plex。既定は light (紙)。見出しの上に 2px の太罫、角は 0。動画枠は poster だけ。
  - AB-Docs: B-Docs の形を IBM Plex で組んだ。dark で描いた。ヘッダーとフッターは AB-Home と共通。
  - AB-Spec: light / dark のトークン、書体の寸法表 (値ごとに外形 / 内側の基準付き)、ja 見出し、本文の部品。寸法表はここを出典にする。
  - 書体の倍率: 本文ページの見出しは B の約 0.8 倍 (h1 52/60、h2 24/32)。等幅は 1 段上げた (コード 13.5/21、ラベル 12/16、インライン 15)。
- canvas executor が残した実装上の注意 (2026-10-26、未検証):
  1. AB-Docs は右に目次を持つ。`mkdocs.yml` の `toc.integrate` を外す。
  2. `navigation.tabs` のままだと、Material はタブを 2 行目に出す。1 行のヘッダーにするなら `header.html` か tabs を上書きする。
  3. `theme.font` を text: IBM Plex Sans、code: IBM Plex Mono にする。Sans JP は extra_css で読む。
  4. `--hl-string` と `--hl-num` は B の配色に無い。ここで決める。
  5. 実際の色は `extra.css` の `--md-*` を通して効く。`tokens.css` だけを変えても効かない。
  6. `mkdocs.yml` は `scheme: di-light` と書き、`extra.css` は `[data-md-color-scheme="default"]` を対象にしている。食い違いの可能性がある。
- 昇格レビュー時に埋める: 統合案の canvas ボード (AB-Home / AB-Docs / AB-Spec) から写す寸法・色の値。値ごとに基準 (外形か内側か) を書く。

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
canvas の A+B 統合案 (AB-Home / AB-Docs / AB-Spec) をユーザーが確かめるのを待つ。
