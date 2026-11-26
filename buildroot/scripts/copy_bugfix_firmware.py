Import("env")

import os
import shutil


def copy_bugfix_firmware(source, target, env):
    repo_path = os.path.realpath("")
    build_dir = env.subst('$BUILD_DIR')
    filename = env['PROGNAME'] + ".bin"

    source_path = os.path.join(repo_path, build_dir, filename)
    target_dir = os.path.join(repo_path, "Firmware-Bugfix")
    target_path = os.path.join(target_dir, filename)

    if not os.path.isfile(source_path):
        raise RuntimeError("Expected firmware binary was not found: " + source_path)

    os.makedirs(target_dir, exist_ok=True)
    shutil.copy2(source_path, target_path)

    print("Bugfix firmware copied to: " + target_path)
    print("Keep the original filename unchanged for the TFT bootloader.")


env.AddPostAction("buildprog", copy_bugfix_firmware)
