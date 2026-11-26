# Bugfix firmware output

Building the `BIGTREE_GD_TFT35_E3_V3_0` PlatformIO environment copies the resulting firmware binary into this directory.

The output filename is:

```
BIGTREE_GD_TFT35_V3.0_E3.28.x.bin
```

This is the currently installed 28.x firmware line, so the filename matches the updater expected by the TFT. The binary contents are built from the `bugfix` branch.

Do not rename the file before copying it to the TFT SD card root.

The generated `.bin` is ignored by Git and will be recreated by the next successful build.
