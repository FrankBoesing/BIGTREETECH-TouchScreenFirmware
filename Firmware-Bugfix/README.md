# Bugfix firmware output

Building the `BIGTREE_GD_TFT35_E3_V3_0` PlatformIO environment copies the resulting firmware binary into this directory.

## First update from the installed 27.x firmware

The file is intentionally named:

```
BIGTREE_GD_TFT35_V3.0_E3.27.x.bin
```

This matches the filename expected by the currently installed TFT firmware, which the user has confirmed accepts the `27.x` name. The file contents are built from this branch's `28.x` sources; after flashing, the new firmware identifies itself as version `28.x`.

Do not rename the file before copying it to the TFT SD card root.

## Later updates

After the 28.x bugfix firmware is installed, its updater expects the corresponding `28.x` filename for subsequent firmware updates. The post-build copy script must then be switched to `BIGTREE_GD_TFT35_V3.0_E3.28.x.bin`.

The generated `.bin` is ignored by Git and will be recreated by the next successful build.
