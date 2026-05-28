---
hide:
  - navigation
---

# Core Concepts

DataIndexer is built around four interconnected concepts. Understanding how they relate makes everything else click.

<div class="ov-mm-diagram">
  <a class="ov-mm-node" href="schema/">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 01</span>
      <span class="ov-mm-node-icon icon-schema"></span>
    </div>
    <span class="ov-mm-node-title">Schema</span>
    <code class="ov-mm-node-code">UDataIndexerSchema</code>
    <span class="ov-mm-node-text">The design blueprint that defines row types, display, and indexes.</span>
  </a>
  <span class="ov-mm-arrow"><span>defines</span></span>
  <a class="ov-mm-node" href="repository/">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 02</span>
      <span class="ov-mm-node-icon icon-repository"></span>
    </div>
    <span class="ov-mm-node-title">Repository</span>
    <code class="ov-mm-node-code">UDataIndexerRepository</code>
    <span class="ov-mm-node-text">The typed asset that stores rows.</span>
  </a>
  <span class="ov-mm-arrow"><span>contains</span></span>
  <a class="ov-mm-node" href="keys-and-handles/">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 03</span>
      <span class="ov-mm-node-icon icon-keys"></span>
    </div>
    <span class="ov-mm-node-title">Keys &amp; Handles</span>
    <code class="ov-mm-node-code">FDataIndexerPrimaryKey</code>
    <span class="ov-mm-node-text">GUIDs that uniquely identify each row.</span>
  </a>
  <span class="ov-mm-arrow"><span>resolves via</span></span>
  <a class="ov-mm-node" href="indexes/">
    <div class="ov-mm-node-header">
      <span class="ov-mm-node-step">STEP 04</span>
      <span class="ov-mm-node-icon icon-indexes"></span>
    </div>
    <span class="ov-mm-node-title">Indexes</span>
    <code class="ov-mm-node-code">FDataIndexerIndexKey</code>
    <span class="ov-mm-node-text">Fast reverse lookup by any attribute.</span>
  </a>
</div>

## The Four Concepts

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
    <p>The contract between a repository and its editor behavior. Defines the row struct type, provides display name logic, controls Data View columns, and registers index builder functions.</p>
    <ul>
      <li>Define row struct type</li>
      <li>Control display name and Data View columns</li>
      <li>Register index builder functions</li>
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
    <p>The data asset that holds rows. Stores a <code>TMap</code> of primary keys to instanced row structs, plus reverse lookup tables for secondary indexes. Repositories can reference parent repositories to inherit rows without duplication.</p>
    <ul>
      <li>PrimaryKey → Row TMap</li>
      <li>Secondary index lookup tables</li>
      <li>Parent repository inheritance</li>
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
    <p>Address types for locating rows. <code>FDataIndexerPrimaryKey</code> is a stable GUID. <code>FDataIndexerRowHandle</code> pairs a repository with a key. <code>FDataIndexerKeysHandle</code> resolves a matching key set at query time via an index.</p>
    <ul>
      <li>PrimaryKey: stable GUID per row</li>
      <li>RowHandle: Repository + Key pair</li>
      <li>KeysHandle: index query result set</li>
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
    <p>Secondary lookup dimensions. An index (<code>FDataIndexerIndex</code>, a GUID) maps a domain attribute — category, faction, rarity — to a set of primary keys. The schema registers the builder function for each row.</p>
    <ul>
      <li>Attribute → PrimaryKey set map</li>
      <li>Schema registers builder functions</li>
      <li>Repository rebuilds on save</li>
    </ul>
  </div>
  <div class="ov-chapter-arrow">
    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"><path d="M9 6l6 6-6 6"/></svg>
  </div>
</a>

</div>
