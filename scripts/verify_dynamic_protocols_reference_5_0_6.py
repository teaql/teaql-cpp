#!/usr/bin/env python3
"""Replay complete retained G16 observers with exact reference dependencies."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import observe, same

FAMILIES = ('dynamic_fields', 'dynamic_storage', 'dynamic_ledger',
            'dynamic_graph_debug', 'dynamic_governance', 'dynamic_query', 'dynamic_relation')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--cpp-prefix', default='teaql_')
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--save-audit', action='store_true',
                        help='Replay complete retained durable Save and audit corpora')
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve evidence; select a fresh record'
    project = PARITY / ('rust_dynamic_save_reference_5_0_6' if args.save_audit else
                       'rust_dynamic_protocols_reference_5_0_6')
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected_versions = versions(reference_lock)
    local = {'teaql_dynamic_protocols_reference_506', 'school-management-service-core',
             'teaql_dynamic_save_reference_506', 'cpp-dynamic-json-core'}
    for name, actual in versions(project / 'Cargo.lock').items():
        if name not in local and not actual <= expected_versions.get(name, set()):
            raise RuntimeError(f'Observer dependency differs: {name}: {actual}')
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference_lock)
    scope = ('Complete unchanged G16 durable Save and private audit projection corpora.'
             if args.save_audit else
             'Seven complete unchanged G16 definition/SPI/ledger/graph/governance/'
             'root/relation observer corpora.')
    record = dict(scope=scope + ' Full capability and combined integration remain separate requirements.',
                  runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  steps=[], matrices=[], hashes={}, complete=False,
                  reference_registry_versions_preserved=True,
                  reference_registry_checksums_preserved=True)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, ensure_ascii=False).encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    for command in [['python3', 'scripts/verify_upstream.py'],
                    ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                     str(project / 'Cargo.toml')]]:
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    output=result.stdout))
        save()
        result.check_returncode()
    paths = {Path(__file__), project / 'Cargo.toml', project / 'Cargo.lock', reference_lock,
             PARITY / 'rust_values/src/fixture_value.rs'}
    families = ('dynamic_save_audit', 'dynamic_typed_save_audit', 'dynamic_audit_projection') if args.save_audit else FAMILIES
    for family in families:
        observer = {'dynamic_relation': 'dynamic_query',
                    'dynamic_typed_save_audit': 'dynamic_save_audit'}.get(family, family)
        corpus_path = PARITY / (family + '_5_0_6_cases.json')
        if not corpus_path.exists():
            corpus_path = corpus_path.with_suffix('.json.gz')
        corpus = json.loads(gzip.decompress(corpus_path.read_bytes())
                            if corpus_path.suffix == '.gz' else corpus_path.read_text())
        assert corpus['baseline'] == record['runtime_commit']
        cases = [case['input'] for case in corpus['cases']]
        retained = [case['expected'] for case in corpus['cases']]
        rust = Path(environment['CARGO_TARGET_DIR']) / 'debug' / ('teaql_' + observer + '_reference_506')
        cpp = args.cpp_dir.resolve() / (args.cpp_prefix + observer + '_probe')
        executable_hashes = {str(path): digest(path) for path in (rust, cpp)}
        original = observe(str(rust), cases)
        current = observe(str(cpp), cases)
        differences = [i for i, (a, b) in enumerate(zip(original, current)) if not same(a, b)]
        retained_differences = [i for i, (a, b) in enumerate(zip(original, retained)) if not same(a, b)]
        record['matrices'].append(dict(family=family, inputs=cases, original=original,
                                       cpp=current, differences=differences,
                                       retained_differences=retained_differences,
                                       executable_hashes=executable_hashes))
        save()
        for path, expected in executable_hashes.items():
            assert digest(Path(path)) == expected, path
        assert not differences and not retained_differences, family
        paths.update((corpus_path, rust, cpp,
                      PARITY / 'rust_field_layout/src/bin' / (observer + '_5_0_6.rs'),
                      PARITY / (observer + '_probe.cpp')))
        print(family, len(cases), 'exact original/native observations', flush=True)
    review = json.loads(gzip.decompress((ROOT / 'docs/parity/g16_current_responsibility_review_5_0_6.json.gz').read_bytes()))
    for source in list(review['primary_sources']) + list(review['transitive_sources']):
        paths.add(ROOT / '.reference/teaql-rs' / source)
    if args.save_audit:
        paths.update((ROOT / 'tests/support/dynamic_json_schema.sql',
                      ROOT / 'tests/support/dynamic_json_dataset.sql'))
        paths.update((ROOT / 'tests/generator/parity/dynamic_json_rust/lib').rglob('*.rs'))
        paths.update((ROOT / '.reference/teaql-rs/teaql-runtime/src/event.rs',
                      ROOT / '.reference/teaql-rs/teaql-runtime/src/log_privacy.rs'))
    record['hashes'] = {str(path): digest(path) for path in sorted(paths)}
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
