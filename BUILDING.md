# Building with VS Code and PlatformIO

This repository is configured for the **BIGTREE GD TFT35 E3 V3.0** target.

## Requirements

- Visual Studio Code
- The **PlatformIO IDE** extension
- Internet access for the first build so PlatformIO can install the GD32 platform and framework packages declared in `platformio.ini`

## Open the project

1. Check out the `bugfix` branch.
2. In VS Code, open the repository root folder — the folder containing `platformio.ini`.
3. Allow PlatformIO to initialize the project and install its required packages.

Do not open the `TFT` subfolder as the workspace root.

## Build

The default environment is already set to:

```ini
default_envs = BIGTREE_GD_TFT35_E3_V3_0
```

In VS Code, run **PlatformIO: Build** from the PlatformIO toolbar or Command Palette.

Alternatively, run this command in the VS Code terminal:

```powershell
pio run -e BIGTREE_GD_TFT35_E3_V3_0
```

Build artifacts are placed below:

```
.pio/build/BIGTREE_GD_TFT35_E3_V3_0/
```

## Other targets

To build another supported board, specify its environment explicitly with `pio run -e <environment>`. The list of environments is defined in `platformio.ini`.

**Note:** This documents the project setup and target selection. A successful local firmware build still needs to be verified in the user's PlatformIO installation.
