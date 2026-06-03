---
hide:
  - navigation
---

# コアコンセプト

DataIndexer は 4 つの相互に関連するコンセプトで構成されています。これらの関係を理解することで、他のすべてがつながります。

<div class="ov-mm-diagram">
  <div class="ov-mm-node">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 01</span>
      <span class="ov-mm-node-icon icon-schema"></span>
    </div>
    <span class="ov-mm-node-title">Schema</span>
    <code class="ov-mm-node-code">UDataIndexerSchema</code>
    <span class="ov-mm-node-text">Row 型・表示・Index を定義する設計図</span>
  </div>
  <span class="ov-mm-arrow"><span>defines</span></span>
  <div class="ov-mm-node">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 02</span>
      <span class="ov-mm-node-icon icon-repository"></span>
    </div>
    <span class="ov-mm-node-title">Repository</span>
    <code class="ov-mm-node-code">UDataIndexerRepository</code>
    <span class="ov-mm-node-text">Row を格納する型付きアセット</span>
  </div>
  <span class="ov-mm-arrow"><span>contains</span></span>
  <div class="ov-mm-node">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 03</span>
      <span class="ov-mm-node-icon icon-keys"></span>
    </div>
    <span class="ov-mm-node-title">Keys &amp; Handles</span>
    <code class="ov-mm-node-code">FDataIndexerPrimaryKey</code>
    <span class="ov-mm-node-text">GUID で Row を一意に識別</span>
  </div>
  <span class="ov-mm-arrow"><span>resolves via</span></span>
  <div class="ov-mm-node">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 04</span>
      <span class="ov-mm-node-icon icon-indexes"></span>
    </div>
    <span class="ov-mm-node-title">Indexes</span>
    <code class="ov-mm-node-code">FDataIndexerIndexKey</code>
    <span class="ov-mm-node-text">属性での高速な逆引き</span>
  </div>
</div>

## 4 つのコンセプト

<div class="ov-chapters">

<a class="ov-chapter" href="schema/">
  <div class="ov-chapter-left">
    <div class="ov-chapter-num">01</div>
    <div class="ov-chapter-num-bar"></div>
  </div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-schema"></span>
      <h3>Schema</h3>
      <code>UDataIndexerSchema</code>
    </div>
    <p>Repository とエディタ動作の間のコントラクト。行構造体の型を定義し、表示名ロジックを提供し、Data View に表示するカラムを制御し、Index ビルダー関数を登録します。</p>
    <ul>
      <li>行構造体の型を定義</li>
      <li>表示名・Data View カラムを制御</li>
      <li>Index ビルダー関数を登録</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">
    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="M9 6l6 6-6 6"/></svg>
  </div>
</a>

<a class="ov-chapter" href="repository/">
  <div class="ov-chapter-left">
    <div class="ov-chapter-num">02</div>
    <div class="ov-chapter-num-bar"></div>
  </div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-repository"></span>
      <h3>Repository</h3>
      <code>UDataIndexerRepository</code>
    </div>
    <p>行を保持するデータアセット。PrimaryKey からインスタンス化された行構造体への <code>TMap</code>、およびセカンダリ Index 用の逆引きテーブルを格納します。Repository は親 Repository を参照して、重複なしに行を継承できます。</p>
    <ul>
      <li>PrimaryKey → Row の TMap</li>
      <li>セカンダリ Index 用の逆引きテーブル</li>
      <li>親 Repository を継承可能</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">
    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="M9 6l6 6-6 6"/></svg>
  </div>
</a>

<a class="ov-chapter" href="keys-and-handles/">
  <div class="ov-chapter-left">
    <div class="ov-chapter-num">03</div>
    <div class="ov-chapter-num-bar"></div>
  </div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-keys"></span>
      <h3>Keys &amp; Handles</h3>
      <code>FDataIndexerPrimaryKey</code>
    </div>
    <p>行を特定するアドレス型。<code>FDataIndexerPrimaryKey</code> は単一行を識別する GUID。<code>FDataIndexerRowHandle</code> は Repository 参照とキーをペアにし、<code>FDataIndexerKeysHandle</code> は Index クエリ用のキーセットを解決します。</p>
    <ul>
      <li>PrimaryKey: GUID で 1 行を識別</li>
      <li>RowHandle: Repository + Key のペア</li>
      <li>KeysHandle: Index クエリの結果セット</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">
    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="M9 6l6 6-6 6"/></svg>
  </div>
</a>

<a class="ov-chapter" href="indexes/">
  <div class="ov-chapter-left">
    <div class="ov-chapter-num">04</div>
    <div class="ov-chapter-num-bar"></div>
  </div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-indexes"></span>
      <h3>Indexes</h3>
      <code>FDataIndexerIndexKey</code>
    </div>
    <p>セカンダリ検索軸。<code>FDataIndexerIndex</code>（GUID）はカテゴリ・陣営・レアリティなどの属性を PrimaryKey のセットにマップします。Schema がビルダー関数を登録します。</p>
    <ul>
      <li>属性 → PrimaryKey セットのマップ</li>
      <li>Schema が builder を登録</li>
      <li>保存時に Repository が再構築</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">
    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="M9 6l6 6-6 6"/></svg>
  </div>
</a>

</div>
