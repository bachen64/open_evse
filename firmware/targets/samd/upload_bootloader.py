"""
PlatformIO post-upload extra_script: after flashing firmware, also flash the
bootloader binary to address 0x0 via the same Atmel-ICE / OpenOCD session.

Reuses PlatformIO's UPLOADER and UPLOADERFLAGS (correct interface cfg, target
cfg, script paths, speed).  The flags are kept as a list the whole time so that
multi-word -c arguments (e.g. "set CHIPNAME at91samd21g18") are never split by
shell parsing — that was the cause of the "Unexpected command line argument:
CHIPNAME" error when they were joined into a flat string.

Only the -c argument that contains 'program' is replaced; all other flags
(scripts dir, interface, target, CHIPNAME setup, etc.) are passed through
unchanged.
"""

Import("env")  # noqa: F821 — SCons-injected global

import subprocess
from pathlib import Path


def upload_bootloader_ice(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))

    bl_bins = sorted(build_dir.glob("bootloader-*.bin"))
    if not bl_bins:
        print("WARNING: No bootloader-*.bin found in build dir — run a full build first")
        return
    bl_bin = bl_bins[0]

    # Forward slashes required inside OpenOCD's { } program argument on all OSes.
    bl_str = str(bl_bin).replace("\\", "/")

    uploader = env.subst("$UPLOADER")

    # Resolve each flag element individually (they may be SCons nodes or contain
    # unexpanded variables like $SOURCE).  env.Flatten() handles nested lists.
    raw_flags = [env.subst(str(f)) for f in env.Flatten(env["UPLOADERFLAGS"])]

    # Walk the flag list and swap out only the -c argument whose value contains
    # 'program'.  Every other flag is kept verbatim so PlatformIO's interface /
    # target / CHIPNAME configuration is preserved.
    new_flags = []
    i = 0
    while i < len(raw_flags):
        if raw_flags[i] == "-c" and i + 1 < len(raw_flags) and "program" in raw_flags[i + 1]:
            new_flags += [
                "-c",
                f"telnet_port disabled; program {{{bl_str}}} 0x0 verify reset; shutdown;",
            ]
            i += 2
        else:
            new_flags.append(raw_flags[i])
            i += 1

    cmd = [uploader] + new_flags

    print(f"\n--- Uploading bootloader via Atmel-ICE ---")
    print(f"  {bl_bin.name}  →  0x0")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.stdout:
        print(result.stdout)
    if result.returncode != 0:
        if result.stderr:
            print(result.stderr)
        print("ERROR: Bootloader upload failed")
    else:
        print("--- Bootloader upload complete ---\n")


env.AddPostAction("upload", upload_bootloader_ice)
