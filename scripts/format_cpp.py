#!/usr/bin/env python3
"""Install, apply, or check the pinned formatter for handwritten project C++."""
import argparse
import fnmatch
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import venv

ROOT = Path(__file__).resolve().parents[1]
TOOL_ENV = ROOT / ".tools" / "clang-format"
TOOL_BIN = TOOL_ENV / ("Scripts" if os.name == "nt" else "bin")
VERSION = "18.1.8"
EXTENSIONS = {".cpp", ".cc", ".cxx", ".cppm", ".h", ".hpp"}


def install():
    venv.EnvBuilder(with_pip=True, symlinks=os.name != "nt").create(TOOL_ENV)
    python = TOOL_BIN / ("python.exe" if os.name == "nt" else "python")
    subprocess.run(
        [str(python), "-m", "pip", "install", "-r", str(ROOT / "scripts/requirements_format.txt")],
        check=True,
    )


def formatter(explicit):
    candidates = [explicit] if explicit else [
        str(TOOL_BIN / ("clang-format.exe" if os.name == "nt" else "clang-format")),
        shutil.which("clang-format-18"),
        shutil.which("clang-format"),
    ]
    for candidate in candidates:
        if not candidate:
            continue
        try:
            result = subprocess.run([candidate, "--version"], text=True, capture_output=True, check=True)
        except (OSError, subprocess.CalledProcessError):
            continue
        if re.search(rf"\bversion {re.escape(VERSION)}\b", result.stdout):
            return os.path.abspath(shutil.which(candidate) or candidate)
    raise RuntimeError(
        f"clang-format {VERSION} is required. Run python3 scripts/format_cpp.py --install, "
        "or supply --clang-format PATH."
    )


def source_files():
    patterns = [line.strip() for line in (ROOT / ".clang-format-ignore").read_text().splitlines()
                if line.strip() and not line.lstrip().startswith("#")]
    # Git excludes nested build trees and third-party generated artifacts too.
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z", "--",
         "teaql_memory", "teaql_async", "teaql_core", "teaql_data_service",
         "teaql_sql", "teaql_runtime", "teaql_provider_sqlite", "compatibility",
         "modules", "examples", "tests"],
        cwd=ROOT, capture_output=True, check=True,
    )
    files = []
    for relative in {os.fsdecode(name) for name in result.stdout.split(b"\0") if name}:
        path = ROOT / relative
        if not path.is_file() or path.suffix not in EXTENSIONS:
            continue
        if not any(fnmatch.fnmatchcase(relative, pattern) for pattern in patterns):
            files.append(relative)
    return sorted(files)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_mutually_exclusive_group()
    actions.add_argument("--install", action="store_true", help="Install the pinned tool locally; do not format")
    actions.add_argument("--check", action="store_true", help="Reject formatting differences without writing")
    parser.add_argument("--clang-format", metavar="PATH", help="Use this exact formatter executable")
    args = parser.parse_args()
    if args.install:
        install()
        print(f"Installed clang-format {VERSION} in {TOOL_ENV}")
        return 0
    executable = formatter(args.clang_format)
    files = source_files()
    command = [executable, f"--style=file:{ROOT / '.clang-format'}", "--Werror"]
    command += ["--dry-run", "--ferror-limit=1"] if args.check else ["-i"]
    failed = False
    for offset in range(0, len(files), 32):
        result = subprocess.run(command + files[offset:offset + 32], cwd=ROOT)
        failed |= result.returncode != 0
    if failed:
        return 1
    action = "Checked" if args.check else "Formatted"
    print(f"{action} {len(files)} handwritten C++ files with clang-format {VERSION}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
