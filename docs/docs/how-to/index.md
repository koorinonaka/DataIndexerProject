---
title: How-to Guides
---

# How-to Guides

Task-oriented guides for common DataIndexer workflows. Find the task that matches what you want to do.

## Reverse Lookup

| I want to… | Guide |
|------------|-------|
| Move my DataTable rows into DataIndexer | [Migrate from DataTable](migrate-from-datatable.md) |
| Export rows to JSON and re-import after a struct change | [Migrate from DataTable → Transform the JSON](migrate-from-datatable.md#transform-the-json) |
| Restrict a repository UPROPERTY picker to a specific schema | [Repository Metadata → Schema Filtering](repository-metadata.md#schema-filtering) |
| Lock certain fields so child repositories cannot change them | [Repository Metadata → NotOverridable](repository-metadata.md#notoverridable) |
| Build a shared base table with per-zone or per-shop overlays | [Parent Repository Hierarchy](parent-hierarchy.md) |
| Override individual rows from a parent without duplicating data | [Parent Repository Hierarchy → Override Mode](parent-hierarchy.md#override-mode) |
| Query both parent and child rows from a single repository | [Parent Repository Hierarchy → Runtime Querying](parent-hierarchy.md#runtime-querying) |
| Prevent a child repository from seeing a subset of parent rows | [Parent Repository Hierarchy → Selective Override](parent-hierarchy.md#selective-override) |

## Related reading

- [Core Concepts](../concepts/index.md) — Repository, Schema, Keys, and Indexes explained
- [Editor Guide](../editor-guide/index.md) — Data View, JSON Support, Driven Collection
- [API Reference](../api-reference/index.md) — Blueprint nodes and C++ interfaces
