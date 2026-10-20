#!/usr/bin/env python3
"""Audit the finite typed-batch source disposition without relabeling G15/P7."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'docs/parity'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_receipt(name):
    return json.loads(gzip.decompress((PARITY / f'{name}_5_0_6.json.gz').read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(scope='Complete nineteen-test shared_load_state.rs disposition; wider G15 and P0-P7 open',
                  complete=False, complete_generator_goal=False, steps=[], evidence={})

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        print('Verified', ' '.join(str(x) for x in command[:4]), flush=True)
        return result.stdout

    def evidence(name):
        path = PARITY / f'{name}_5_0_6.json.gz'
        record['evidence'][str(path.relative_to(ROOT))] = digest(path)
        result = read_receipt(name)
        assert result.get('complete') is True, name
        return result

    run(['python3', 'scripts/verify_upstream.py'])
    corpus = json.loads(gzip.decompress((ROOT / 'tests/parity/generated_load_state_bulk_5_0_6_cases.json.gz').read_bytes()))
    assert sum(len(item['rows']) for item in corpus['expected']) == 40404
    for path, expected in corpus['source_hashes'].items():
        assert digest(ROOT / path) == expected, path
    for name, count in [('native', 6), ('module', 6), ('sanitize', 3)]:
        profile = evidence(f'generated_load_state_bulk_{name}')
        assert profile['passed'] == count
        assert all(step['exit_code'] == 0 for step in profile['steps'])
        assert 'no work to do' in profile['steps'][-1]['stdout']
        for path, expected in profile['hashes'].items():
            assert digest(Path(path)) == expected, path
        source_path = Path(profile['actual_source']['path'])
        assert digest(source_path) == profile['actual_source']['sha256']
        record['evidence'][str(source_path.relative_to(ROOT)) if source_path.is_absolute()
                           else str(source_path)] = digest(source_path)
        source = json.loads(gzip.decompress(source_path.read_bytes()))
        assert source['complete'] and source['actual_original_replayed']
        assert source['rows'] == 40404 and source['batches'] == 12
        assert {step['label'] for step in source['steps']} == {'original', 'cpp'}
        for step in source['steps']:
            assert step['exit_code'] == 0
            assert digest(Path(step['binary'])) == step['sha256']
            assert [json.loads(line) for line in step['stdout'].splitlines()] == corpus['expected']
    original = evidence('generated_load_state_bulk_original')
    assert original['exit_code'] == 0
    assert [json.loads(line) for line in original['stdout'].splitlines()] == corpus['expected']

    implementation = evidence('shared_state_source_implementation_review')
    for path, expected in implementation['implementation_hashes'].items():
        assert digest(ROOT / path) == expected, path
    runtime = evidence('typed_member_alias_runtime_acceptance')
    preserved_runtime = {}
    for path, expected in runtime['hashes'].items():
        if '/teaql_runtime/' in path or path.endswith('typed_member_alias_5_0_6.rs'):
            assert digest(Path(path)) == expected, path
            preserved_runtime[path] = expected
    java = evidence('generated_member_alias_java')
    for path, expected in java['source_hashes'].items():
        assert digest(Path(java['cwd']) / path) == expected, path
    sdks = evidence('generated_member_alias_sdk_update')
    assert len(sdks['sdks']) == 78 and len(sdks['protected_artifacts']) == 2201
    for sdk in sdks['sdks']:
        for path, expected in sdk['after'].items():
            assert digest(ROOT / sdk['path'] / path) == expected, path
    for path, expected in sdks['protected_artifacts'].items():
        assert digest(ROOT / path) == expected, path
    evidence('generated_member_alias_acceptance')
    record['preservation'] = dict(implementation_hashes=implementation['implementation_hashes'],
                                  alias_runtime_hashes=preserved_runtime,
                                  producer_root=java['cwd'], producer_hashes=java['source_hashes'], sdk_count=78,
                                  protected_artifacts=2201,
                                  reused='Java80/compiler193/78-SDK reproduction retain their previous execution scope')

    source_path = ROOT / '.reference/teaql-rs/teaql-core/tests/shared_load_state.rs'
    functions = re.findall(r'#\[test\]\s*fn\s+(\w+)\(', source_path.read_text())
    assert len(functions) == 19
    review = (ROOT / 'docs/generated_load_state_bulk_5_0_6.md').read_text()
    assert all(f'`{name}`' in review for name in functions)
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                  '.reference/teaql-rs/teaql-core/Cargo.toml', '--test', 'shared_load_state'], environment)
    assert '19 passed; 0 failed; 0 ignored;' in output
    assert all(f'test {name} ... ok' in output for name in functions)
    record['source_review'] = dict(path=str(source_path.relative_to(ROOT)), sha256=digest(source_path),
                                   tests=functions, disposition='accepted_with_existing_Context_owner_thread_adaptations',
                                   wider_quoted_fk='Five additional quoted-FK outcomes remain unaccepted under partial G15; unquoted test inputs do not exercise them')
    for checkout in ['.reference/teaql-rs', '.reference/teaql_code_gen']:
        assert not run(['git', '-C', checkout, 'status', '--porcelain'])
    assert run(['git', '-C', '/home/philip/githome/teaql_code_gen_cpp', 'branch', '--show-current']).strip() == 'cpp_support'
    run(['python3', 'scripts/verify_module_paths.py'])
    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
    registry = json.loads(run(['ctest', '--preset', 'native', '--show-only=json-v1']))
    assert len(registry['tests']) == 728
    assert {'generated_load_state_bulk_contracts', 'generated_load_state_bulk_rust_parity'} <= {test['name'] for test in registry['tests']}
    record['native_registry_count'] = len(registry['tests'])
    dispositions = json.loads((ROOT / 'docs/parity/v5_0_6/source_dispositions.json').read_text())['sources']
    counts = {status: sum(entry['status'] == status for entry in dispositions)
              for status in ['accepted', 'pending_review']}
    assert counts == {'accepted': 19, 'pending_review': 105}, counts
    entry = next(item for item in dispositions if item['source'] == 'teaql-core/tests/shared_load_state.rs')
    assert entry['status'] == 'accepted' and entry['sha256'] == digest(source_path)
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    assert next(item for item in ledger['capabilities'] if item['id'] == 'G15')['status'] == 'partial'
    assert next(item for item in ledger['capabilities'] if item['id'] == 'G13')['status'] == 'partial'
    record['source_status'] = counts
    formatting = Path('/tmp/teaql_generated_load_state_bulk_format_5_0_6.log')
    assert 'Formatted 1668 handwritten C++ files' in formatting.read_text()
    assert 'Checked 1668 handwritten C++ files' in formatting.read_text()
    record['formatting'] = dict(log=formatting.read_text(), sha256=digest(formatting))
    paths = ['docs/generated_load_state_bulk_5_0_6.md', 'docs/cpp_generator_parity_ledger.json',
             'docs/parity/v5_0_6/source_dispositions.json', 'docs/shared_state_source_tests_5_0_6.md',
             'docs/shared_load_state_requirements_5_0_6.md', 'docs/shared_load_state_acceptance_5_0_6.md',
             'docs/baseline_5_0_6_upgrade.md', 'docs/semantic_qualification_requirements_5_0_6.md',
             'docs/native_integration_5_0_6.md', 'docs/cpp_generator_parity_plan.md',
             'docs/generated_member_aliases_5_0_6.md', 'tests/CMakeLists.txt', 'modules/CMakeLists.txt',
             'modules/contracts.json', 'CMakePresets.json', str(Path(__file__).relative_to(ROOT))]
    record['hashes'] = {path: digest(ROOT / path) for path in paths}
    previous = PARITY / 'generated_load_state_bulk_previous_disposition_5_0_6.json.gz'
    record['previous_disposition'] = dict(path=str(previous.relative_to(ROOT)), sha256=digest(previous))
    record['complete'] = True
    save()
    print('Accepted19 complete source-test responsibilities; source19/105; G15/G13/P0-P7 remain open')


if __name__ == '__main__':
    main()
