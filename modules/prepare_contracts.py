#!/usr/bin/env python3
"""Adapt canonical header contracts into import-only development consumers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re


def write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    contracts = json.loads((root / "modules/contracts.json").read_text())
    standard = set()
    for package in ("teaql_memory", "teaql_async", "teaql_core", "teaql_data_service",
                    "teaql_sql", "teaql_runtime", "teaql_provider_sqlite"):
        for header in (root / package / "include").rglob("*.hpp"):
            standard.update(re.findall(r"^#include <([^>]+)>", header.read_text(), re.M))
    # Do not flatten platform-gated C headers (e.g. Windows process.h).
    standard = sorted(header for header in standard if re.fullmatch(r"[a-z_0-9]+", header))
    preamble = "".join(f"#include <{header}>\n" for header in standard)
    # Preserve the platform facilities used by original identity contracts.
    preamble += "#if defined(__linux__)\n#include <sys/syscall.h>\n#include <unistd.h>\n#endif\n"
    imports = "import teaql_provider_sqlite;\n"
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = []
    prepared = set()

    def adapt(source, destination, entry=False, qualified_calls=()):
        source = source.resolve()
        if source in prepared:
            return
        prepared.add(source)
        text = source.read_text()
        original = re.sub(r"^#include [^\n]+\n", "", text, flags=re.M)
        text = re.sub(r"^#include <teaql/[^>]+>\n", "", text, flags=re.M)

        def quoted_include(match):
            dependency = (source.parent / match[1]).resolve()
            if not dependency.is_relative_to(root) or not dependency.is_file():
                raise RuntimeError(f"Missing or external fixture include: {source}: {match[1]}")
            if dependency.relative_to(root).parts[0] not in {"tests", "examples"}:
                raise RuntimeError(f"Cannot copy a production implementation into test fixtures: {dependency}")
            target = args.output / dependency.relative_to(root)
            adapt(dependency, target)
            return '#include "' + os.path.relpath(target, destination.parent) + '"\n'

        text = re.sub(r'^#include "([^\"]+)"\n', quoted_include, text, flags=re.M)
        if entry:
            quoted = re.findall(r'^#include "[^\"]+"\n', text, re.M)
            text = re.sub(r'^#include "[^\"]+"\n', "", text, flags=re.M)
            includes = re.findall(r"^#include [^\n]+\n", text, re.M)
            text = re.sub(r"^#include [^\n]+\n", "", text, flags=re.M)
            # Complete imports expose teaql::request; preserve global fixture calls.
            for name in qualified_calls:
                text = re.sub(rf"(?<![\w:]){name}(?=\s*\()", "::" + name, text)
            text = (preamble + "".join(includes) + imports +
                    "// TEAQL canonical consumer begin\n" + "".join(quoted) + text)
        write_if_changed(destination, text)
        manifest.append({"source": str(source.relative_to(root)),
                         "consumer": str(destination.relative_to(args.output)),
                         "body_sha256": hashlib.sha256(original.encode()).hexdigest(),
                         "entry": entry,
                         "qualified_calls": list(qualified_calls)})

    for fixture in contracts["fixtures"]:
        adapt(root / fixture, args.output / fixture)
    for contract in contracts["contracts"]:
        source = (root / contract["source"] if "source" in contract else
                  root / contract["package"] / "tests" / (contract["stem"] + ".cpp"))
        adapt(source, args.output / (contract["stem"] + ".cpp"), entry=True,
              qualified_calls=contract.get("qualified_calls", ()))
    for example in contracts.get("examples", []):
        adapt(root / example, args.output / Path(example).name, entry=True)
    write_if_changed(args.output / "manifest.json", json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
