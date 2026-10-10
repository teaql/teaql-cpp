#!/usr/bin/env python3
"""Install and check the locked Linux x86_64 development toolchain locally."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[2]
LOCK = json.loads((ROOT / "toolchains.lock.json").read_text())
INSTALL = ROOT / ".tools/toolchains"
DOWNLOAD = ROOT / ".tools/toolchain-upgrade"


def run(arguments, cwd=None):
    subprocess.run([str(arg) for arg in arguments], cwd=cwd, check=True)


def archive(tool):
    path = DOWNLOAD / tool["file"]
    algorithm = "sha512" if "sha512" in tool else "sha256"
    expected = tool[algorithm]
    if not path.exists():
        partial = path.with_name(path.name + ".partial")
        urllib.request.urlretrieve(tool["url"], partial)
        partial.replace(path)
    digest = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    if digest.hexdigest() != expected:
        raise RuntimeError(f"{tool['tool']}: {algorithm} mismatch: {path}")
    return path


def install(tool):
    destination = INSTALL / tool["directory"]
    marker = destination / ".teaql-installed.json"
    if marker.is_file():
        if json.loads(marker.read_text()) != tool:
            raise RuntimeError(f"Installed directory does not match the lock: {destination}")
        return
    destination.mkdir(parents=True, exist_ok=True)
    path = None if tool["tool"] == "gcc" else archive(tool)
    if tool["tool"] == "ninja":
        with zipfile.ZipFile(path) as source:
            source.extract("ninja", destination)
        (destination / "ninja").chmod(0o755)
    elif tool["tool"] == "gcc":
        for package in tool["packages"]:
            path = archive(dict(package, tool=package["package"]))
            with zipfile.ZipFile(path) as source:
                member = next(name for name in source.namelist()
                              if name.startswith("pkg-") and name.endswith(".tar.zst"))
                source.extract(member, DOWNLOAD)
            run(["tar", "-I", "zstd --long=30 -d", "-xf", DOWNLOAD / member,
                 "-C", destination])
        for alias, target in (("gcc", "x86_64-conda-linux-gnu-gcc"),
                              ("g++", "x86_64-conda-linux-gnu-g++"),
                              ("cc", "x86_64-conda-linux-gnu-gcc"),
                              ("c++", "x86_64-conda-linux-gnu-g++")):
            path = destination / "bin" / alias
            if not path.exists():
                path.symlink_to(target)
    elif tool["tool"] == "sqlite":
        include = destination / "include"
        library = destination / "lib"
        include.mkdir(exist_ok=True)
        library.mkdir(exist_ok=True)
        with zipfile.ZipFile(path) as source:
            for name in ("sqlite3.c", "sqlite3.h", "sqlite3ext.h"):
                (include / name).write_bytes(source.read("sqlite-amalgamation-3530200/" + name))
        if hashlib.sha256((include / "sqlite3.c").read_bytes()).hexdigest() != tool["source_sha256"]:
            raise RuntimeError("SQLite source does not match the accepted source")
        cross = INSTALL / "openwrt-25.12.5/toolchain-mips_24kc_gcc-14.3.0_musl"
        run(["env", f"STAGING_DIR={cross}", cross / "bin/mips-openwrt-linux-musl-gcc",
             "-march=74kc", "-msoft-float", "-Os", "-ffunction-sections", "-fdata-sections",
             "-DSQLITE_THREADSAFE=1", "-DSQLITE_ENABLE_COLUMN_METADATA",
             "-c", include / "sqlite3.c", "-o", destination / "sqlite3.o"])
        run([cross / "bin/mips-openwrt-linux-musl-ar", "rcs", library / "libsqlite3.a",
             destination / "sqlite3.o"])
    else:
        arguments = ["tar"]
        if path.name.endswith(".zst"):
            arguments += ["-I", "zstd --long=30 -d"]
        run(arguments + ["-xf", path, "-C", destination, "--strip-components=1"])
    (destination / ".teaql-installed.json").write_text(json.dumps(tool, indent=2) + "\n")


def check():
    executables = {
        "clang++": ("llvm-23.1.3/bin/clang++", "23.1.3"),
        "clang-scan-deps": ("llvm-23.1.3/bin/clang-scan-deps", "23.1.3"),
        "g++": ("gcc-16.2.0/bin/g++", "16.2.0"),
        "cmake": ("cmake-4.4.4/bin/cmake", "4.4.4"),
        "ninja": ("ninja-1.13.2/ninja", "1.13.2"),
        "mips-g++": ("openwrt-25.12.5/toolchain-mips_24kc_gcc-14.3.0_musl/bin/mips-openwrt-linux-musl-g++", "14.3.0"),
    }
    for name, (relative, version) in executables.items():
        executable = INSTALL / relative
        result = subprocess.run([str(executable), "--version"], capture_output=True, text=True, check=True)
        if version not in result.stdout:
            raise RuntimeError(f"Wrong {name} version: {result.stdout}")
        print(f"{name}: {version} ({executable})")
    if not (INSTALL / "sqlite-3.53.2/lib/libsqlite3.a").is_file():
        raise RuntimeError("Missing newly compiled MIPS SQLite archive")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Check installed tools without downloading/building")
    args = parser.parse_args()
    if not args.check:
        INSTALL.mkdir(parents=True, exist_ok=True)
        DOWNLOAD.mkdir(parents=True, exist_ok=True)
        for tool in LOCK["tools"]:
            install(tool)
        gcc = INSTALL / "gcc-16.2.0"
        configuration = INSTALL / "llvm-23.1.3/bin/clang++.cfg"
        configuration.write_text(f"--gcc-install-dir={gcc}/lib/gcc/x86_64-conda-linux-gnu/16.2.0\n-Wl,-rpath,{gcc}/lib\n")
    check()


if __name__ == "__main__":
    main()
