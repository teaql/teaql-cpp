#!/usr/bin/env python3
"""Replay complete core primitive evidence without relabeling historical corpora."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import observe, same

FILES = ('business_id', 'entity_graph', 'naming', 'time', 'trace')
MATRICES = (
    ('business_id', 'business_id_5_0_6_cases.json.gz'),
    ('core_api', 'core_api_5_0_6_cases.json.gz'),
    ('core_aux_debug', 'core_aux_debug_5_0_6_cases.json.gz'),
    ('core_graph_api', 'core_graph_api_baseline_5_0_6_cases.json.gz'),
    ('timestamp_api', 'timestamp_api_baseline_5_0_6_cases.json.gz'),
    ('chrono_value', 'chrono_value_baseline_5_0_6_cases.json.gz'),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()) if path.suffix == '.gz' else path.read_text())


def new_inputs():
    names = ['', 'A', 'Z', 'a', 'Entity', 'HTTPServer', 'URL', 'customerID',
             '_Entity', '__', 'Entity_', '42Name', '42', 'Order-Line', 'two words',
             'a.b/C', 'éName', 'ÄName', '雪A', '😀B', 'a\u0301B', '\0Z',
             '\u0085U\u2028', 'ΣName', '𝔄Name', 'a' * 1024 + 'Z', 'A' * 256]
    result = [dict(mode='name', name=name) for name in names]
    for kind in range(9):
        for identity in [None, '0', '18446744073709551615']:
            for name, comment in [('', ''), ('Order', 'read'), ('雪😀', 'reason\n\0'),
                                  ('\u0085\u2028', '\u2029'), ('e' * 256, 'c' * 4096)]:
                result.append(dict(mode='trace', kind=str(kind), name=name,
                                   id=identity, comment=comment))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust-dir', type=Path, default=Path('/tmp/teaql_cpp_core_baselines_506/debug'))
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--cpp-prefix', default='teaql_')
    parser.add_argument('--record-new-corpus', action='store_true')
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    if args.receipt.exists():
        parser.error('Receipt already exists; use a fresh path')
    reference = ROOT / '.reference/teaql-rs'
    lock = read(ROOT / 'upstream.lock.json')
    actual = subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'], text=True).strip()
    assert lock['version'] == '5.0.6' and actual == lock['commit']
    observer = PARITY / 'rust_core_primitives_reference_5_0_6'
    assert registry_identities(observer / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock')
    record = dict(scope='Complete source primitive group replay; not runtime business-ID service, graph Save, query trace orchestration or G13/P7 acceptance',
                  complete=False, baseline=lock, source_files={}, matrices=[], drivers={}, inputs={})
    for stem in FILES:
        source = f'teaql-core/src/{stem}.rs'
        current = (reference / source).read_bytes()
        previous = subprocess.check_output(['git', '-C', str(reference), 'show',
                                            '62940ae2b88e7acc32763853b8b4685a01c7e90a:' + source])
        assert current == previous, source
        record['source_files'][source] = dict(sha256=hashlib.sha256(current).hexdigest(),
                                              unchanged=True, complete_source=current.decode())
    for stem, filename in MATRICES:
        corpus_path = PARITY / filename
        corpus = read(corpus_path)
        assert corpus['baseline'] == lock['commit']
        inputs = [case['input'] for case in corpus['cases']]
        expected = [case['expected'] for case in corpus['cases']]
        rust = args.rust_dir / (stem + '_oracle')
        cpp = args.build_dir / (args.cpp_prefix + stem + '_probe')
        original = observe(str(rust), inputs)
        native = observe(str(cpp), inputs)
        assert same(expected, original) and same(original, native), stem
        driver = PARITY / 'rust_values/src/bin' / (stem + '_oracle.rs')
        record['drivers'][str(driver.relative_to(ROOT))] = dict(sha256=digest(driver), source=driver.read_text())
        record['matrices'].append(dict(name=stem, observations=len(inputs), corpus=str(corpus_path.relative_to(ROOT)),
                                       corpus_sha256=digest(corpus_path), rust_sha256=digest(rust),
                                       cpp_sha256=digest(cpp), outputs=original))
        record['inputs'][stem] = inputs
        print(f'{stem}: {len(inputs)}/{len(inputs)} exact source/native observations', flush=True)
    inputs = new_inputs()
    original = observe(str(args.rust_dir / 'teaql_core_primitives_reference_506'), inputs)
    native = observe(str(args.build_dir / (args.cpp_prefix + 'core_primitives_probe')), inputs)
    assert same(original, native), 'Naming/Trace actual calls differ'
    corpus_path = PARITY / 'core_primitives_5_0_6_cases.json.gz'
    corpus = dict(baseline=lock['commit'], notice='Actual locked naming/Trace calls; exact JSON results, no normalization',
                  cases=[dict(input=value, expected=output) for value, output in zip(inputs, original)])
    if args.record_new_corpus:
        if corpus_path.exists():
            parser.error('The new corpus already exists; do not overwrite evidence')
        corpus_path.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    else:
        assert same(read(corpus_path), corpus)
    record['matrices'].append(dict(name='naming_trace', observations=len(inputs), outputs=original,
                                   corpus_sha256=digest(corpus_path),
                                   rust_sha256=digest(args.rust_dir / 'teaql_core_primitives_reference_506'),
                                   cpp_sha256=digest(args.build_dir / (args.cpp_prefix + 'core_primitives_probe'))))
    record['inputs']['naming_trace'] = inputs
    for driver in [observer / 'src/main.rs', PARITY / 'core_primitives_probe.cpp',
                   PARITY / 'rust_values/src/fixture_value.rs']:
        record['drivers'][str(driver.relative_to(ROOT))] = dict(sha256=digest(driver), source=driver.read_text())
    record.update(complete=True, manifest=(observer / 'Cargo.toml').read_text(),
                  resolved_lock=(observer / 'Cargo.lock').read_text(),
                  reference_lock_sha256=digest(reference / 'Cargo.lock'),
                  observations=sum(row['observations'] for row in record['matrices']))
    args.receipt.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))
    print(f'Complete replay: {record["observations"]} exact observations')


if __name__ == '__main__':
    main()
