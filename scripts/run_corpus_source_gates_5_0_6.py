#!/usr/bin/env python3
"""Run all 27 source-bound comparisons and their permanent source/negative checks."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    names = ['corpus_source_contracts', 'resolved_corpus_source_contracts',
             'composite_corpus_source_contracts', 'remaining_corpus_source_contracts']
    registry_paths = [ROOT / 'tests/parity' / (n + '_5_0_6.json') for n in names]
    registries = [json.loads(p.read_text()) for p in registry_paths]
    contracts = [c for r in registries for c in r['contracts']]
    assert len(contracts) == 27 and sum(c['count'] for c in contracts) == 58920
    prefixes = ['parity_corpus_source', 'parity_resolved_corpus_source',
                'parity_composite_corpus_source', 'parity_remaining_corpus_source']
    expected = [c['native_test'] for c in contracts] + [p + s for p in prefixes for s in ('_contracts', '_controls')]
    paths = {Path(__file__), ROOT / 'tests/CMakeLists.txt', *registry_paths,
             ROOT / 'scripts/verify_corpus_source_contracts_5_0_6.py', ROOT / 'scripts/corpus_source_assets_5_0_6.py',
             ROOT / 'scripts/verify_remaining_corpus_source_contracts_5_0_6.py',
             ROOT / 'scripts/replay_remaining_corpus_sources_5_0_6.py', ROOT / 'scripts/corpus_input_contracts_5_0_6.py',
             ROOT / 'tests/parity/verify_corpus_source_controls_5_0_6.py',
             ROOT / 'tests/parity/verify_composite_corpus_source_controls_5_0_6.py',
             ROOT / 'tests/parity/verify_remaining_corpus_source_controls_5_0_6.py',
             ROOT / 'docs/parity/remaining_corpus_source_execution_5_0_6.json.gz'}
    paths.update(ROOT / c[key] for c in contracts for key in ('corpus', 'receipt', 'producer', 'observer', 'input_generator') if key in c)
    paths.update(ROOT / name for c in contracts for key in ('asset_versions', 'input_files', 'compile_inputs') for name in c.get(key, {}))
    hashes = lambda: {str(p): digest(p) for p in sorted(paths)}
    record = dict(complete=False, whole_goal_complete=False, steps=[], expected_tests=expected,
                  input_hashes_before=hashes())
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1')
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_all_corpus_source_gates_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            registry = json.loads(subprocess.check_output(['ctest', '--preset', 'native', '--show-only=json-v1'],
                                                         cwd=ROOT, text=True, env=environment))
            record['registered_tests'] = len(registry['tests'])
            tests = {t['name']: t for t in registry['tests']}
            assert set(expected) <= set(tests)
            for cohort, fixture in zip(registries, ['context_corpus_source', 'resolved_corpus_source',
                                                   'composite_corpus_source', 'remaining_corpus_source']):
                for c in cohort['contracts']:
                    properties = {p['name']: p['value'] for p in tests[c['native_test']]['properties']}
                    assert fixture in properties['FIXTURES_REQUIRED']
            focused = ['generated_support_source_rust_parity', 'row_constructor_rust_parity', 'schema_dynamic_source_rust_parity']
            for label, selection, required in [('affected', expected, expected),
                    ('focused', focused, ['parity_remaining_corpus_source_contracts', *focused])]:
                junit = owned / (label + '.xml')
                command = ['ctest', '--preset', 'native', '--output-on-failure', '--no-tests=error',
                           '--output-junit', str(junit), '-R', '^(' + '|'.join(selection) + ')$']
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, env=environment)
                step = dict(command=command, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)
                record['steps'].append(step)
                result.check_returncode()
                tree = ET.parse(junit).getroot()
                actual = [t.attrib['name'] for t in tree.findall('.//testcase')]
                assert sorted(actual) == sorted(required), (actual, required)
                assert not any(tree.findall('.//' + tag) for tag in ('failure', 'error', 'skipped'))
                step.update(tests=actual, junit=junit.read_text())
                print(label, len(actual), 'passed', flush=True)
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = hashes()
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))


if __name__ == '__main__':
    main()
