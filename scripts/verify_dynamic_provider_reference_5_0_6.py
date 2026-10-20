#!/usr/bin/env python3
"""Replay the unchanged provider observer with exact reference dependencies."""
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
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, default=ROOT / 'build_native_current/teaql_dynamic_provider_probe')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve prior observations; use a fresh record'
    project = ROOT / 'tests/parity/rust_dynamic_provider_reference_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    expected = versions(reference)
    for name, actual in versions(project / 'Cargo.lock').items():
        if name != 'teaql_dynamic_provider_reference_506' and not actual <= expected.get(name, set()):
            raise RuntimeError(f'Provider oracle dependency differs: {name}: {actual}')
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    record = dict(scope='Complete unchanged original/native72 provider observations with '
                        'original dependency versions/checksums; not full G16 acceptance',
                  steps=[], complete=False, reference_registry_versions_preserved=True,
                  reference_registry_checksums_preserved=True)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, ensure_ascii=False).encode(), mtime=0))

    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    for command in [['python3', 'scripts/verify_upstream.py'],
                    ['cargo', 'build', '--offline', '--locked', '--manifest-path', str(project / 'Cargo.toml')]]:
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        record['steps'].append(dict(command=command, exit_code=result.returncode, output=result.stdout))
        save()
        result.check_returncode()
    corpus_path = ROOT / 'tests/parity/dynamic_provider_5_0_6_cases.json'
    corpus = json.loads(corpus_path.read_text())
    record['runtime_commit'] = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert corpus['baseline'] == record['runtime_commit']
    cases = [c['input'] for c in corpus['cases']]
    expected = [c['expected'] for c in corpus['cases']]
    rust = Path(environment['CARGO_TARGET_DIR']) / 'debug/teaql_dynamic_provider_reference_506'
    record['original'] = observe(str(rust), cases)
    record['cpp'] = observe(str(args.cpp.resolve()), cases)
    record['differences'] = [i for i, (a, b) in enumerate(zip(record['original'], record['cpp'])) if not same(a, b)]
    record['retained_differences'] = [i for i, (a, b) in enumerate(zip(expected, record['original'])) if not same(a, b)]
    record['inputs'] = cases
    paths = [Path(__file__), project / 'Cargo.toml', project / 'Cargo.lock', reference,
             ROOT / 'tests/parity/rust_field_layout/src/bin/dynamic_provider_5_0_6.rs',
             ROOT / 'tests/parity/rust_values/src/fixture_value.rs', corpus_path,
             ROOT / 'tests/parity/dynamic_provider_probe.cpp', args.cpp, rust]
    paths.extend(ROOT / '.reference/teaql-rs' / p for p in corpus['source'])
    record['hashes'] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    record['complete'] = not record['differences'] and not record['retained_differences']
    save()
    print(f"Provider source replay: {len(cases)} cases, {len(record['differences'])} native "
          f"differences, {len(record['retained_differences'])} retained-corpus differences")
    assert record['complete']


if __name__ == '__main__':
    main()
