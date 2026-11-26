Import("env")

import os
import shutil


def copy_bugfix_firmware(source, target, env):
    repo_path = os.path.realpath("")
    build_dir = env.subst('$BUILD_DIR')
    build_filename = env['PROGNAME'] + ".bin"
    # Keep the versioned filename aligned with the currently installed 28.x firmware.
    update_filename = "BIGTREE_GD_TFT35_V3.0_E3.28.x.bin"

    source_path = os.path.join(repo_path, build_dir, build_filename)
    target_dir = os.path.join(repo_path, "Firmware-Bugfix")
    target_path = os.path.join(target_dir, update_filename)

    if not os.path.isfile(source_path):
        raise RuntimeError("Expected firmware binary was not found: " + source_path)

    os.makedirs(target_dir, exist_ok=True)
    shutil.copy2(source_path, target_path)

    print("Built firmware: " + build_filename)
    print("Update file copied to: " + target_path)
    print("The output filename matches the currently installed 28.x updater.")


env.AddPostAction("buildprog", copy_bugfix_firmware)
