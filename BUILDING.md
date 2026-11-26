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

After a successful build of the GD TFT35 E3 target, the firmware is also copied to:

```
Firmware-Bugfix/BIGTREE_GD_TFT35_V3.0_E3.27.x.bin
```

The dedicated folder distinguishes this build from the official release files. The **27.x filename is intentional**: the currently installed TFT updater searches for the exact versioned name it was built with. The binary contents are built from the 28.x branch and identify themselves as 28.x after flashing. Copy the file to the SD card root without changing its name. After the first successful update, subsequent update files must use the 28.x name.

## First test after flashing

The firmware default and the supplied `config.ini` use `tx_delay:4`. Existing settings stored in the TFT's persistent memory are not overwritten by a firmware update. For this branch, set the value explicitly on the display under **Settings → Feature → TX delay → 4 ms** before testing. This value follows BTT's 3–4 ms recommendation for `ADVANCED_OK` enabled and also provides margin when it is disabled.

## Other targets

To build another supported board, specify its environment explicitly with `pio run -e <environment>`. The list of environments is defined in `platformio.ini`.

**Note:** This documents the project setup and target selection. A successful local firmware build still needs to be verified in the user's PlatformIO installation.
