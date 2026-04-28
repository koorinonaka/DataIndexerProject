# Core Concepts

DataIndexer is built around four interconnected concepts. Understanding how they relate makes everything else click.

```
Schema ──defines structure of──► Repository ──stores rows addressed by──► PrimaryKey
  │                                   │
  │ registers index builders           │ builds ReverseLookup table
  ▼                                   ▼
Index ──maps rows to──► IndexKey ──enables secondary lookup
```

## The four concepts

[**Repository**](repository.md)
: The data asset that holds rows. A repository stores a `TMap` of primary keys to instanced row structs, plus reverse lookup tables for secondary indexes. Repositories can reference parent repositories to inherit rows without duplication.

[**Schema**](schema.md)
: The contract between a repository and its editor behavior. A schema defines the row struct type, provides display name logic, controls which columns appear in the Data View, and registers index key builder functions.

[**Keys & Handles**](keys-and-handles.md)
: The address types used to locate rows. `FDataIndexerPrimaryKey` is a GUID that stably identifies a single row. `FDataIndexerRowHandle` pairs a repository reference with a primary key for use in Blueprint variables and UPROPERTY fields. `FDataIndexerRowsHandle` addresses a set of rows via a secondary index.

[**Indexes**](indexes.md)
: Secondary lookup dimensions. An index (identified by `FDataIndexerIndex`, a GUID) maps a domain attribute — category, faction, rarity — to a set of primary keys. The schema registers the function that computes the `FDataIndexerIndexKey` for each row.
