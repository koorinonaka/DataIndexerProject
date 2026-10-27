---
status: done
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
  - 2026-10-26 (quick): `cd docs; python -m mkdocs build -d <scratchpad>/site` → exit 0、「Documentation built in 1.36 seconds」。ERROR / WARNING は 0 件 (INFO のアンカー欠け 3 件は変更前のビルドにもある)。full は run 終端。

## Verification
- ビルド結果を `python -m http.server` で配信し、トップと `concepts/schema` を
  en / ja × light / dark × 幅 1440px / 375px でスクリーンショットに撮り、canvas の選んだ案と並べて Execution Log に残す。
- 2026-10-26 実施 (ALICE、内蔵ブラウザ)。16 通りすべてで寸法表との不一致 0、幅 375 で横スクロール無し。
  証跡: `.agents/tasks/evidence/t261025-dbeb-docs-site-restyle/report.html` (測った値の表 + スクリーンショット 16 枚)。
  canvas は support.js のランタイムが要り、ローカルでは描けない。並べる相手は canvas の画像ではなく、AB-Spec の寸法表の数値にした。

## Execution Log
- 2026-10-26 07:45 着手 (dispatch [1/1])。前提 `t261025-4983` は done。canvas の出典 (AB-Home / AB-Docs / AB-Spec) はローカルにあった。
- 方針 (Advisor に相談して確定):
  - `tokens.css` を AB-Spec のトークンだけに書き直した。旧ファイルの `.di-doc` などの artboard 用規則は、サイトのどこからも使われていない (grep で 0 件) ので消した。
    色は `[data-md-color-scheme="di-light"]` と `[data-md-color-scheme="slate"]` に置く。旧 `extra.css` の `[data-md-color-scheme="default"]` は、`mkdocs.yml` の `di-light` と食い違っていて効いていなかった (canvas 注意 6)。
  - `extra.css` を基調の 1 枚として書き直した。`--md-*` をトークンに渡し、ヘッダー・段組・サイドバー・本文・コード・note・表・タブ・前後リンク・フッターを寸法表の値で組んだ。
  - `c-variant.css` から基調の規則 (Inter / JetBrains Mono、青いアクセント、角丸、影、タブのアイコン、旧ヘッダー・旧サイドバー) を外し、部品 (Quick Start・Changelog・Concepts の図・画像の拡大) だけを残してトークンで塗り直した。
    使われていない部品 (`di-qr`・`quickref`・`ov-sample*`・`ov-feature-desc`・`ov-eyebrow`・`ov-mm-eyebrow`・`qs-progress-eta`・`release-group--break`) は消した (md / overrides で grep 0 件)。
  - 書体: `theme.font: false` にして、`main.html` の `fonts` ブロックで IBM Plex Sans 400/500/600・Mono 400/500・Sans JP 400/500/700 を読む。`theme.font` のままだと 300/400/700 しか読まれず、500/600 が合成になる。`document.fonts` で 7 書体とも loaded を確認した。
  - ヘッダー: `partials/header.html` を上書きして 1 行にした。`navigation.tabs` / `navigation.tabs.sticky` を外し、最上位ナビは Material の tabs-item マクロでヘッダーの中に描く (`data-md-component="tabs"` を付けないので、スクロールで隠す JS は動かない)。言語は `partials/alternate.html` で「EN · JA」の文字リンクにした。
  - サイドバー: canvas は全セクションを出し、今いないセクションを畳む。そこで `navigation.sections` / `navigation.expand` を外し、`navigation.indexes` は残した。`toc.integrate` を外して右の目次を出した (canvas 注意 1・2)。
  - フッター: `partials/footer.html` で 56px の 1 行 (著作権・版・変更履歴・Get on Fab)。版は `extra.di_version: v1.0.1` (changelog の最新版と一致)。前後リンクは canvas どおり本文の末尾に `main.html` で置いた。
  - メタ行: `partials/path.html` を URL の段 (`concepts / schema`) を出す形にした。親セクションが index を持てば、その段はリンクになる。
  - トップ: `overview.html` と `home.css` を AB-Home の構成で書き直した。en / ja は同じ構成で文言だけを切り替える。ja の文言は旧トップの ja 文をそのまま流用した (新しく書いた日本語の散文は無い)。画像の alt は旧トップと同じく英語のまま。
  - コード: トップのコードは canvas の手塗りの span をそのまま使い、highlight.js (CDN 2 本) をやめた。タブと copy は `overview-tabs.js` で動かす (canvas の「押しても動かない部品は置かない」)。Blueprint タブは旧トップの構造体の図を出し、copy を隠す。
  - 動画: `extra.hero_video` が空なら poster (editor-overview.png) の `<img>` を出す。パスを書くと `<video autoplay muted loop playsinline poster>` に替わる。`hero_video: assets/videos/overview.mp4` を入れた一時設定でビルドし、en / ja とも `<video … poster=…editor-overview.png>` と `<source src=…overview.mp4>` が出ることを確かめた (一時設定は削除)。
- 既定で進めた選択 (どれも戻せる):
  - `--hl-string` / `--hl-num` (AB-Spec に無い): light は `#7a4a0a` (--bg-sunken `#ebe5d6` に対して 5.94:1) と `#1d5a8c` (5.78:1)。dark は `#e0c07a` (コード面 `#0b0f0d` に対して 11.02:1、--bg-code `#1b241f` に対して 9.09:1) と `#8cc4f0` (10.35:1 / 8.55:1)。比は WCAG 2 の式でスクリプトで計算した。どれも 4.5:1 以上。
  - Pygments の C++ は `UCLASS` / `UPROPERTY` / 型名を `.n` (名前) として出す。そのため本文ページのコードではマクロ色と型色が付かない (`class` 宣言の型名 `.nc` と `#include` の `.cp` だけに付く)。トップのコードは手塗りなので canvas どおりに付く。
  - テーマ: 「OS の設定に従い、無ければ light」。Material は初回表示時に選んだパレットを localStorage に保存し、以後は OS の変更に追従しない (実測: 保存値 `{"index":0,…"scheme":"di-light"}` が残り、OS を dark にしても light のまま)。そこで `media: "(prefers-color-scheme)"` の自動モードを先頭に足した。トグルは 自動 → light → dark の順に回る。localStorage を消して OS を dark にすると slate、light にすると di-light になることを確かめた。
  - ロゴ: canvas は light の紙の上でも暗いバッジ (`logo-di.svg`) を使っている。`theme.logo` を `logo-di.svg` にし、slate で差し替える規則を消した。`logo-di-light.svg` はもう参照されない (ファイルは残した)。
  - note 以外の admonition (tip / info / example / warning): note と同じ形。warning 系の太罫と印は `--hl-keyword` の色にした。ラベルは種類名 (note / tip / warning …)。題を書いていない 9 個は「note Note」のように種類名が重なる。
  - H2 の右端のアンカー: en は `#role` のように id を出す。ja の id は `_1` のような連番なので「#」だけを出す。幅 45em 未満も「#」だけを出す。
  - 本文ページのメタ行は canvas の「· UDataIndexerSchema」(ページごとの主役の型) を出していない。ページごとに front matter を足す必要があり、本文 md の変更は Out of Scope。
  - 角 0 の例外: キーの印 (半径 3) のほか、Quick Start の進捗の丸と番号の円、Concepts の章番号の円は円のまま残した。Changelog の種類の印 (幅 4) は半径 2 (AB-Spec の「幅 4 の印は 2」)。矩形のカード・図・画像の角丸と影は 0 にした。
  - 1220〜1375px では 1 行に収まらない。この幅ではロゴの語を隠し、間を詰める (ナビの間 20 → 14、群の間 24 → 16)。1376px 以上は寸法表どおり。1220 / 1300 / 1375 / 1376 / 1440 で、ナビの右端と右の群の左端が重ならないことを en / ja / schema で確かめた。
  - 検索欄は 160px 幅なので、入力の補完候補 (`search.suggest`) の薄い字を幅 960 以上で出さない。機能は `mkdocs.yml` に残した。
  - 使われなくなったアセット: `logo-di-light.svg` と、旧タブのアイコン `assets/icons/icon-{book,arrow,cube,edit,search,log}.svg`。ファイルは残した。
  - 幅 1220 未満はナビをドロワーへ移す (Material の既定)。幅 960 未満は Get on Fab をヘッダーから外す (ドロワーの source に残る)。幅 480 未満は言語を相手側だけ (EN か JA) にする。
- 検証で見つけて直したもの:
  - `.md-content__inner` の左右 24px が、Material の `:not([hidden])` 付きの規則 (詳細度 0,5,0) に負けて残っていた → 詳細度を上げた。
  - admonition の枠色が Material の種類別の規則 (0,3,0) に負けて青だった → 直した。
  - 幅 375 で `/concepts/schema/` と `/api-reference/schema-api/` (en / ja) に横スクロールが出た (H2 / H3 のアンカーと長い識別子) → アンカーを縮められるようにし、見出しを `overflow-wrap: anywhere` にした。
- `toc-fix.js` は変更していない。この JS は左のサイドバー内の `#` リンクに現在地を付ける。`toc.integrate` を外したので、その `#` リンクは幅 1220 以上では隠れた目次 (ドロワー用) にしか無く、見た目には効かない。`.md-tabs` が無いので補正量は 64 + 42 (既定値) + 2 = 108 になり、見出しの `scroll-margin-top` 80 と合わない。右の目次の現在地は Material 自身が付ける。
- 寸法の検証 (内蔵ブラウザ、ビルド結果を `python -m http.server` で配信): `getBoundingClientRect` / `getComputedStyle` を読むスクリプトで、トップと `concepts/schema` を en / ja × light / dark × 1440 / 375 の 16 通り測った。寸法表との不一致 0、幅 375 の横スクロール 0。
  幅 1440 はスクロールバー 15px を含む。そのためトップの内容幅は 1265 (スクロールバーの無い 1440 なら 1280)、本文ページの段落の幅は 760。
  sitemap の全 48 ページ × 幅 375 / 768 / 1440 で `scrollWidth > clientWidth` は 0 件。
  トップのタブ: Blueprint を押すと C++ の枠が隠れ、Blueprint の枠が出て copy が隠れる。C++ を押すと元に戻る。
- 要因 1〜6 (`t261025-4983` の Context) のうち、トップから消えたもの: 1 (eyebrow)、2 (2 行見出し + ボタン 2 つ + meta チップ)、3 (セクションごとの eyebrow → 見出し → リード)、4 (「01 / MAINTAINABILITY」)、5 (✕ / ✓ の比較表) の 5 つ。6 は既定の light では消えた (紙 + 緑)。dark は「暗い背景 + 緑 1 色」の形が残る (AB-Spec の想定どおり)。
  ビルド結果のトップ (en / ja) で、旧クラス・旧文言 (`UNREAL ENGINE PLUGIN`・`ov-eyebrow`・`ov-pulse-dot`・`01 /`・`ov-ba`・`#2e6ee0`・`highlight.min.js`) は各 0 件。
- Critic (task-critic) round 1:
  ```
  PASS
  NOTE: docs/docs/assets/javascripts/toc-fix.js:9-10 — `.md-tabs` がもう無いので `t` は null になる。そのため offset は 64 + 42(既定値) + 2 = 108 になり、新しい scroll-margin-top (header-h + 16 = 80、extra.css:551) と合わない。Execution Log は「何もしない」と書いているが、`.md-sidebar--primary` に `#` リンクがあれば補正は今も動く。toc.integrate を外したので、実害は出にくい。
  NOTE: docs/docs/assets/icons/icon-{book,arrow,cube,edit,search,log}.svg と docs/docs/assets/images/logo-di-light.svg — 最後の参照元 (旧 c-variant.css のタブアイコン規則と mkdocs.yml の theme.logo) がこの差分で消え、リポジトリ内の参照は 0 件になった。どれもアセットファイルでコードの定義ではないので、kind 5 にはしない。logo-di-light.svg は Execution Log が残すと明記している。タブアイコン 6 つは記載が無い。
  NOTE: docs/docs/assets/stylesheets/c-variant.css:76,146,190,251,496 — 寸法表の「角 0 (キーの印だけ半径 3)」に対し、border-radius 50% の円が残っている (Quick Start の進捗の丸・番号、Concepts の章番号の円)。c-variant.css:290 は changelog の印に 2px を足している。circle に角は無く、canvas 自身も AB-Docs.dc.html:152-154 で 2px を使う。そのため FAIL にはしない。Log の「角 0 の例外」の項は 2px の印に触れていない。
  NOTE: docs/docs/assets/stylesheets/extra.css:232 — mkdocs.yml の features に `search.suggest` が残る一方、この行は `.md-search__suggest { display: none; }` にしている (メディアクエリの中の可能性あり)。機能が実際に要るかは確かめていない。
  NOTE: 受け入れ条件の「en/ja × light/dark で canvas と一致」「幅 375 で横スクロール無し」は画面でしか確かめられない。差分からは判定できない。根拠は作者の証跡 .agents/tasks/evidence/t261025-dbeb-docs-site-restyle/report.html だけで、この証跡は canvas の画像ではなく寸法表の数値と比べている (Verification に明記)。
  NOTE: 実施した確かめ。docs/docs/**/*.md と JS が使うクラス (qs-*・step-label・release*・ov-mm-*・ov-chapter*・icon-*・fn-node) は、すべて変更後の CSS に定義が残っている。tokens.css と c-variant.css の新しいカスタムプロパティは、すべて参照されている。extra.css と home.css の di-* クラスは、すべてテンプレートが使っている。削除した ov-dual・di-doc・highlight.min.js・`data-md-color-scheme="default"` を参照する箇所は、リポジトリ (.agents を除く) に残っていない。差分は本文 *.md と MkDocs / Material のバージョンを触っていない。
  ```
  NOTE の 1〜4 は Log の該当項を直して答えた (toc-fix の説明、未使用アセット、半径 2 の印、補完候補)。
- 自分の差分の点検 (`git diff --stat` + `git status --porcelain`): 触ったのは Context の変更対象の一覧にあるファイル (`tokens.css` / `extra.css` / `c-variant.css` / `home.css` / `overview.html` / `partials/*.html` / `main.html` / `mkdocs.yml` / `overview-tabs.js`) と証跡だけ。新規の partial は header / footer / alternate の 3 つ。本文 md と MkDocs / Material のバージョンは触っていない。

## Summary
AB 統合案 (A のトップ構成 + B の紙・インク・緑 + IBM Plex) を docs サイトに反映した。トークンは `tokens.css`、基調は `extra.css`、部品は `c-variant.css`、トップは `overview.html` + `home.css` が持つ。ヘッダー・フッター・言語・メタ行は partial の上書きで canvas の形にした。トップと `concepts/schema` の 16 通りで寸法表と一致し、幅 375 で横スクロールは無い。動画は `extra.hero_video` にパスを書けば差し替わる。

## Changed Files
- `docs/mkdocs.yml`
- `docs/overrides/main.html`
- `docs/overrides/overview.html`
- `docs/overrides/partials/header.html` (新規)
- `docs/overrides/partials/footer.html` (新規)
- `docs/overrides/partials/alternate.html` (新規)
- `docs/overrides/partials/path.html`
- `docs/docs/assets/stylesheets/tokens.css`
- `docs/docs/assets/stylesheets/extra.css`
- `docs/docs/assets/stylesheets/c-variant.css`
- `docs/docs/assets/stylesheets/home.css`
- `docs/docs/assets/javascripts/overview-tabs.js`
- `.agents/tasks/evidence/t261025-dbeb-docs-site-restyle/` (report.html + スクリーンショット 16 枚)

## Blockers
None
