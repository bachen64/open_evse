"""
PlatformIO extra_script for the OpenEVSE NXT SAMD21 build.

Pre-build : compile the uf2-samdx1 bootloader using PlatformIO's own
            arm-none-eabi toolchain (no external 'make' required).
            Output: uf2-samdx1/build/OpenEVSE_NXT/bootloader-*.bin  (+.uf2)

Post-build: convert the PlatformIO firmware binary to UF2 format so it can
            be drag-and-drop flashed via the UF2 bootloader's USB drive.
            Output: $BUILD_DIR/firmware.uf2

The uf2-samdx1 directory must exist inside this project root.
"""

Import("env")  # noqa: F821 — SCons-injected global

import shutil
import subprocess
import sys
from pathlib import Path

BOARD = "OpenEVSE_NXT"
CHIP_VARIANT = "SAMD21G18A"
BOOTLOADER_SIZE = 8192  # bytes — SAMD21 bootloader partition

# Main bootloader sources (mirrors SOURCES in the Makefile)
SOURCES = [
    "src/flash_samd21.c",
    "src/init_samd21.c",
    "src/startup_samd21.c",
    "src/usart_sam_ba.c",
    "src/screen.c",
    "src/images.c",
    "src/utils.c",
    "src/cdc_enumerate.c",
    "src/fat.c",
    "src/main.c",
    "src/msc.c",
    "src/sam_ba_monitor.c",
    "src/uart_driver.c",
    "src/hid.c",
]


def _run(cmd, cwd=None):
    result = subprocess.run(
        [str(c) for c in cmd],
        capture_output=True, text=True, cwd=cwd
    )
    if result.returncode != 0:
        print(f"\nFailed: {' '.join(str(c) for c in cmd)}")
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(result.stderr)
        raise RuntimeError(f"Exit code {result.returncode}")
    return result.stdout.strip()


def _needs_rebuild(output: Path, sources):
    if not output.exists():
        return True
    out_mtime = output.stat().st_mtime
    return any(s.exists() and s.stat().st_mtime > out_mtime for s in sources)


def build_bootloader(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    bl_dir = project_dir / "uf2-samdx1"
    build_path = bl_dir / "build" / BOARD

    if not bl_dir.exists():
        print(f"WARNING: bootloader directory not found: {bl_dir}")
        print("  Clone https://github.com/adafruit/uf2-samdx1 next to this project.")
        return

    # PlatformIO sets $CC to the full path of arm-none-eabi-gcc it installed.
    cc = Path(env.subst("$CC"))
    objcopy = cc.parent / "arm-none-eabi-objcopy"

    # Version string from the bootloader repo's git history.
    try:
        version = _run(
            ["git", "describe", "--dirty=+", "--always", "--tags"],
            cwd=str(bl_dir),
        )
    except Exception:
        version = "unknown"

    name = f"bootloader-{BOARD}-{version}"
    bin_path = build_path / f"{name}.bin"

    # Rebuild check — same logic as make's file dependency tracking.
    watch = (
        list((bl_dir / "src").glob("*.c"))
        + list((bl_dir / "inc").glob("**/*.h"))
        + list((bl_dir / f"boards/{BOARD}").glob("*.h"))
        + [bl_dir / "Makefile"]
    )
    if not _needs_rebuild(bin_path, watch):
        print(f"Bootloader up to date: {bin_path.name}")
        return

    print(f"\n--- Building bootloader for {BOARD} ---")
    build_path.mkdir(parents=True, exist_ok=True)

    # Generate uf2_version.h (replaces the make rule that echo-writes the file).
    (build_path / "uf2_version.h").write_text(
        f'#define UF2_VERSION_BASE "{version}"\n'
    )

    common = ["-mthumb", "-mcpu=cortex-m0plus", "-Os", "-g", "-DSAMD21"]

    cflags = common + [
        "-x", "c", "-c", "-pipe", "-nostdlib",
        "-fno-strict-aliasing", "-fdata-sections", "-ffunction-sections",
        f"-D__{CHIP_VARIANT}__",
        "-fno-jump-tables",
        # Warning flags from the Makefile WFLAGS variable.
        "-Werror", "-Wall", "-Wstrict-prototypes",
        "-Werror-implicit-function-declaration", "-Wpointer-arith", "-std=gnu99",
        "-Wchar-subscripts", "-Wcomment", "-Wformat=2",
        "-Wimplicit-int", "-Wmain", "-Wparentheses", "-Wsequence-point",
        "-Wreturn-type", "-Wswitch", "-Wtrigraphs", "-Wunused", "-Wuninitialized",
        "-Wunknown-pragmas", "-Wfloat-equal", "-Wno-undef", "-Wbad-function-cast",
        "-Wwrite-strings", "-Waggregate-return", "-Wmissing-format-attribute",
        "-Wno-deprecated-declarations", "-Wpacked", "-Wredundant-decls",
        "-Wnested-externs", "-Wlong-long", "-Wunreachable-code", "-Wcast-align",
        "-Wno-missing-braces", "-Wno-overflow", "-Wno-shadow", "-Wno-attributes",
        "-Wno-packed", "-Wno-pointer-sign",
    ]

    includes = [
        f"-I{bl_dir}",
        f"-I{bl_dir / 'inc'}",
        f"-I{bl_dir / 'inc' / 'preprocessor'}",
        f"-I{bl_dir / 'boards' / BOARD}",
        f"-I{bl_dir / 'lib' / 'cmsis' / 'CMSIS' / 'Include'}",
        f"-I{bl_dir / 'lib' / 'usb_msc'}",
        f"-I{bl_dir / 'lib' / 'samd21' / 'samd21a' / 'include'}",
        f"-I{build_path}",
    ]

    # Compile each source file.
    objects = []
    for src in SOURCES:
        src_path = bl_dir / src
        obj_path = build_path / (Path(src).stem + ".o")
        try:
            _run([cc] + cflags + includes + [src_path, "-o", obj_path])
        except RuntimeError:
            env.Exit(1)
            return
        objects.append(obj_path)

    # Link.
    elf_path = build_path / f"{name}.elf"
    linker_script = bl_dir / "scripts" / "samd21j18a.ld"
    map_path = build_path / f"{name}.map"

    ldflags = common + [
        "-Wall",
        "-Wl,--cref", "-Wl,--check-sections", "-Wl,--gc-sections",
        "-Wl,--unresolved-symbols=report-all", "-Wl,--warn-common",
        "-Wl,--warn-section-align",
        "-nostartfiles",
        "--specs=nano.specs", "--specs=nosys.specs",
        f"-L{build_path}",
        f"-T{linker_script}",
        f"-Wl,-Map,{map_path}",
        "-o", elf_path,
    ]

    try:
        _run([cc] + ldflags + objects, cwd=str(build_path))
    except RuntimeError:
        env.Exit(1)
        return

    # Produce the flat binary.
    try:
        _run([objcopy, "-O", "binary", elf_path, bin_path])
    except RuntimeError:
        env.Exit(1)
        return

    # Produce the self-update UF2 (optional — needs the uf2 submodule).
    uf2conv = bl_dir / "lib" / "uf2" / "utils" / "uf2conv.py"
    if uf2conv.exists():
        uf2_path = build_path / f"update-{name}.uf2"
        try:
            _run([
                sys.executable, str(uf2conv),
                "-b", str(BOOTLOADER_SIZE), "-c",
                "-o", str(uf2_path),
                str(bin_path),
            ])
            print(f"  UF2 self-update : {uf2_path.name}")
        except RuntimeError:
            pass  # UF2 generation is nice-to-have, not required for flashing

    # Copy bootloader .bin to the PlatformIO build directory alongside firmware.
    build_dir = Path(env.subst("$BUILD_DIR"))
    dest = build_dir / bin_path.name
    build_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(bin_path, dest)

    print(f"  Bootloader .bin : {bin_path.name}")
    print(f"  Copied to       : {dest}")

    # Clean the bootloader build tree now that the .bin is safely copied.
    shutil.rmtree(build_path, ignore_errors=True)
    print(f"  Cleaned         : {build_path}")
    print(f"--- Bootloader build complete ---\n")


def convert_firmware_to_uf2(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))
    prog_name = env.subst("$PROGNAME")
    elf_path = build_dir / f"{prog_name}.elf"
    bin_path = build_dir / f"{prog_name}.bin"
    uf2_path = build_dir / f"{prog_name}.uf2"

    project_dir = Path(env.subst("$PROJECT_DIR"))
    uf2conv = project_dir / "uf2-samdx1" / "lib" / "uf2" / "utils" / "uf2conv.py"

    if not uf2conv.exists():
        print(f"WARNING: uf2conv.py not found at {uf2conv}, skipping UF2 conversion")
        return

    # The .bin is produced by the platform as a side-effect of the .elf build and
    # may not exist yet when this post-action fires.  Generate it ourselves if needed.
    if not bin_path.exists():
        objcopy = Path(env.subst("$CC")).parent / "arm-none-eabi-objcopy"
        try:
            _run([str(objcopy), "-O", "binary", str(elf_path), str(bin_path)])
        except RuntimeError:
            print("WARNING: objcopy failed, cannot produce .bin or .uf2")
            return

    print(f"\n--- Converting firmware to UF2 ---")
    try:
        # -b 0x2000 : application base address (after 8 KB SAMD21 bootloader)
        # -f 0x68ed2b88 : SAMD21 family ID
        # -c : convert only, do not flash
        _run([
            sys.executable, str(uf2conv),
            "-b", "0x2000",
            "-f", "0x68ed2b88",
            "-c",
            "-o", str(uf2_path),
            str(bin_path),
        ])
        print(f"  Firmware UF2    : {uf2_path}")
    except RuntimeError:
        print("WARNING: UF2 conversion failed — .bin is still available for JLink/ICE flashing")


env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", build_bootloader)
# Hook into the ELF (a real SCons target), not the .bin (a side-effect artifact
# that SCons does not track and therefore never triggers AddPostAction).
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", convert_firmware_to_uf2)
