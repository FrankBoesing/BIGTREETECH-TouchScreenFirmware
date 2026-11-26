Import("env")

import os
import shutil


def copy_bugfix_firmware(source, target, env):
    repo_path = os.path.realpath("")
    build_dir = env.subst('$BUILD_DIR')
    build_filename = env['PROGNAME'] + ".bin"
    # The currently installed TFT firmware (27.x) searches for this exact name.
    # The binary itself is built from the 28.x sources and reports 28.x after flashing.
    update_filename = "BIGTREE_GD_TFT35_V3.0_E3.27.x.bin"

    source_path = os.path.join(repo_path, build_dir, build_filename)
    target_dir = os.path.join(repo_path, "Firmware-Bugfix")
    target_path = os.path.join(target_dir, update_filename)

    if not os.path.isfile(source_path):
        raise RuntimeError("Expected firmware binary was not found: " + source_path)

    os.makedirs(target_dir, exist_ok=True)
    shutil.copy2(source_path, target_path)

    print("Built firmware: " + build_filename)
    print("Update file copied to: " + target_path)
    print("The 27.x filename is for the currently installed updater; firmware content remains 28.x.")


env.AddPostAction("buildprog", copy_bugfix_firmware)
