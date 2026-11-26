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
Firmware-Bugfix/BIGTREE_GD_TFT35_V3.0_E3.28.x.bin
```

The dedicated folder distinguishes this build from the official release files. The **28.x filename matches the currently installed firmware line** and must be copied to the SD card root without changing its name.

## Diagnostic build: interrupt-driven TX

To isolate the TX-DMA path, a second environment is available:

```powershell
pio run -e BIGTREE_GD_TFT35_E3_V3_0_TXIRQ
```

This build keeps the same board, firmware version, UART configuration and RX-DMA implementation. It changes only UART TX from DMA to the USART transmit-empty interrupt. Its update file is copied to the same `Firmware-Bugfix` filename above, so build and copy only one environment at a time. Use this image as a diagnostic comparison, not as a presumed permanent fix.

## Diagnostic build: interrupt-driven RX and TX

To test the receive path without DMA, build the combined interrupt-driven variant:

```powershell
pio run -e BIGTREE_GD_TFT35_E3_V3_0_TXRXIRQ
```

This variant disables both RX-DMA and TX-DMA. RX bytes are collected by the USART receive-buffer-not-empty interrupt into the same circular software buffer used by the normal line parser. TX uses the interrupt-driven path already available in the `_TXIRQ` build. The resulting firmware overwrites the same `Firmware-Bugfix/BIGTREE_GD_TFT35_V3.0_E3.28.x.bin` output, so build and copy only one environment at a time.

This is a diagnostic build, not a presumed permanent fix. Test with the printer idle and do not start a print until communication is stable.

## First test after flashing

Settings stored in the TFT's persistent memory are not overwritten by a firmware update. For the current diagnostic, leave the existing **TX delay at 10 ms**, keep the baud rate and checksum setting unchanged, and do not change other communication settings.

## Other targets

To build another supported board, specify its environment explicitly with `pio run -e <environment>`. The list of environments is defined in `platformio.ini`.

**Note:** This documents the project setup and target selection. A successful local firmware build still needs to be verified in the user's PlatformIO installation.
