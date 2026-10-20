#!/usr/bin/env python3
"""Qualify the two core metadata test files; retain the independent typed-alias gap."""
import argparse
import difflib
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
TESTS = {'dynamic_field_state': 11, 'dynamic_property_metadata': 3, 'shared_load_state': 19}


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, default=ROOT / 'build_native_current/teaql_shared_state_source_tests')
    parser.add_argument('--module', type=Path, default=ROOT / 'build_modules_native/modules/teaql_module_shared_state_source_tests')
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    if args.receipt.exists():
        parser.error('Use a fresh receipt; preserve historical evidence')
    record = dict(scope='Whole dynamic_field_state.rs and dynamic_property_metadata.rs test responsibilities; '
                       'shared_load_state.rs is reviewed but remains pending for actual three-name typed decoding',
                  complete=False, steps=[], source_files={}, cpp_deltas={})

    def save():
        args.receipt.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, text=True,
                                env=environment, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                     stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    lock = read(ROOT / 'upstream.lock.json')
    if lock['version'] != '5.0.6':
        raise RuntimeError('This audit requires the locked 5.0.6 source')
    record['runtime_commit'] = lock['commit']
    dispositions = {x['source']: x for x in read(ROOT / 'docs/parity/v5_0_6/source_dispositions.json')['sources']}
    for name, count in TESTS.items():
        relative = f'teaql-core/tests/{name}.rs'
        path = ROOT / '.reference/teaql-rs' / relative
        source = path.read_text()
        names = re.findall(r'#\[test\]\s*fn ([a-z_0-9]+)\(', source)
        if len(names) != count or digest(path) != dispositions[relative]['sha256']:
            raise RuntimeError(f'Original file/hash/test geometry changed: {relative}')
        record['source_files'][relative] = dict(sha256=digest(path), complete_source=source,
                                               tests=names, accepted_by_this_scope=name != 'shared_load_state')
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    command = ['cargo', 'test', '--offline', '--locked', '--manifest-path',
               ROOT / '.reference/teaql-rs/teaql-core/Cargo.toml']
    for name in TESTS:
        command.extend(['--test', name])
    output = run(command, environment)
    for name, count in TESTS.items():
        for test in record['source_files'][f'teaql-core/tests/{name}.rs']['tests']:
            if f'test {test} ... ok' not in output:
                raise RuntimeError(f'Original test did not pass: {test}')
        if f'{count} passed; 0 failed; 0 ignored; 0 measured; 0 filtered out' not in output:
            raise RuntimeError(f'Original suite incomplete: {name}')
    for label, binary in [('header', args.native), ('module', args.module)]:
        output = run([binary.resolve()])
        for count in (1, 100, 10000):
            if output.count(f'bulk_dynamic_rows={count}\n') != 1:
                raise RuntimeError(f'Incomplete native bulk matrix: {label}/{count}')
        match = re.search(r'selection_handle_bytes=(\d+) pointer_bytes=(\d+)', output)
        if not match:
            raise RuntimeError('Selection representation observation omitted')
        record.setdefault('native_observations', {})[label] = dict(
            binary=str(binary), sha256=digest(binary), handle_bytes=int(match[1]),
            pointer_bytes=int(match[2]), bulk_sizes=[1, 100, 10000])
        print(label, 'dictionary, selection, readonly and complete bulk contracts pass', flush=True)
    initial = read(ROOT / 'docs/parity/shared_state_source_initial_5_0_6.json.gz')
    for relative, previous in initial['production'].items():
        path = ROOT / relative
        record['cpp_deltas'][relative] = dict(before_sha256=previous['sha256'], current_sha256=digest(path),
            diff=''.join(difflib.unified_diff(previous['source'].splitlines(True), path.read_text().splitlines(True))))
    old_arena = read(ROOT / 'docs/parity/shared_state_source_sanitize_timeout_5_0_6.json.gz')['arena_source']
    path = ROOT / 'teaql_memory/src/arena.cpp'
    record['cpp_deltas']['teaql_memory/src/arena.cpp'] = dict(
        before_sha256=hashlib.sha256(old_arena.encode()).hexdigest(), current_sha256=digest(path),
        diff=''.join(difflib.unified_diff(old_arena.splitlines(True), path.read_text().splitlines(True))))
    gap = read(ROOT / 'docs/parity/shared_state_three_name_alias_gap_final_5_0_6.json.gz')
    if gap['compile']['exit_code'] != 0 or gap['observation']['exit_code'] != 1:
        raise RuntimeError('The documented independent alias gap is not supported by its actual witness')
    record['pending_alias_gap'] = gap['observation']
    record['adaptations'] = [
        'Context-owned immutable payloads and explicit destination copies replace Arc ownership; no refcount extends Context lifetime.',
        'The two-word native ScopedBox retains its existing type; it is not the one-word Rust Option<Arc> ABI.',
        'The registered metadata dictionary consumes one managed identity; backing strings/map nodes do not each consume one.',
        'Raw get()/release() on the Context-managed selection yields a borrow, not ownership of its registered payload.',
    ]
    paths = [Path(__file__), ROOT / 'teaql_core/tests/shared_state_source_tests.cpp',
             ROOT / '.reference/teaql-rs/Cargo.lock', ROOT / 'docs/shared_state_source_tests_5_0_6.md',
             ROOT / 'docs/parity/shared_state_source_initial_5_0_6.json.gz',
             ROOT / 'docs/parity/shared_state_source_sanitize_initial_5_0_6.json.gz',
             ROOT / 'docs/parity/shared_state_source_sanitize_timeout_5_0_6.json.gz',
             ROOT / 'docs/parity/shared_state_three_name_alias_gap_final_5_0_6.json.gz']
    record['hashes'] = {str(path.relative_to(ROOT)): digest(path) for path in paths}
    record['original_tests_passed'] = 33
    record['source_files_qualified_by_this_scope'] = 2
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
