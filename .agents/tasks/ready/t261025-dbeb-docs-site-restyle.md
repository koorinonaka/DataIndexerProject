---
status: ready
order: 20
tier: standard
ds: AB
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
- ユーザーが統合案を承認した (2026-10-26、「これで進めて」)。
- 統合案の出典 (同じマシンのローカルにある。読むだけ):
  - フォルダ: `C:/Users/koori/AppData/Local/Temp/claude/E--workspace-DataIndexerProject--claude-worktrees-webpage-brushup-f06e9d/51965596-7144-4ddf-9788-b4785a1ef74e/scratchpad/canvas/project/`
  - `AB-Spec.dc.html`: トークンと寸法の正。値はすべて `box-sizing: border-box` で測ってある。
  - `AB-Home.dc.html` / `AB-Docs.dc.html`: トップと本文ページの構成と inline style。
  - フォルダが消えていたら、canvas https://claude.ai/artifact/8uPTrWirULD7DSRCRAPPYs の同名ファイルを見る。
- 色トークン (AB-Spec、light / dark):

  | token | light | dark |
  |---|---|---|
  | --bg | #f3efe4 | #0f1411 |
  | --bg-alt | #faf7ef | #151c18 |
  | --bg-sunken | #ebe5d6 | #0b0f0d |
  | --bg-code | #ebe5d6 | #1b241f |
  | --border | #d8d0bd | #26302b |
  | --border-strong | #bfb59e | #26302b |
  | --text | #1c1a15 | #e6ebe5 |
  | --text-muted | #5c5649 | #a3ada5 |
  | --text-faint | #5c5649 | #8a958d |
  | --accent | #0f6b45 | #3ecf8e |
  | --accent-hover | #0a4d31 | #7fe3b5 |
  | --mark | #16b978 (文字に使わない) | #3ecf8e |
  | --button-bg / -fg | #1c1a15 / #f3efe4 | #3ecf8e / #0f1411 |
  | --hl-fg | #1c1a15 | #dfe6df |
  | --hl-macro | #5b3f9e | #b9a3f0 |
  | --hl-keyword | #8a2c5c | #e39ac0 |
  | --hl-type | #0f6b45 | #3ecf8e |
  | --hl-comment | #5c5649 | #7f8b83 |

  `--hl-string` と `--hl-num` は AB-Spec に無い。実行者が 4.5:1 以上で決め、Execution Log に値と比を書く。
- 既定のテーマは light。OS の設定に従い、設定が無ければ light を出す。

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
- 寸法表 (AB-Spec が出典。単位 px。外形 = border-box の外寸、内側 = 枠と padding の内側)。次の値が実装と一致する:

  | 部品 | 値 | 基準 |
  |---|---|---|
  | ヘッダー | 高さ 64 (下罫 1px 込み)、左右 padding 40、折り返さない | 外形 |
  | ロゴ | 28×28、語まで 10、語は Sans 600 18/24 | 外形 |
  | ナビ | Sans 500 14/20、項目の間 20、現在地は下線 2px --accent | — |
  | 検索欄 | 160×34 (枠 1px 込み)、左右 padding 12 | 外形 |
  | ボタン | 高さ 36、左右 padding 16、Sans 600 14/20 | 外形 |
  | フッター | 高さ 56 (上罫 1px 込み)、左右 40。版 (v1.0.1) を置く | 外形 |
  | 角 | 0 (キーの印だけ半径 3) | — |
  | 本文ページの列 | 280 : 残り : 248 | 外形 |
  | サイドバーの行 | 高さ 36 (上罫 1px 込み)、Sans 14.5/20、子の字下げ 18 | 外形 |
  | 本文列 | padding 40 64 72、max-width 760 | 760 は内側 |
  | H1 (本文ページ) | Sans 600 52/60、-0.01em | — |
  | H2 | Sans 600 24/32、上に太罫 2px、罫から字まで 14 | — |
  | H3 | Sans 600 19/28 | — |
  | 本文 | Sans 400 17/28、インラインコード Mono 15 | — |
  | コードブロック | Mono 400 13.5/21、頭は高さ 34 (下罫 1px 込み) | 外形 |
  | note (admonition) | 上に太罫 2px --accent、下罫 1px、padding 12 0 14 | — |
  | 右の目次 | 左罫 1px、padding 40 32 40 24、行は min-height 34 | 外形 |
  | トップの内容幅 | max-width 1280、左右 80 | 1280 は内側 |
  | トップ h1 (Display) | Sans 600 40/48、-0.01em。ja は 700 40/52 | — |
  | リード文 | Sans 400 18/28、--text-muted | — |
  | 動画枠 | 16:9。1440 幅で 1280×720。枠線は outline 1px で寸法に入れない | 内側 |
  | セクション | padding 64 80 (最後だけ下 72)、見出しの太罫 2px | — |
  | タイル | 4 列、間 16、枠 1px --border-strong、面 --bg-alt、画像 16:10 | 画像は内側 |

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
なし
