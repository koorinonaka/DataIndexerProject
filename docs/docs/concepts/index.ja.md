# コアコンセプト

DataIndexer は 4 つの相互に関連するコンセプトで構成されています。これらの関係を理解することで、他のすべてがつながります。

<div class="ov-mm-diagram" aria-hidden="true">
  <div class="ov-mm-node tone-purple">
    <div class="ov-mm-node-num">01</div>
    <div class="ov-mm-node-title">Schema</div>
    <code class="ov-mm-node-code">UDataIndexerSchema</code>
    <div class="ov-mm-node-text">Row 型・表示・Index を定義する設計図</div>
  </div>
  <div class="ov-mm-arrow">
    <div class="ov-mm-arrow-line"></div>
    <div class="ov-mm-arrow-label">defines</div>
    <svg width="20" height="12" viewBox="0 0 20 12" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M0 6h18m-4-4 4 4-4 4"/></svg>
  </div>
  <div class="ov-mm-node tone-blue">
    <div class="ov-mm-node-num">02</div>
    <div class="ov-mm-node-title">Repository</div>
    <code class="ov-mm-node-code">UDataIndexerRepository</code>
    <div class="ov-mm-node-text">Row を格納する型付きアセット</div>
  </div>
  <div class="ov-mm-arrow">
    <div class="ov-mm-arrow-line"></div>
    <div class="ov-mm-arrow-label">contains</div>
    <svg width="20" height="12" viewBox="0 0 20 12" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M0 6h18m-4-4 4 4-4 4"/></svg>
  </div>
  <div class="ov-mm-node tone-green">
    <div class="ov-mm-node-num">03</div>
    <div class="ov-mm-node-title">Keys &amp; Handles</div>
    <code class="ov-mm-node-code">FDataIndexerPrimaryKey</code>
    <div class="ov-mm-node-text">GUID で Row を一意に識別</div>
  </div>
  <div class="ov-mm-arrow">
    <div class="ov-mm-arrow-line"></div>
    <div class="ov-mm-arrow-label">resolves via</div>
    <svg width="20" height="12" viewBox="0 0 20 12" fill="none" stroke="currentColor" stroke-width="1.6"><path d="M0 6h18m-4-4 4 4-4 4"/></svg>
  </div>
  <div class="ov-mm-node tone-orange">
    <div class="ov-mm-node-num">04</div>
    <div class="ov-mm-node-title">Indexes</div>
    <code class="ov-mm-node-code">FDataIndexerIndexKey</code>
    <div class="ov-mm-node-text">属性での高速な逆引き</div>
  </div>
</div>
<div class="ov-mm-note">
  <span class="ov-mm-eyebrow">READING ORDER</span>
  <p>新規ユーザーはこの順番で読むのがおすすめ：<br><a href="schema/">Schema</a> → <a href="repository/">Repository</a> → <a href="keys-and-handles/">Keys &amp; Handles</a> → <a href="indexes/">Indexes</a>。</p>
</div>

## 4 つのコンセプト

<div class="grid cards" markdown>

- :material-file-document-outline:{ .lg .middle } &nbsp; **[Schema](schema.md)**

    ---

    Repository とエディタ動作の間のコントラクト。行構造体の型を定義し、表示名ロジックを提供し、Data View に表示するカラムを制御し、Index ビルダー関数を登録します。

- :material-database:{ .lg .middle } &nbsp; **[Repository](repository.md)**

    ---

    行を保持するデータアセット。PrimaryKeyからインスタンス化された行構造体への `TMap`、およびセカンダリIndex用の逆引きテーブルを格納します。

- :material-key-variant:{ .lg .middle } &nbsp; **[キー & Handles](keys-and-handles.md)**

    ---

    行を特定するアドレス型。`FDataIndexerPrimaryKey` は単一行を識別する GUID。`FDataIndexerRowHandle` はRepository参照とキーをペアにし、`FDataIndexerKeysHandle` はIndex クエリ用のキーセットを解決します。

- :material-table-search:{ .lg .middle } &nbsp; **[Indexes](indexes.md)**

    ---

    セカンダリ検索軸。`FDataIndexerIndex`（GUID）はカテゴリ・陣営・レアリティなどの属性をPrimaryKeyのセットにマップします。Schema がビルダー関数を登録します。


</div>
