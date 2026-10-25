---
status: ready
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
Not started.

## Summary
Not started.

## Changed Files
Not started.

## Blockers
なし
