---
title: Installation
---

# Installation

Add DataIndexer to your project and get the editor running.


## Prerequisites

<div class="grid cards" markdown>

- :material-unreal:{ .lg .middle } &nbsp; **Unreal Engine 5.3 or later**

    ---

    Tested on UE 5.3 / 5.4 / 5.5. Older versions are not supported.

</div>

---

## Enable the plugin

Launch Unreal Editor → **Edit → Plugins**, search for **DataIndexer**, and enable it. Restart when prompted.

??? note "C++ project setup"

    If you're using DataIndexer from source in a C++ project, complete these steps before enabling the plugin.

    #### 1. Copy the plugin

    Place the `DataIndexer` folder directly under your project's `Plugins/` directory:

    ``` { .text .no-copy }
    YourProject/
    ├── YourProject.uproject
    ├── Source/
    └── Plugins/
        └── DataIndexer/         ← plugin root
            ├── Source/
            ├── DataIndexer.uplugin
            └── ...
    ```

    #### 2. Regenerate project files

    === ":fontawesome-brands-windows: Windows"

        Right-click the `.uproject` file → **Generate Visual Studio project files**.

    === ":fontawesome-brands-apple: macOS"

        In a terminal, navigate to the project root and run `GenerateProjectFiles.command`.

    === ":fontawesome-brands-linux: Linux"

        ```bash
        ./GenerateProjectFiles.sh
        ```

    #### 3. Build the project

    Open the solution in your IDE and build. Both modules compile automatically:

    | Module | Type | Description |
    |--------|------|-------------|
    | `DataIndexer` | Runtime | Asset types, primary keys, schema interface, Blueprint library |
    | `DataIndexerEd` | Editor (UncookedOnly) | Custom asset editor, property customizations, data view |

    #### Adding a module dependency

    To use the runtime API from your game module, add `DataIndexer` to your `.Build.cs`:

    ```csharp title="YourGame.Build.cs" hl_lines="4"
    PublicDependencyModuleNames.AddRange(new string[]
    {
        "Core", "CoreUObject", "Engine",
        "DataIndexer",
    });
    ```

    !!! warning "Editor-only module"
        Never reference `DataIndexerEd` from a Runtime or game module. It is declared `UncookedOnly` and will not be available in packaged builds.

---

## Troubleshooting

??? question "\"Plugin could not be loaded\" on editor launch"

    Project file regeneration (step 2) or the build (step 3) may be incomplete. Try a clean rebuild.

??? question "Asset types not showing in the Content Browser"

    The plugin may not be enabled. Check **Edit → Plugins** and confirm DataIndexer is ON, then restart the editor.

---

## Next steps

<div class="grid cards" markdown>

- :material-rocket-launch:{ .lg .middle } &nbsp; **[Quick Start](quick-start.md)**

    ---

    Create your first Schema and Repository, author rows, and query them from Blueprint and C++ in 10 minutes.

- :material-book-open-variant:{ .lg .middle } &nbsp; **[Core Concepts](concepts/index.md)**

    ---

    Understand how Repository, Schema, Keys & Handles, and Indexes fit together.

</div>
