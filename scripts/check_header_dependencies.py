#!/usr/bin/env python3
"""Reject compiled Ninja objects with missing recorded compiler dependencies."""
import argparse
import re
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    args = parser.parse_args()
    targets = subprocess.run(['ninja', '-C', str(args.build_dir), '-t', 'targets', 'all'],
                             text=True, capture_output=True, check=True)
    active = {line.partition(': ')[0] for line in targets.stdout.splitlines()}
    result = subprocess.run(['ninja', '-C', str(args.build_dir), '-t', 'deps'],
                            text=True, capture_output=True, check=True)
    missing = []
    compiled = 0
    for name, count in re.findall(r'^(.+?\.o): #deps (\d+),', result.stdout, re.M):
        # Source moves can leave retired objects in an incremental build tree.
        # Verify the current graph rather than counting those stale artifacts.
        if name not in active or not (args.build_dir / name).is_file():
            continue
        compiled += 1
        if int(count) == 0:
            missing.append(name)
    if compiled == 0:
        print('No compiled object dependency records are available to verify')
        return 1
    if missing:
        print(f'{len(missing)} compiled objects have empty dependency records:')
        for name in missing:
            print(name)
        print('A clean rebuild is required before relying on header-sensitive integration.')
        return 1
    print(f'Checked nonempty compiler dependency records for {compiled} compiled objects')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
