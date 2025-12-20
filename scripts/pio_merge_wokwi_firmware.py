Import("env")  # type: ignore[name-defined]

import os
import subprocess


def _merge_for_wokwi(source, target, env, **kwargs):
    build_dir = env.subst("$BUILD_DIR")

    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin")
    app_bin = os.path.join(build_dir, "firmware.bin")
    out_bin = os.path.join(build_dir, "wokwi-firmware.bin")

    for path in (bootloader, partitions, app_bin):
        if not os.path.isfile(path):
            print(f"[wokwi] Missing {path}; skipping merge")
            return

    board_config = env.BoardConfig()

    flash_mode = board_config.get("build.flash_mode", "dio")
    flash_freq = board_config.get("build.f_flash", "40000000L")
    flash_size = board_config.get("upload.flash_size", "4MB")

    # f_flash is typically like "40000000L" (Arduino-ESP32). esptool expects "40m".
    flash_freq_map = {
        "80000000L": "80m",
        "40000000L": "40m",
        "20000000L": "20m",
        "16000000L": "16m",
    }
    flash_freq = flash_freq_map.get(str(flash_freq), "40m")

    esptool_dir = os.path.join(env.subst("$PROJECT_PACKAGES_DIR"), "tool-esptoolpy")
    esptool_py = os.path.join(esptool_dir, "esptool.py")

    if not os.path.isfile(esptool_py):
        print(f"[wokwi] esptool.py not found at {esptool_py}; skipping merge")
        return

    python_exe = env.subst("$PYTHONEXE")

    cmd = [
        python_exe,
        esptool_py,
        "--chip",
        "esp32",
        "merge_bin",
        "-o",
        out_bin,
        "--flash_mode",
        str(flash_mode),
        "--flash_freq",
        str(flash_freq),
        "--flash_size",
        str(flash_size),
        "0x1000",
        bootloader,
        "0x8000",
        partitions,
        "0x10000",
        app_bin,
    ]

    print(f"[wokwi] Generating merged firmware: {out_bin}")
    try:
        # Use subprocess to avoid any SCons argument/Action quirks.
        subprocess.run(cmd, check=True)
    except Exception as exc:
        print(f"[wokwi] Failed to generate merged firmware: {exc}")


# After the app binary is created, generate a merged flash image usable by Wokwi.
env.AddPostAction(os.path.join("$BUILD_DIR", "firmware.bin"), _merge_for_wokwi)  # type: ignore[name-defined]
