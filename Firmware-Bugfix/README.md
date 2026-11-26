# Bugfix firmware output

Building the `BIGTREE_GD_TFT35_E3_V3_0` PlatformIO environment copies the resulting firmware binary into this directory.

The binary keeps the official BIGTREETECH filename generated from the target hardware and software version. Do not rename it: the TFT bootloader expects the model-specific filename.

To flash:

1. Copy the `.bin` file from this directory to the root of the TFT SD card.
2. Keep the filename and capitalization unchanged.
3. Insert the card into the TFT and reboot or press Reset to start the update.

The generated `.bin` is ignored by Git and will be recreated by the next successful build.
