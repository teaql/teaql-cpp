#!/usr/bin/env python3
"""Verify original allocation-test obligations and current scoped C++ measurements."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import observe, same


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def geometry():
    result = set()
    for name, widths in [('cold_projection', (4, 64, 130)),
                         ('same_shape_merge', (3, 64, 141)),
                         ('flat_edge_shape', (4,)),
                         ('cached_projection', (4, 64, 130)),
                         ('loaded_fixed_noop', (4, 64, 130)),
                         ('loaded_dynamic_noop', (4, 64, 130)),
                         ('reference_dynamic_noop', (4, 64, 130))]:
        result.update(itertools.product([name], widths, (1, 100, 10000)))
    return result


def measurements(output, original):
    rows = {}
    for line in output.split('\n'):
        if original:
            # libtest can print its status prefix before a test's first println.
            line = re.sub(r'^test [a-z_]+ \.\.\. ', '', line, count=1)
        if original and line.startswith('COMPACT_SAME_SHAPE_MERGE,'):
            match = re.fullmatch(r'COMPACT_SAME_SHAPE_MERGE,width=(\d+),rows=(\d+),calls=(\d+),bytes=(\d+)', line)
            if not match:
                raise RuntimeError('Malformed original same-shape measurement')
            width, size, calls, size_bytes = map(int, match.groups())
            key = ('same_shape_merge', width, size)
            value = dict(requests=calls, bytes=size_bytes)
        else:
            parts = line.split(',')
            if len(parts) != (6 if original else 8) or parts[0] not in {x[0] for x in geometry()} | {'positive_control'}:
                continue
            key = (parts[0], int(parts[1]), int(parts[2]))
            value = dict(requests=int(parts[3]), bytes=int(parts[4]))
            if not original:
                value.update(heap_calls=int(parts[5]), heap_bytes=int(parts[6]))
        if key in rows:
            raise RuntimeError(f'Duplicate measurement: {key}')
        rows[key] = value
    expected = geometry() if original else geometry() | {('positive_control', 128, 1)}
    if rows.keys() != expected:
        raise RuntimeError(f'Incomplete measurement geometry: {rows.keys() ^ expected}')
    for key, value in rows.items():
        if key[0] in {'same_shape_merge', 'cached_projection', 'loaded_fixed_noop', 'loaded_dynamic_noop'}:
            if any(value.values()):
                raise RuntimeError(f'Allocation in required zero-allocation branch: {key}: {value}')
    for name, widths in [('cold_projection', (4, 64, 130)), ('flat_edge_shape', (4,))]:
        for width in widths:
            values = [rows[(name, width, size)] for size in (1, 100, 10000)]
            baseline = (values[0]['requests'], values[0]['bytes'])
            if not all((v['requests'], v['bytes']) == baseline for v in values):
                raise RuntimeError(f'Metadata cost grows with row count: {name}/{width}')
            if name == 'cold_projection' and baseline[0] == 0:
                raise RuntimeError('Cold fixture was already warm')
            if name == 'flat_edge_shape' and baseline[0] > 10:
                raise RuntimeError('Flat geometry exceeded the original bounded allocation count')
    if not original:
        control = rows[('positive_control', 128, 1)]
        if control['requests'] < 1 or control['bytes'] < 128 or control['heap_calls'] < 1 or control['heap_bytes'] < 128:
            raise RuntimeError('Counters did not observe actual allocation controls')
    return [dict(case=name, width=width, iterations=count, **value)
            for (name, width, count), value in sorted(rows.items())]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-dir', type=Path, default=ROOT / 'build_modules_native/modules')
    parser.add_argument('--rust-dir', type=Path, default=Path('/tmp/teaql_cpp_core_baselines_506/debug'))
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    if args.receipt.exists():
        parser.error('Preserve previous receipts; select a fresh path')
    record = dict(scope='Entire original load_state_allocations.rs: four test bodies, 57 cases, '
                       'counter controls and adjacent CompactRow semantic regression; not full G13/P7',
                  complete=False, steps=[], measurements={}, semantics={})

    def save():
        args.receipt.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                capture_output=True, text=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                     stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run([sys.executable, ROOT / 'scripts/verify_upstream.py'])
    lock = read(ROOT / 'upstream.lock.json')
    if lock['version'] != '5.0.6':
        raise RuntimeError('This source qualification requires 5.0.6')
    record['baseline'] = lock
    witness = read(ROOT / 'docs/parity/load_state_allocation_initial_5_0_6.json.gz')
    current = (ROOT / 'teaql_core/src/row.cpp').read_text()
    start = current.index('    const auto* layout = shared_columns();',
                          current.index('CompactRow::try_extend('))
    end = current.index('    Vector<String> names = copy_names(r, keys());', start)
    branch = current[start:end]
    restored = current[:start] + current[end:]
    marker = '    if (!std::ranges::equal(names, keys()) ||'
    if restored.count(marker) != 1:
        raise RuntimeError('Cannot isolate the exact merge repair')
    restored = restored.replace(marker,
        '    const auto* layout = shared_columns();\n'
        '    const bool same_schema = left == definitions || (left && definitions && *left == *definitions);\n'
        + marker, 1)
    original_hash = hashlib.sha256(restored.encode()).hexdigest()
    if original_hash != witness['hashes']['teaql_core/src/row.cpp']:
        raise RuntimeError('An unrelated CompactRow implementation body changed')
    record['repair_delta'] = dict(pre_repair_sha256=original_hash,
                                  current_sha256=digest(ROOT / 'teaql_core/src/row.cpp'),
                                  added_branch=branch,
                                  disposition='Only the new validated branch and two relocated declarations differ')
    source = ROOT / '.reference/teaql-rs/teaql-core/tests/load_state_allocations.rs'
    disposition = next(x for x in read(ROOT / 'docs/parity/v5_0_6/source_dispositions.json')['sources']
                       if x['source'] == 'teaql-core/tests/load_state_allocations.rs')
    if digest(source) != disposition['sha256']:
        raise RuntimeError('The complete original allocation-test file changed')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_dir.parent),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                  ROOT / '.reference/teaql-rs/teaql-core/Cargo.toml', '--test',
                  'load_state_allocations', '--', '--nocapture', '--test-threads=1'], environment)
    if '4 passed; 0 failed; 0 ignored; 0 measured; 0 filtered out' not in output:
        raise RuntimeError('Original allocation suite did not execute all four tests')
    record['measurements']['original'] = measurements(output, True)
    for name, directory, prefix in [('header', args.native_dir, 'teaql_'),
                                     ('module', args.module_dir, 'teaql_module_')]:
        binary = directory / (prefix + 'load_state_allocation_tests')
        output = run([binary.resolve()])
        record['measurements'][name] = measurements(output, False)
        record.setdefault('binary_hashes', {})[str(binary)] = digest(binary)
        print(f'{name}: 57 complete allocation cases and dual-counter positive control', flush=True)
    corpus_path = PARITY / 'compact_row_api_5_0_6_cases.json.gz'
    corpus = read(corpus_path)
    if corpus['baseline'] != lock['commit'] or len(corpus['cases']) != 401:
        raise RuntimeError('CompactRow corpus baseline or geometry changed')
    project = PARITY / 'rust_core_primitives_reference_5_0_6'
    if not registry_identities(project / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock'):
        raise RuntimeError('CompactRow source observer dependencies differ from the reference')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
         project / 'Cargo.toml', '--bin', 'compact_row_api_oracle'], environment)
    inputs = [case['input'] for case in corpus['cases']]
    original_binary = args.rust_dir / 'compact_row_api_oracle'
    original = observe(str(original_binary), inputs)
    if not same(original, [case['expected'] for case in corpus['cases']]):
        raise RuntimeError('Actual CompactRow source differs from retained observations')
    record['semantics']['original'] = original
    record.setdefault('binary_hashes', {})[str(original_binary)] = digest(original_binary)
    for name, directory, prefix in [('header', args.native_dir, 'teaql_'),
                                     ('module', args.module_dir, 'teaql_module_')]:
        binary = directory / (prefix + 'compact_row_api_probe')
        actual = observe(str(binary), inputs)
        record['semantics'][name] = actual
        record['binary_hashes'][str(binary)] = digest(binary)
        save()
        if not same(original, actual):
            raise RuntimeError(f'CompactRow value/layout semantics changed: {name}')
        print(f'{name}: 401/401 actual source CompactRow observations', flush=True)
    paths = [source, ROOT / '.reference/teaql-rs/teaql-core/src/query.rs',
             ROOT / '.reference/teaql-rs/Cargo.lock', project / 'Cargo.toml', project / 'Cargo.lock',
             PARITY / 'rust_values/src/bin/compact_row_api_oracle.rs', corpus_path,
             ROOT / 'teaql_memory/include/teaql/memory/arena.hpp',
             ROOT / 'teaql_core/src/row.cpp', ROOT / 'teaql_core/tests/load_state_allocation_tests.cpp',
             ROOT / 'docs/parity/load_state_allocation_initial_5_0_6.json.gz', Path(__file__)]
    record['hashes'] = {str(path.relative_to(ROOT)): digest(path) for path in paths}
    record['allocation_comparison'] = ('Preserve zero-request/hot and constant-cost/cold obligations, '
        'including the ten-request flat-shape ceiling. Arena bookkeeping and Rust heap allocation '
        'have distinct byte/call totals; report both directly. No timing equality or speedup claim.')
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
