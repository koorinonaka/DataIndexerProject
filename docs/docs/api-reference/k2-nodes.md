# Custom K2 Nodes

DataIndexer provides four custom Blueprint graph nodes that go beyond what a standard `UFUNCTION` can express. These nodes generate typed pins at compile time based on the schema and repository configuration.

---

## Get Row (`UK2Node_DataIndexerGetRow`)

The primary way to retrieve a typed row in Blueprint.

**Purpose:** Given a `FDataIndexerRowHandle`, retrieves the row struct value in a type-safe way, with a success/failure execution path.

**Pin layout:**

| Pin | Direction | Type | Description |
|-----|-----------|------|-------------|
| (exec) | Input | — | Entry execution |
| `Handle` | Input | `FDataIndexerRowHandle` | The row to retrieve. Auto-splits into `Repository` and `PrimaryKey` sub-pins on placement |
| ✓ (Then) | Output (exec) | — | Row found |
| ✗ (Missing) | Output (exec) | — | Row not found or handle invalid |
| `Out Row` | Output | Wildcard → `FMyRowStruct` | The typed row struct |

![Get Row node](../assets/images/k2node-get-row.png)

**Usage:**

1. Place a **Get Row** node in your Blueprint graph
2. Set the `Repository` sub-pin on the `Handle` pin to your Repository asset
3. Wire or set the `PrimaryKey` sub-pin
4. The `Out Row` output pin resolves to the Repository's row struct type

---

## Get Keys by Index (`UK2Node_DataIndexerGetKeysByIndex`)

Retrieves the list of primary keys matching a query, looked up via an index key.

**Purpose:** Given a `FDataIndexerKeysHandle` and a query struct, returns the `FDataIndexerPrimaryKey` array of matching rows, with a success/failure execution path.

**Pin layout:**

| Pin | Direction | Type | Description |
|-----|-----------|------|-------------|
| (exec) | Input | — | Entry execution |
| `Handle` | Input | `FDataIndexerKeysHandle` | The index lookup handle. Auto-splits into `Repository` and `IndexKey` sub-pins on placement |
| `Query` | Input | Wildcard → row struct | Query struct used to filter results |
| ✓ (Then) | Output (exec) | — | One or more keys found |
| ✗ (Missing) | Output (exec) | — | No match or handle invalid |
| `Out Keys` | Output | `TArray<FDataIndexerPrimaryKey>` | The matching primary keys |

![Get Keys by Index node](../assets/images/k2node-get-keys-by-index.png)

---

## Select by Primary Key (`UK2Node_DataIndexerSelectPrimaryKey`)

A pure node that selects a **value** based on a `FDataIndexerPrimaryKey`.

**Purpose:** Returns a different value depending on which row a primary key addresses — equivalent to UE's `Select` node but keyed by `FDataIndexerPrimaryKey`. Has no exec pins (pure node).

**Node properties (set in Details panel):**

| Property | Type | Description |
|----------|------|-------------|
| `Repository` | `UDataIndexerRepository*` | Constrains which keys are valid selections |
| `Pin Keys` | `TArray<FDataIndexerPrimaryKey>` | Keys to expose as case input pins |
| `Has Default Pin` | `bool` | Whether to generate a `Fallback` input pin (default: true) |

**Pin layout:**

| Pin | Direction | Type | Description |
|-----|-----------|------|-------------|
| `PrimaryKey` | Input | `FDataIndexerPrimaryKey` | The key to select on |
| `[Row Name]` | Input (per key) | Wildcard → any type | Value input for each key configured in `Pin Keys` |
| `Fallback` | Input (optional) | Wildcard → any type | Value returned when no key matches (when `Has Default Pin = true`) |
| (ReturnValue) | Output | Wildcard → any type | The selected value |

![Select by Primary Key node](../assets/images/k2node-select-by-primary-key.png)

---

## Switch on Primary Key (`UK2Node_DataIndexerSwitchPrimaryKey`)

Routes **execution** based on a `FDataIndexerPrimaryKey` value, following the standard UE `Switch` node convention.

**Purpose:** Dispatch execution to different branches based on which specific row a primary key addresses — like `Switch on Int` but for DataIndexer rows.

**Node properties (set in Details panel):**

| Property | Type | Description |
|----------|------|-------------|
| `Repository` | `UDataIndexerRepository*` | Constrains which keys are valid selections |
| `Pin Keys` | `TArray<FDataIndexerPrimaryKey>` | Keys to expose as output execution pins |

**Pin layout:**

| Pin | Direction | Type | Description |
|-----|-----------|------|-------------|
| (exec) | Input | — | Entry execution |
| `Selection` | Input | `FDataIndexerPrimaryKey` | The key to switch on |
| `Default` | Output (exec) | — | Fires if key matches none of the configured pins |
| `[Row Name]` | Output (exec, per key) | — | One output per key configured in `Pin Keys` |

![Switch on Primary Key node](../assets/images/k2node-switch-on-primary-key.png)

---

!!! tip "When to use each node"
    - **Get Row** — you have a handle and want the row data
    - **Get Keys by Index** — you have an index key and want matching primary keys
    - **Select by Primary Key** — you have a key and want to pick a value based on which row it is
    - **Switch on Primary Key** — you have a key and want to branch execution based on which row it is
