# DataIndexer Docs — デザイン実装ガイド

計測ソース: `docs/DataIndexer Docs Measurement.html`（v0.9.2, 1480×760）

## 原則

- このページの計測値は**絶対**。ピクセル単位で一致させる。
- 色・サイズは必ず `tokens.css` の `:root` から `var(--…)` 経由で参照。直書き禁止。
- 新規スタイルは `c-variant.css` の**末尾にのみ**追記。既存セレクタの書き換えは PR で説明必須。
- `mkdocs serve` で `concepts/repository/` をスクショし、計測ガイドと並べて diff が出ない状態にしてからコミット。

## パンくず（Breadcrumbs）

- 位置: 中央カラム最上段、H1 の **14px 上**。
- フォント: JetBrains Mono **12px / 500**、色 `--text-muted`。
- 区切り: 半角スラッシュ `/`、faint 色 `--text-faint`。
- 現在地: `--text-strong` + `font-weight: 600`、下線なし。
- ホバー時のみ `border-bottom: 1px dotted`。
- Material 標準の `navigation.path` 機能を有効化、見た目だけ CSS で差し替え。

## マージン・パディング

- ヘッダ高さ **52px** / セクションバー高さ **42px**。この 2 つは触らない。
- 左ナビ幅 **230px**（Material のデフォルト 240 から変更している）。
- 中央カラム `max-width: 820px`、`padding: 28px 48px 64px`。
- セクション余白は **4 の倍数**のみ使用（4 / 8 / 14 / 18 / 28 / 36 / 48 / 64）。
- 段落間: `p { margin: 0 0 14px; }`。最後の `p` の `margin-bottom` は**必ず保つ**（Material 既定の 0 にしない）。
- H2 は前 36px / 後 10px、H1 直下のリード段落は 28px 下マージン。

## フォント・サイズ感

- Sans = `Inter` → 日本語フォールバック `Noto Sans JP, Hiragino Sans, Yu Gothic`。Roboto・system-ui は使わない。
- Mono = `JetBrains Mono`。コード・パンくず・メタ表記はすべて mono。
- 本文 **15px / 1.65**。14 / 16 にしない。
- H1 **36 / 1.12 / -0.03em**。Material 既定の 32 ではなく 36。
- H2 **22 / 1.25 / -0.015em**。Material 既定の ~25 ではなく 22。
- H3 **16 / 1.35 / 600**。Material 既定の ~20 ではなく 16。
- リード段落は H1 直後の `<p>` に `.lead` クラス（または CSS で `h1 + p`）を当てて **17 / 1.55 / muted**。
- 全体トラッキング `-0.005em`。日本語のみのページでは無効化（`:lang(ja)`）。
- コードブロックは **warm near-black `#1a1917`**。Material の純黒にはしない。

## 検証手順（PR ごと必須）

1. `mkdocs serve` 起動 → `/ja/concepts/repository/` をスクショ。
2. `docs/DataIndexer Docs Measurement.html` と並べて diff。
3. 差分が出たら `c-variant.css` の該当行のみ修正。`tokens.css` は触らない。
4. パンくず・H1・Lead・コードブロックの 4 箇所だけは**必ず一致**させる。
