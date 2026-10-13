#!/usr/bin/env python3
"""Record pinned Rust source declarations and tests; this is not a parity verdict.

Uses committed files, never a moving working-tree snapshot. --check detects stale
inventory. No Rust generator, package manager, or network access is involved.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CRATES = ("teaql-core", "teaql-data-service", "teaql-sql", "teaql-runtime", "teaql-provider-sqlite")
parser = argparse.ArgumentParser()
parser.add_argument("--reference", type=Path, default=ROOT / ".reference/teaql-rs")
parser.add_argument("--check", action="store_true")
parser.add_argument("--inventory-only", action="store_true",
                    help="Index the active source without promoting a historical module map")
parser.add_argument("--output-dir", type=Path, default=ROOT / "docs/parity")
args = parser.parse_args()
args.output_dir = args.output_dir.resolve()
lock = json.loads((ROOT / "upstream.lock.json").read_text())

def git(*argv):
    return subprocess.check_output(["git", "-C", str(args.reference), *argv], text=True)

commit = lock["commit"]
if git("rev-parse", lock["tag"] + "^{commit}").strip() != commit:
    raise SystemExit("Reference tag differs from locked commit")
paths = git("ls-tree", "-r", "--name-only", commit).splitlines()
files = []
for path in paths:
    parts = path.split("/")
    if parts[0] not in CRATES or parts[1] not in ("src", "tests") or not path.endswith(".rs"):
        continue
    source = git("show", f"{commit}:{path}")
    declarations = []
    for m in re.finditer(r"^(pub(?:\(crate\))?)\s+(struct|enum|trait|type)\s+(\w+)", source, re.M):
        declarations.append({"name": m[3], "kind": m[2], "visibility": m[1],
                             "line": source.count("\n", 0, m.start()) + 1})
    tests = []
    # Explicit adapter for the pinned source's six mask_case! modules. The two
    # template functions are not runnable tests until their module is expanded.
    mask_macro = None
    if path == "teaql-runtime/src/masking_contract_tests.rs":
        mask_macro = re.search(r"macro_rules! mask_case \{.*?\n\}", source, re.S)
        if not mask_macro:
            raise SystemExit("Pinned mask_case macro shape changed")
    for m in re.finditer(r"#\[(?:\w+::)?test(?:\([^\]]*\))?\]\s*(?:#\[[^\]]*\]\s*)*(?:async\s+)?fn\s+(\w+)", source):
        if mask_macro and mask_macro.start() <= m.start() < mask_macro.end():
            continue
        tests.append({"name": m[1], "line": source.count("\n", 0, m.start()) + 1})
    if mask_macro:
        templates = re.findall(r"#\[test\]\s*fn\s+(\w+)", mask_macro[0])
        invocations = list(re.finditer(r"^mask_case!\(\s*(\w+)\s*,", source, re.M))
        if templates != ["mask_contract_expanded_sql", "mask_contract_audit_algorithm"] or len(invocations) != 6:
            raise SystemExit("Pinned mask_case test expansion changed")
        for invocation in invocations:
            for template in templates:
                tests.append({"name": f"{invocation[1]}::{template}",
                              "line": source.count("\n", 0, invocation.start()) + 1,
                              "macro": "mask_case", "template": template})
        tests.sort(key=lambda test: (test["line"], test["name"]))
    # Also index public methods/functions, constants, exports and module declarations.
    # This is a source index, not a Rust parser: trait defaults, cfg reachability,
    # macro expansion and serialized representations still require semantic review.
    public_items = []
    for m in re.finditer(r"^[ \t]*pub(?:\(crate\))?\s+(?:(?:async|const|unsafe)\s+)*(fn|const|static|use|mod)\s+([^\n]+)", source, re.M):
        public_items.append({"kind": m[1], "source": m[0].strip(),
                             "line": source.count("\n", 0, m.start()) + 1})
    files.append({"path": path, "sha256": hashlib.sha256(source.encode()).hexdigest(),
                  "declarations": declarations, "public_items": public_items, "tests": tests})
manifest = {"commit": commit, "tag": lock["tag"], "crates": list(CRATES),
            "note": "Source-declaration inventory, including cfg-gated and source-only modules. Presence does not prove API reachability or C++ parity. Public functions/reexports and behavioral contracts require the design review.",
            "files": files}
json_text = json.dumps(manifest, indent=2, ensure_ascii=False) + "\n"
design_link = Path(os.path.relpath(ROOT / "docs/design_parity.md", args.output_dir)).as_posix()
md = [f"# Rust {lock['version']} source concept inventory", "", f"Pinned commit: `{commit}`.", "",
      "Generated from committed source objects by `scripts/inventory_upstream.py`.",
      "Declarations, locations, and test counts do not establish parity. The index includes",
      f"conditional and unexported sources; see the [historical design contract]({design_link}).", "",
      "| Rust file | Declared core types / abstractions | Source tests |", "| --- | --- | --- |"]
for f in files:
    if not f["declarations"]:
        continue
    names = ", ".join(f"`{d['name']}`" for d in f["declarations"])
    md.append(f"| {f['path']} | {names} | {len(f['tests'])} |")
md_text = "\n".join(md) + "\n"
outputs = {args.output_dir / "upstream_inventory.json": json_text,
           args.output_dir / "upstream_concepts.md": md_text}

if not args.inventory_only:
    mapping = json.loads((ROOT / "docs/parity/cpp_modules.json").read_text())
    if mapping["commit"] != commit:
        raise SystemExit("C++ module map differs from locked commit")
    expected = {f["path"] for f in files}
    mapped = set()
    status_labels = {"partial": "Partial", "missing": "Pending implementation/validation",
                     "accepted": "Accepted module contract (not stage completion)",
                     "reference_only": "Reference oracle", "excluded_external": "External adapter excluded"}
    rows = ["# Rust to C++ module and acceptance map", "", f"Pinned commit: `{commit}`.", "",
            "Generated from the manually maintained responsibilities in `cpp_modules.json`; planned paths may not exist.",
            "All declarations, public items, and tests in each source inherit its row's contract. The public-item index",
            "locates methods, functions, constants, exports, and modules; it cannot replace trait-default or serialization review.",
            "Partial implementation is not stage acceptance. Stage 6 tests may accompany their implementations early.", "",
            "| Rust source | C++ target | Stage / status | Acceptance contract | Existing evidence (not complete parity proof) |",
            "| --- | --- | --- | --- | --- |"]
    for group in mapping["groups"]:
        if group["status"] not in status_labels or group["stage"] not in range(1, 7):
            raise SystemExit("Invalid module stage/status")
        if group["status"] == "accepted":
            acceptance = group.get("acceptance", {})
            if acceptance.get("commit") != commit or not acceptance.get("review"):
                raise SystemExit("Accepted module requires a pinned, explicit review")
            if acceptance.get("sources") != group["sources"] or not acceptance.get("checks"):
                raise SystemExit("Accepted module requires complete source locators and named checks")
            for requirement in ("public_surface", "defaults_and_traits", "wire_protocol", "ownership"):
                if not acceptance.get(requirement):
                    raise SystemExit(f"Accepted module requires review of {requirement}")
            if acceptance["review"] not in group["evidence"]:
                raise SystemExit("Accepted module review must be existing evidence")
        for source in group["sources"]:
            if source not in expected or source in mapped:
                raise SystemExit(f"Unknown or duplicate source mapping: {source}")
            mapped.add(source)
        for evidence in group["evidence"]:
            if not (ROOT / evidence).is_file():
                raise SystemExit(f"Missing existing evidence: {evidence}")
        sources = "<br>".join(f"`{s}`" for s in group["sources"])
        evidence = "<br>".join(f"[{p}](../../{p})" for p in group["evidence"]) or "—"
        rows.append(f"| {sources} | `{group['cpp_target']}` | {group['stage']} / {status_labels[group['status']]} | {group['contract']} | {evidence} |")
    if mapped != expected:
        raise SystemExit(f"Unmapped source files: {sorted(expected - mapped)}")
    outputs[args.output_dir / "cpp_module_map.md"] = "\n".join(rows) + "\n"
for path, content in outputs.items():
    if not args.check:
        path.parent.mkdir(parents=True, exist_ok=True)
    if args.check:
        if not path.exists() or path.read_text() != content:
            raise SystemExit(f"Stale inventory: {path.relative_to(ROOT)}")
    else:
        path.write_text(content)
print(f"{'Verified' if args.check else 'Recorded'} {len(files)} Rust source files, "
      f"{sum(len(f['declarations']) for f in files)} declarations, "
      f"{sum(len(f['tests']) for f in files)} source tests; "
      + ("source inventory only (not parity acceptance)" if args.inventory_only
         else f"{len(mapping['groups'])} module contracts"))
