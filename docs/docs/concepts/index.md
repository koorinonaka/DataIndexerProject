---
hide:
  - navigation
---

# Core Concepts

DataIndexer is built around four interconnected concepts. Understanding how they relate makes everything else click.

<div class="ov-mm-diagram">
  <a class="ov-mm-node" href="schema/">
    <span class="ov-mm-node-step">STEP 01</span>
    <span class="ov-mm-node-icon icon-schema"></span>
    <span class="ov-mm-node-title">Schema</span>
    <code class="ov-mm-node-code">UDataIndexerSchema</code>
    <span class="ov-mm-node-text">The design blueprint that defines row types, display, and indexes.</span>
  </a>
  <span class="ov-mm-arrow"><span>defines</span></span>
  <a class="ov-mm-node" href="repository/">
    <span class="ov-mm-node-step">STEP 02</span>
    <span class="ov-mm-node-icon icon-repository"></span>
    <span class="ov-mm-node-title">Repository</span>
    <code class="ov-mm-node-code">UDataIndexerRepository</code>
    <span class="ov-mm-node-text">The typed asset that stores rows.</span>
  </a>
  <span class="ov-mm-arrow"><span>contains</span></span>
  <a class="ov-mm-node" href="keys-and-handles/">
    <span class="ov-mm-node-step">STEP 03</span>
    <span class="ov-mm-node-icon icon-keys"></span>
    <span class="ov-mm-node-title">Keys &amp; Handles</span>
    <code class="ov-mm-node-code">FDataIndexerPrimaryKey</code>
    <span class="ov-mm-node-text">GUIDs that uniquely identify each row.</span>
  </a>
  <span class="ov-mm-arrow"><span>resolves via</span></span>
  <a class="ov-mm-node" href="indexes/">
    <span class="ov-mm-node-step">STEP 04</span>
    <span class="ov-mm-node-icon icon-indexes"></span>
    <span class="ov-mm-node-title">Indexes</span>
    <code class="ov-mm-node-code">FDataIndexerIndexKey</code>
    <span class="ov-mm-node-text">Fast reverse lookup by any attribute.</span>
  </a>
</div>

<p class="ov-mm-reading">
  <span class="ov-mm-reading-label">READING ORDER</span>
  <span>·</span>
  New to DataIndexer? Read in order:
  <a href="schema/">Schema</a> →
  <a href="repository/">Repository</a> →
  <a href="keys-and-handles/">Keys &amp; Handles</a> →
  <a href="indexes/">Indexes</a>
</p>

## The four concepts

<div class="ov-chapters">

<a class="ov-chapter" href="schema/">
  <div class="ov-chapter-num">01</div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-schema"></span>
      <h3>Schema</h3>
      <code>UDataIndexerSchema</code>
    </div>
    <p>The contract between a repository and its editor behavior. Defines the row struct type, provides display name logic, controls Data View columns, and registers index builder functions.</p>
    <ul>
      <li>Define row struct type</li>
      <li>Control display name and Data View columns</li>
      <li>Register index builder functions</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">›</div>
</a>

<a class="ov-chapter" href="repository/">
  <div class="ov-chapter-num">02</div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-repository"></span>
      <h3>Repository</h3>
      <code>UDataIndexerRepository</code>
    </div>
    <p>The data asset that holds rows. Stores a <code>TMap</code> of primary keys to instanced row structs, plus reverse lookup tables for secondary indexes. Repositories can reference parent repositories to inherit rows without duplication.</p>
    <ul>
      <li>Store typed row data as a TMap</li>
      <li>Maintain secondary index lookup tables</li>
      <li>Support parent repository inheritance</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">›</div>
</a>

<a class="ov-chapter" href="keys-and-handles/">
  <div class="ov-chapter-num">03</div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-keys"></span>
      <h3>Keys &amp; Handles</h3>
      <code>FDataIndexerPrimaryKey</code>
    </div>
    <p>Address types for locating rows. <code>FDataIndexerPrimaryKey</code> is a stable GUID. <code>FDataIndexerRowHandle</code> pairs a repository with a key. <code>FDataIndexerKeysHandle</code> resolves a matching key set at query time via an index.</p>
    <ul>
      <li>Stable GUID-based row identity</li>
      <li>Repository-scoped row handles</li>
      <li>Index-driven key set resolution</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">›</div>
</a>

<a class="ov-chapter" href="indexes/">
  <div class="ov-chapter-num">04</div>
  <div class="ov-chapter-body">
    <div class="ov-chapter-head">
      <span class="ov-chapter-icon icon-indexes"></span>
      <h3>Indexes</h3>
      <code>FDataIndexerIndexKey</code>
    </div>
    <p>Secondary lookup dimensions. An index (<code>FDataIndexerIndex</code>, a GUID) maps a domain attribute — category, faction, rarity — to a set of primary keys. The schema registers the builder function for each row.</p>
    <ul>
      <li>Map attributes to primary key sets</li>
      <li>Enable fast reverse lookup</li>
      <li>Builder functions registered in Schema</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">›</div>
</a>

</div>
