#!/usr/bin/env python3
"""Verify the reference without switching, fetching or modifying the source repo."""
import json
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parents[1]
lock = json.loads((root / "upstream.lock.json").read_text())
reference = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / ".reference/teaql-rs"

def git(*args):
    return subprocess.check_output(["git", "-C", str(reference), *args], text=True).strip()

try:
    commit = git("rev-parse", "HEAD")
    tag_commit = git("rev-parse", lock["tag"] + "^{commit}")
    if commit != lock["commit"] or tag_commit != lock["commit"]:
        sys.exit("Reference HEAD/tag differs from upstream.lock.json")
    if git("status", "--porcelain"):
        sys.exit("Reference has local changes; restore a clean pinned reference before comparing")
except subprocess.CalledProcessError:
    sys.exit("Cannot verify reference repository")
print(f"Verified {lock['tag']} @ {commit}")
