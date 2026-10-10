#!/usr/bin/env python3
"""Check native facade archives for duplicate out-of-line implementation symbols."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--nm", type=Path,
                        default=ROOT / ".tools/toolchains/llvm-23.1.3/bin/llvm-nm")
    args = parser.parse_args()
    catalog = json.loads((ROOT / "docs/module_exports.json").read_text())
    records = []
    for package in catalog["packages"]:
        archive = args.build_dir / "modules" / f"lib{package}_module.a"
        if not archive.is_file():
            raise SystemExit(f"Missing completed native module archive: {archive}")
        result = subprocess.run([str(args.nm), "--defined-only", "--extern-only",
                                 "--format=posix", str(archive)], capture_output=True,
                                text=True, check=True)
        strong = []
        for line in result.stdout.splitlines():
            fields = line.split()
            if len(fields) >= 2 and fields[1] in {"T", "D", "B", "R", "S", "G"}:
                strong.append(fields[0])
        # Clang emits one externally visible initializer for each named module.
        # These distinct ABI entry points are not copies of library functions.
        initializer = f"_ZGIW{len(package)}{package}"
        if strong != [initializer]:
            raise SystemExit(f"Unexpected strong facade definitions in {archive}: {strong}")
        records.append({"archive": archive.name, "strong_definitions": strong,
                        "module_initializer": initializer,
                        "out_of_line_implementation_definitions": []})
    evidence = {
        "scope": "Native Clang23/Itanium ABI facade archives; weak inline/template symbols are allowed",
        "archives": records,
    }
    if args.output:
        args.output.write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"Verified {len(records)} native facade archives: only distinct module initializers "
          "are strong definitions; no duplicated out-of-line implementation")


if __name__ == "__main__":
    main()
