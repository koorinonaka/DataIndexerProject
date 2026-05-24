---
hide:
  - navigation
---

# Core Concepts

DataIndexer is built around four interconnected concepts. Understanding how they relate makes everything else click.

<div class="ov-mm-diagram" aria-hidden="true">
  <div class="ov-mm-node tone-purple">
    <div class="ov-mm-node-num">01</div>
    <div class="ov-mm-node-title">Schema</div>
    <code class="ov-mm-node-code">UDataIndexerSchema</code>
    <div class="ov-mm-node-text">The schema that defines row types, display, and indexes.</div>
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
    <div class="ov-mm-node-text">The typed asset that stores rows.</div>
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
    <div class="ov-mm-node-text">GUIDs that uniquely identify each row.</div>
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
    <div class="ov-mm-node-text">Fast reverse lookup by any attribute.</div>
  </div>
</div>
<div class="ov-mm-note">
  <span class="ov-mm-eyebrow">READING ORDER</span>
  <p>New to DataIndexer? Read in order:<br><a href="schema/">Schema</a> → <a href="repository/">Repository</a> → <a href="keys-and-handles/">Keys &amp; Handles</a> → <a href="indexes/">Indexes</a>.</p>
</div>

## The four concepts

<div class="grid cards" markdown>

- :material-file-document-outline:{ .lg .middle } &nbsp; **[Schema](schema.md)**

    ---

    The contract between a repository and its editor behavior. Defines the row struct type, provides display name logic, controls Data View columns, and registers index builder functions.

- :material-database:{ .lg .middle } &nbsp; **[Repository](repository.md)**

    ---

    The data asset that holds rows. Stores a `TMap` of primary keys to instanced row structs, plus reverse lookup tables for secondary indexes. Repositories can reference parent repositories to inherit rows without duplication.

- :material-key-variant:{ .lg .middle } &nbsp; **[Keys & Handles](keys-and-handles.md)**

    ---

    Address types for locating rows. `FDataIndexerPrimaryKey` is a stable GUID. `FDataIndexerRowHandle` pairs a repository with a key. `FDataIndexerKeysHandle` resolves a matching key set at query time via an index.

- :material-table-search:{ .lg .middle } &nbsp; **[Indexes](indexes.md)**

    ---

    Secondary lookup dimensions. An index (`FDataIndexerIndex`, a GUID) maps a domain attribute — category, faction, rarity — to a set of primary keys. The schema registers the builder function for each row.


</div>
