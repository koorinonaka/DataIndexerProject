# Custom K2 Nodes

DataIndexer provides three custom Blueprint graph nodes that go beyond what a standard `UFUNCTION` can express. These nodes generate typed pins at compile time based on the schema and repository configuration.

---

## Get Row (`UK2Node_DataIndexerGetRow`)

The primary way to retrieve a typed row in Blueprint.

**Purpose:** Given a `FDataIndexerRowHandle`, retrieves the row struct value in a type-safe way, with a success/failure execution path.

**Pin layout:**

| Pin | Direction | Type | Description |
|-----|-----------|------|-------------|
| `Handle` | Input | `FDataIndexerRowHandle` | The row to retrieve |
| `Schema Class` | Input (hidden detail) | `TSubclassOf<UDataIndexerSchema>` | Constrains `Handle` to a specific schema |
| ✓ (then) | Output (exec) | — | Row found |
| ✗ (else) | Output (exec) | — | Row not found or handle invalid |
| `Row` | Output | Wildcard → `FMyRowStruct` | The typed row struct |

**Usage:**

1. Place a **Get Row** node in your Blueprint graph
2. Set the **Schema Class** in the node's details panel
3. Wire a `FDataIndexerRowHandle` variable to the `Handle` pin
4. The `Row` output pin resolves to the concrete row struct type

---

## Select Primary Key (`UK2Node_DataIndexerSelectPrimaryKey`)

A switch-style node that routes execution based on a `FDataIndexerPrimaryKey` value.

**Purpose:** Dispatch execution to different branches based on which specific row a primary key addresses — similar to a `Switch on Enum` node but for DataIndexer rows.

**Pin layout:**

| Pin | Direction | Type | Description |
|-----|-----------|------|-------------|
| `Repository` | Input | `UDataIndexerRepository*` | Constrains which keys are valid selections |
| `Key` | Input | `FDataIndexerPrimaryKey` | The key to switch on |
| `Default` | Output (exec, optional) | — | Fires if key matches none of the configured pins (when `bHasDefaultPin = true`) |
| `[Row Name]` | Output (exec, per key) | — | One output per key configured in `PinKeys` |

**Configuration:**

In the node's details panel, add entries to **Pin Keys** — each entry specifies a `FDataIndexerPrimaryKey` GUID and creates a labeled output execution pin.

---

## Switch Primary Key (`UK2Node_DataIndexerSwitchPrimaryKey`)

Similar to Select Primary Key but follows the `Switch` node convention from the standard UE Blueprint library.

**Difference from Select Primary Key:** Select Primary Key returns a single output value pin (like a ternary selector), while Switch Primary Key provides only execution output pins (like `Switch on Int`). Use Switch when you want to branch execution without selecting a value.

**Pin layout:**

| Pin | Direction | Type | Description |
|-----|-----------|------|-------------|
| (exec) | Input | — | Entry execution |
| `Selection` | Input | `FDataIndexerPrimaryKey` | The key to switch on |
| `Default` | Output (exec) | — | Fires if no match |
| `[Row Name]` | Output (exec, per key) | — | One output per configured key |

---

!!! tip "When to use each node"
    - **Get Row** — you have a handle and want the row data
    - **Select Primary Key** — you have a key and want to pick a value based on which row it is
    - **Switch Primary Key** — you have a key and want to branch execution based on which row it is
