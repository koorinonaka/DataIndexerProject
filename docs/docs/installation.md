# Installation

## Prerequisites

- Unreal Engine 5.3 or later
- A C++ UE5 project (the plugin contains C++ source and must be compiled)

## Steps

### 1. Copy the plugin

Place the `DataIndexer` folder under your project's `Plugins/` directory:

```
YourProject/
└── Plugins/
    └── DataIndexer/        ← plugin root
        ├── Source/
        ├── DataIndexer.uplugin
        └── ...
```

### 2. Regenerate project files

Right-click the `.uproject` file and choose **Generate Visual Studio project files** (Windows) or run `GenerateProjectFiles.sh` (Mac/Linux).

### 3. Build the project

Open the solution in your IDE and build. Both modules compile automatically:

| Module | Type | Description |
|--------|------|-------------|
| `DataIndexer` | Runtime | Asset types, primary keys, schema interface, Blueprint library |
| `DataIndexerEd` | Editor (UncookedOnly) | Custom asset editor, property customizations, data view |

### 4. Enable the plugin

Open the **Plugins** panel in Unreal Editor (**Edit → Plugins**), search for **DataIndexer**, and enable it. Restart the editor when prompted.

## Verification

After restarting, confirm both modules are loaded:

1. Open **Output Log**
2. Search for `LogDataIndexer` — you should see initialization messages from both modules
3. Right-click in the Content Browser to verify the DataIndexer asset types appear in the **Miscellaneous** and **Blueprint Class** menus

## Adding as a Module Dependency

To use the runtime API from your game module, add `DataIndexer` to your `.Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "DataIndexer",
    // ...
});
```

!!! warning "Editor-only module"
    Never reference `DataIndexerEd` from a Runtime or game module. It is declared `UncookedOnly` and will not be available in packaged builds.
