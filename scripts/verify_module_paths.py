#!/usr/bin/env python3
"""Verify active package/source paths; replay immutable move bodies at their checkpoint."""
import argparse
import io
import json
from pathlib import Path
import re
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args])


def load(path):
    return json.loads((ROOT / path).read_text())


def safe_path(root, spelling):
    path = Path(spelling)
    if path.is_absolute() or '..' in path.parts:
        raise ValueError(f'Invalid owned path: {spelling}')
    result = (root / path).resolve()
    if not result.is_relative_to(root.resolve()):
        raise ValueError(f'Owned path escapes the workspace: {spelling}')
    return result


def verify_packages(root, packages, paths):
    for package, owner in packages.items():
        if not re.fullmatch(r'teaql_[a-z]+(?:_[a-z]+)*', package):
            raise ValueError(f'Invalid package name: {package}')
        definition = safe_path(root, f'{package}/CMakeLists.txt').read_text()
        definition = re.sub(r'#[^\n]*', '', definition)
        if not re.search(rf'\badd_library\(\s*{package}(?:\s|\))', definition):
            raise ValueError(f'Package target differs from its directory: {package}')
        interface = safe_path(root, f'{package}/src/lib.cppm').read_text()
        if not re.search(rf'^export module {package};$', interface, re.M):
            raise ValueError(f'Primary module differs from its package: {package}')
        if owner.get('rust_crate') is None and not owner.get('adaptation'):
            raise ValueError(f'Unspecified adaptation responsibility: {package}')
    production = 0
    for spelling in paths:
        if Path(spelling).parts[0] not in packages:
            continue
        if any('-' in part for part in Path(spelling).parts):
            raise ValueError(f'Owned package path uses a hyphen: {spelling}')
        path = safe_path(root, spelling)
        if not path.is_file():
            raise ValueError(f'Missing tracked/active package file: {spelling}')
        if path.suffix in {'.cpp', '.cppm'}:
            if not spelling.startswith(f'{Path(spelling).parts[0]}/src/') and '/tests/' not in spelling:
                raise ValueError(f'Production implementation is outside package src: {spelling}')
            production += '/src/' in spelling
    return production


def verify_source_map(lock, contract, inventory, dispositions):
    if inventory['commit'] != lock['commit'] or dispositions['commit'] != lock['commit']:
        raise ValueError('Active source inventory/dispositions differ from the runtime lock')
    if contract['baseline'] != lock['commit']:
        raise ValueError('Active workspace contract differs from the runtime lock')
    records = {item['path']: item for item in inventory['files']}
    reviewed = {item['source']: item for item in dispositions['sources']}
    if len(records) != len(inventory['files']) or len(reviewed) != len(dispositions['sources']):
        raise ValueError('Duplicate active source locator')
    if records.keys() != reviewed.keys():
        raise ValueError('Active source inventory and disposition coverage differ')
    owners = {owner['rust_crate']: package for package, owner in contract['packages'].items()
              if owner.get('rust_crate')}
    if len(owners) != sum(bool(owner.get('rust_crate')) for owner in contract['packages'].values()):
        raise ValueError('Multiple packages claim the same Rust crate')
    if set(owners) != set(inventory['crates']):
        raise ValueError('Active in-scope Rust crate ownership is incomplete')
    for source, record in records.items():
        if source.split('/')[0] not in owners:
            raise ValueError(f'No package responsibility for source: {source}')
        if reviewed[source]['sha256'] != record['sha256']:
            raise ValueError(f'Stale active disposition hash: {source}')
    return len(records)


def verify_historical_records(contract):
    history = contract['historical_move']
    checkpoint = history['cpp_commit']
    if git('rev-parse', history['cpp_tag'] + '^{commit}').decode().strip() != checkpoint:
        raise ValueError('Historical C++ tag differs from its fixed checkpoint')
    old_lock = json.loads(git('show', checkpoint + ':upstream.lock.json'))
    if old_lock['commit'] != history['rust_commit']:
        raise ValueError('Historical runtime lock differs from the recorded move baseline')
    for spelling in history['ledgers']:
        archived = git('show', checkpoint + ':' + spelling)
        current = safe_path(ROOT, spelling).read_bytes()
        if current != archived:
            raise ValueError(f'Historical move evidence was rewritten: {spelling}')
    move = load('docs/module_path_migration.json')
    retired = load('docs/retired_compatibility.json')
    structure = load('docs/module_structure_migration.json')
    if any(ledger['baseline'] != history['rust_commit'] for ledger in [move, retired, structure]):
        raise ValueError('Historical ledger baselines disagree')
    retired_paths = {record['new'] for record in retired['files']}
    relocated = {record['source']: record['destination'] for record in structure['relocations']}
    destinations = set()
    for record in move['files']:
        old, new = record['old'], record['new']
        if new in destinations:
            raise ValueError(f'Duplicate historical destination: {new}')
        destinations.add(new)
        if safe_path(ROOT, old).exists():
            raise ValueError(f'Pre-move path was restored: {old}')
        destination = safe_path(ROOT, new)
        if new in retired_paths:
            if destination.exists():
                raise ValueError(f'Retired compatibility path was restored: {new}')
        elif not destination.is_file():
            raise ValueError(f'Historical destination is missing: {new}')
        if new in relocated:
            canonical = safe_path(ROOT, relocated[new])
            include_root = ROOT / Path(new).parts[0] / 'include'
            if not canonical.is_relative_to(include_root) or not canonical.is_file():
                raise ValueError(f'Missing canonical declaration destination: {new}')
            spelling = canonical.relative_to(include_root).as_posix()
            if f'#include <{spelling}>' not in destination.read_text():
                raise ValueError(f'Canonical facade link is missing: {new}')
    return len(destinations) - len(retired_paths)


def replay_historical(contract):
    checkpoint = contract['historical_move']['cpp_commit']
    archive = git('archive', '--format=tar', checkpoint)
    with tempfile.TemporaryDirectory(prefix='teaql_historical_paths_') as directory:
        with tarfile.open(fileobj=io.BytesIO(archive)) as package:
            for member in package.getmembers():
                if not (member.isfile() or member.isdir()):
                    raise ValueError(f'Unsupported historical archive entry: {member.name}')
                safe_path(Path(directory), member.name)
            package.extractall(directory)
        subprocess.run(['python3', str(Path(directory) / 'scripts/verify_module_paths.py')], check=True)
    print(f'Historical body hashes replayed at C++ {checkpoint}; no current-baseline acceptance inferred')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--historical-checkpoint', action='store_true',
                        help='Also replay the original body-digest verifier on the fixed C++ snapshot')
    args = parser.parse_args()
    contract = load('docs/workspace_paths_5_0_6.json')
    lock = load('upstream.lock.json')
    inventory = load(contract['source_inventory'])
    dispositions = load(contract['source_dispositions'])
    count = verify_source_map(lock, contract, inventory, dispositions)
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run(['python3', str(ROOT / 'scripts/inventory_upstream.py'), '--inventory-only',
                    '--output-dir', str((ROOT / contract['source_inventory']).parent), '--check'], check=True)
    paths = [path for path in git('ls-files', '--cached', '--others', '--exclude-standard', '-z', '--',
                                  *contract['packages']).decode().split('\0') if path]
    production = verify_packages(ROOT, contract['packages'], paths)
    retained = verify_historical_records(contract)
    for package, owner in contract['packages'].items():
        if owner.get('rust_crate') and not (ROOT / '.reference/teaql-rs' / owner['rust_crate']).is_dir():
            raise ValueError(f'Missing fixed reference crate: {owner["rust_crate"]}')
    print(f'Verified {len(contract["packages"])} active packages, {production} source units, '
          f'{count} source ownership/disposition links and {retained} retained move destinations; '
          'structure/provenance only, not unchanged-body or semantic acceptance')
    if args.historical_checkpoint:
        replay_historical(contract)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))
