#!/usr/bin/env python3
"""Build native SQLite from the source already pinned for the MIPS profile."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LOCK = json.loads((ROOT / 'toolchains.lock.json').read_text())
TOOLS = ROOT / '.tools/toolchains'
SQLITE = next(tool for tool in LOCK['tools'] if tool['tool'] == 'sqlite')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    directory = TOOLS / SQLITE['directory']
    source = directory / 'include/sqlite3.c'
    if hashlib.sha256(source.read_bytes()).hexdigest() != SQLITE['source_sha256']:
        raise SystemExit('Native SQLite source differs from toolchains.lock.json')
    destination = directory / 'lib_native'
    archive = destination / 'libsqlite3.a'
    compiler = TOOLS / 'llvm-23.1.3/bin/clang'
    flags = ['-O2', '-fPIC', '-DSQLITE_THREADSAFE=1', '-DSQLITE_ENABLE_COLUMN_METADATA', '-DSQLITE_SOUNDEX']
    stamp = dict(source_sha256=SQLITE['source_sha256'], version=SQLITE['version'],
                 compiler=str(compiler), flags=flags)
    marker = destination / 'provenance.json'
    if not args.check and (not archive.is_file() or not marker.is_file() or
                           json.loads(marker.read_text()) != stamp):
        destination.mkdir(parents=True, exist_ok=True)
        obj = destination / 'sqlite3.o'
        subprocess.run([str(compiler), *flags, '-c', str(source), '-o', str(obj)], check=True)
        subprocess.run([str(TOOLS/'llvm-23.1.3/bin/llvm-ar'), 'rcs', str(archive), str(obj)], check=True)
        marker.write_text(json.dumps(stamp, indent=2) + '\n')
    if not archive.is_file() or not marker.is_file() or json.loads(marker.read_text()) != stamp:
        raise SystemExit('Native SQLite archive/provenance is missing or stale')
    print(f'Native SQLite {SQLITE["version"]}: {archive}')


if __name__ == '__main__':
    main()
