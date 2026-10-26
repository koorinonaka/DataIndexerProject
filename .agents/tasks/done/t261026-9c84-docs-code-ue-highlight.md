---
status: done
order: 30
tier: standard
screen: required
agent: any
created_by: task-create
---

# docs のコードブロックで UE のマクロと型名に色を付ける

## Goal
docs の本文ページのコードブロック (C++) で、UE のマクロ (`UCLASS` / `UPROPERTY` / `GENERATED_BODY` など) を `--hl-macro` の色、UE の型名 (`F*` / `U*` / `A*` / `E*` / `T*` で始まる型) を `--hl-type` の色で表示する。トップのコード枠 (`.di-codepane`、手塗り) と同じ見え方に揃える。

## Context
- 重複チェック通過 (2件対象)。
- 起点: t261025-dbeb (done) の終わりにユーザーが「タスク化」と答えた (2026-10-26)。承認済みの follow-up なので ready に直接置く。
- 原因 (2026-10-26 にビルド結果で確かめた): Pygments の C++ lexer は `UCLASS` と `FItemRow` を、どちらも普通の名前 `<span class=n>` として出す。CSS の色だけでは区別できない。
  - 例: `concepts/schema/index.html` の `<span class=n>UCLASS</span><span class=p>()</span>`。
- 色トークンは既にある: `docs/docs/assets/stylesheets/tokens.css` の `--hl-macro` / `--hl-type` (light / dark 両方)。トップは `home.css` の `.di-codepane .m` / `.t` で使っている。
- `extra.css` は Material の `--md-code-hl-special-color` に `--hl-macro`、`--md-code-hl-function-color` に `--hl-type` を当てている。どちらも `.n` には効かない。
- 方式の候補 (実行者が選び、理由を Execution Log に書く):
  1. JS の後処理: `extra_javascript` に 1 本足す。`.highlight` の中の `span.n` を走査し、正規表現でマクロと型名にクラスを足す。Material の instant navigation (`document$` の購読) にも対応する。
  2. カスタム Pygments lexer: Python パッケージとして登録が要る。GitHub Pages のビルド環境にも入れる必要がある。
  既定は 1 (依存が増えず、ビルド環境を変えない)。
- マクロの判定: 全部大文字と `_` の識別子で、直後が `(`。例 `UCLASS(`、`DI_DEFINE_INDEX(`。
- 型名の判定: `^[FUAETI][A-Z][A-Za-z0-9]*$`。例 `FItemRow`、`UItemSchema`、`TObjectPtr`。`Row` のような変数名は対象外。

## Scope
- 方式 1 なら `docs/docs/assets/javascripts/` に 1 ファイルを足し、`docs/mkdocs.yml` の `extra_javascript` に登録する。
- `extra.css` に、足したクラスの色 (`--hl-macro` / `--hl-type`) を書く。

## Out of Scope
- 本文 md の変更。
- トップの `.di-codepane` の変更。
- 色トークンの値の変更。

## Preconditions
- t261025-dbeb が done (済み)。

## Steps
1. 方式を決める (既定は JS の後処理)。
2. マクロと型名を判定してクラスを足す処理を書く。ページ遷移後にも走るようにする。
3. `extra.css` に色を書く。
4. ビルドし、Verification のとおり確かめる。

## Acceptance Criteria
- `concepts/schema` の最初のコードブロックで、`UCLASS` が `--hl-macro` の色、`FItemRow` が `--hl-type` の色で描かれる (light / dark とも)。今は両方とも本文の文字色。
- 変数名 (`Row` など) と関数名の色は変わらない。
- 別ページから `concepts/schema` へ instant navigation で移っても色が付く。

## Tests
- `cd docs; python -m mkdocs build` がエラー無しで終わる。

## Verification
- ビルド結果を `python -m http.server` で配信する。`concepts/schema` と `concepts/indexes` のコードブロックを light / dark でスクリーンショットに撮る。`getComputedStyle` で `UCLASS` と `FItemRow` の色がトークンの値と一致することを確かめ、Execution Log に残す。

## Execution Log
- 2026-10-26: 方式は 1 (JS の後処理) にした。理由: 依存が増えず、GitHub Pages のビルド環境も変えない。Material の `document$` を購読するので、instant navigation を後で有効にしても遷移ごとに走る。
- 判定: マクロは「全部大文字と `_`、直後が `(`」。型名は「`F/U/A/E/T/I` + 大文字で始まり、小文字を 1 つ以上含む」。
- サイト全体のビルド結果で、色が付く名前を全部列挙した。最初の正規表現では `UI` が型名と誤判定された。小文字を必須にして直した。
  - マクロ (12): DI_DEFINE_INDEX, DI_REGISTER_BUILD_INDEX, GENERATED_BODY, GET_MEMBER_NAME_CHECKED, NSLOCTEXT, TEXT, UCLASS, UENUM, UE_LOG, UFUNCTION, UPROPERTY, USTRUCT
  - 型名 (55): AGameModeBase, FItemRow, TObjectPtr, UDataIndexerSchema, UTexture2D など。誤判定は無い。
- 実測 (`getComputedStyle`、ビルド結果を http.server で配信):
  - light / concepts/schema: `UCLASS` rgb(91,63,158) = --hl-macro #5b3f9e。`FItemRow` rgb(15,107,69) = --hl-type #0f6b45。`Row` rgb(28,26,21) = --hl-fg #1c1a15 (変わらない)。
  - dark / concepts/indexes: `UCLASS` と `DI_DEFINE_INDEX` rgb(185,163,240) = --hl-macro #b9a3f0。型名 rgb(62,207,142) = --hl-type #3ecf8e。
- instant navigation はこのサイトで無効 (`navigation.instant` が features に無い)。そのため AC の「instant navigation で移っても」は、`document$` の購読で満たす形にした。実遷移では確かめていない。
- critic r1:
  ```
  PASS
  NOTE: docs/docs/assets/javascripts/ue-highlight.js:7 — `.highlight span.n` は C++ に限らず全言語のコードブロックに当たる。Python や JSON のブロックでも `UPPER(` や `FFoo` の形の名前は色が変わる。今は害なし。
  NOTE: docs/docs/assets/javascripts/ue-highlight.js:4 — 型名の正規表現は、task の Context にある `^[FUAETI][A-Z][A-Za-z0-9]*$` より狭い (小文字を必須にした)。理由 (`UI` の誤判定) は Execution Log に書いてある。AC の `FItemRow` は当たる。
  NOTE: .agents/tasks/done/t261026-9c84-docs-code-ue-highlight.md:67 — AC の 3 番目 (instant navigation で遷移したときの色) は、実際の遷移では確かめていない。このサイトでは `navigation.instant` が無効で、diff は `document$.subscribe` に頼っている。AC の 1 番目 (light / dark の色) も画面でしか確かめられない。Execution Log の `getComputedStyle` 実測値は、tokens.css:32/34/66/68 の値と一致する。
  ```
- critic の NOTE 1 を折り込んだ: 対象を `.language-cpp.highlight span.n` に絞った。
本文のコードブロックで、UE のマクロを `--hl-macro`、型名を `--hl-type` で塗るようにした。Pygments の出力 (`span.n`) を JS で後から分類する。

## Changed Files
- docs/docs/assets/javascripts/ue-highlight.js (新規)
- docs/mkdocs.yml (extra_javascript に登録)
- docs/docs/assets/stylesheets/extra.css (.ue-macro / .ue-type の色)

## Blockers
None
