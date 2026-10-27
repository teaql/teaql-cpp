#!/usr/bin/env python3
"""Bind registered corpora to archived original execution and current native output."""
import argparse
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, observe, same
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
from corpus_source_assets_5_0_6 import generate_inputs, render_observer, verify_assets


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def expected_manifest(package):
    assert package['version'] == '5.0.6' and package['edition'] == '2024'
    assert package['features'] == {'opentelemetry': ['dep:opentelemetry']}
    result = '[package]\nname="teaql-runtime"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
    omitted = []
    for kind, heading in [(None, 'dependencies'), ('dev', 'dev-dependencies')]:
        result += '\n[' + heading + ']\n'
        for dependency in package['dependencies']:
            if dependency['kind'] != kind:
                continue
            if kind == 'dev' and dependency['name'] != 'teaql-macros':
                omitted.append(dependency['name'])
                continue
            assert dependency['target'] is None
            fields = ['version=' + json.dumps(dependency['req'])]
            for key in ('path', 'features', 'optional'):
                if dependency.get(key):
                    fields.append(key + '=' + json.dumps(dependency[key]))
            if not dependency.get('uses_default_features', True):
                fields.append('default-features=false')
            result += dependency['name'] + '={' + ','.join(fields) + '}\n'
    assert set(omitted) == {'opentelemetry_sdk', 'opentelemetry-otlp'}
    return result + '\n[features]\nopentelemetry=["dep:opentelemetry"]\n'


def validate_source(contract, receipt, corpus, directory):
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert receipt['complete'] is True and receipt['unchanged_inputs'] is True
    assert receipt['references']['teaql-rs'] == corpus['baseline'] == baseline
    assert receipt['references']['teaql_code_gen'] == json.loads(
        (ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']
    assets = receipt['reproduction_sources']
    composite = 'asset_versions' in contract
    if composite:
        verify_assets(contract, receipt)
    else:
        for key in ('producer', 'observer', 'input_generator'):
            name = contract[key]
            snapshot_hash = hashlib.sha256(assets[name].encode()).hexdigest()
            assert receipt['inputs']['runtime:' + name] == snapshot_hash, name
            if key != 'observer' or contract.get('observer_version', 'current') == 'current':
                assert assets[name] == (ROOT / name).read_text(), name
    observer_version = contract.get('observer_version', 'current')
    assert observer_version in ('current', 'archived')
    template = assets[contract['observer']]
    template_hash = hashlib.sha256(template.encode()).hexdigest()
    if observer_version == 'archived':
        assert template != (ROOT / contract['observer']).read_text()
    if 'observer_snapshot_sha256' in contract or observer_version == 'archived':
        assert template_hash == contract['observer_snapshot_sha256']
    helper = ROOT / 'scripts/reference_runtime_view_5_0_6.py'
    assert receipt['inputs']['runtime:scripts/reference_runtime_view_5_0_6.py'] == digest(helper)
    if composite:
        generated = generate_inputs(contract, receipt)
    else:
        spec = importlib.util.spec_from_file_location(contract['name'] + '_source_inputs', ROOT / contract['input_generator'])
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        generated = [module.inputs()]
    for inputs in generated:
        assert type(corpus['cases']) is list and len(corpus['cases']) == len(inputs) == contract['count']
        assert same(inputs, [c['input'] for c in corpus['cases']])
    view = receipt['original']['runtime_view']
    assert view['all_original_bodies_preserved'] is True
    assert set(view['omitted_dev_dependencies']) == {'opentelemetry_sdk', 'opentelemetry-otlp'}
    source_root = ROOT / '.reference/teaql-rs/teaql-runtime'
    actual_files = {str(p.relative_to(source_root)): digest(p)
                    for folder in ('src', 'tests', 'test-vectors')
                    for p in (source_root / folder).rglob('*') if p.is_file()}
    assert actual_files == view['source_files_sha256']
    observer_target = contract.get('observer_target', 'src/data_service/context.rs')
    assert set(view['observers']) == {observer_target}
    observer = view['observers'][observer_target]
    assert hashlib.sha256(observer['text'].encode()).hexdigest() == observer['sha256']
    assert template.count(contract['marker']) == 1
    assembly = None
    if composite:
        embedded_path, assembly = render_observer(contract, receipt, observer)
    else:
        before, after = template.split(contract['marker'])
        assert observer['text'].startswith(before) and observer['text'].endswith(after)
        embedded_path = observer['text'][len(before):len(observer['text']) - len(after)]
    assert Path(embedded_path).is_absolute() and Path(embedded_path).name == 'inputs.json'
    lock = directory / 'Cargo.lock'
    lock.write_text(view['lock'])
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(lock) <= registry_identities(reference_lock)
    known_versions = versions(reference_lock)
    assert all(resolved <= known_versions.get(name, set()) for name, resolved in versions(lock).items())
    manifest = view['manifest']
    metadata = [s for s in receipt['steps'] if s['command'][:2] == ['cargo', 'metadata']]
    assert metadata and all(s['exit_code'] == 0 for s in metadata)
    package = next(p for p in json.loads(metadata[0]['stdout'])['packages']
                   if p['manifest_path'] == str(source_root / 'Cargo.toml'))
    assert manifest == expected_manifest(package)
    expected_test = contract['test_name']
    assert receipt['original']['tests'] == [expected_test]
    tests = [s for s in receipt['steps'] if s['command'][:2] == ['cargo', 'test']]
    assert len(tests) == 1
    step = tests[0]
    assert step['exit_code'] == 0 and '--offline' in step['command'] and '--locked' in step['command']
    assert '--nocapture' in step['command'] and '--test-threads=1' in step['command']
    assert 'Compiling teaql-runtime v5.0.6' in step['stderr']
    assert expected_test in step['stdout'] and '1 passed; 0 failed' in step['stdout']
    outputs = [json.loads(line.split(contract['output_prefix'], 1)[1])
               for line in step['stdout'].split('\n') if contract['output_prefix'] in line]
    assert len(outputs) == contract['count']
    assert same(outputs, receipt['original']['observations'])
    assert same(outputs, [c['expected'] for c in corpus['cases']])
    return dict(observations=contract['count'], original_test=expected_test,
                original_source_files=len(actual_files), embedded_input_path=embedded_path,
                original_stdout_sha256=hashlib.sha256(step['stdout'].encode()).hexdigest(),
                observer_target=observer_target, observer_version=observer_version,
                observer_snapshot_sha256=template_hash,
                current_observer_sha256=digest(ROOT / contract['observer']),
                composite_assembly=assembly,
                observer_sha256=observer['sha256'], manifest_sha256=hashlib.sha256(manifest.encode()).hexdigest(),
                registry_packages=len(registry_identities(lock)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path,
                        default=ROOT / 'tests/parity/corpus_source_contracts_5_0_6.json')
    parser.add_argument('--source-only', action='store_true',
                        help='Check archived source bindings without building or invoking native probes')
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    registry_path = args.contracts.resolve()
    registry = json.loads(registry_path.read_text())
    contracts = registry['contracts']
    assert type(contracts) is list and contracts
    assert len({c['name'] for c in contracts}) == len(contracts)
    assert registry['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert registry['generator_commit'] == json.loads(
        (ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']
    record = dict(complete=False, whole_provenance_complete=False, steps=[], contracts=[],
                  scope=__doc__, input_hashes_before={str(registry_path): digest(registry_path)})
    inputs = [Path(__file__), registry_path, ROOT / 'scripts/reference_runtime_view_5_0_6.py',
              ROOT / 'scripts/corpus_source_assets_5_0_6.py',
              ROOT / 'tests/parity/compare_values.py', ROOT / 'upstream.lock.json',
              ROOT / '.reference/teaql-rs/Cargo.lock']
    inputs += [ROOT / c[k] for c in contracts for k in ('corpus', 'receipt', 'producer', 'observer', 'input_generator')]
    inputs += [ROOT / name for c in contracts for key in ('asset_versions', 'input_files', 'compile_inputs')
               for name in c.get(key, {})]
    record['input_hashes_before'] = {str(p): digest(p) for p in inputs}

    def run(command):
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                                env=dict(os.environ, TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1'))
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        print(' '.join(command[:5]), 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        if not args.source_only:
            run(['cmake', '--build', '--preset', 'native', '--parallel', '2', '--target',
                 *dict.fromkeys(c['native_target'] for c in contracts)])
        with tempfile.TemporaryDirectory(prefix='teaql_corpus_source_contracts_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            for contract in contracts:
                corpus = read(ROOT / contract['corpus'])
                receipt = read(ROOT / contract['receipt'])
                result = validate_source(contract, receipt, corpus, owned)
                binary = ROOT / 'build_native_current' / contract['native_target']
                actual = None
                if not args.source_only:
                    actual = observe(str(binary), [c['input'] for c in corpus['cases']])
                    assert same(actual, [c['expected'] for c in corpus['cases']]), contract['name']
                record['contracts'].append(dict(name=contract['name'], corpus=contract['corpus'],
                    corpus_sha256=digest(ROOT / contract['corpus']), receipt=contract['receipt'],
                    receipt_sha256=digest(ROOT / contract['receipt']), validation=result,
                    current_binary_sha256=None if args.source_only else digest(binary), current_observations=actual))
                print(contract['name'], contract['count'],
                      'archived source verified' if args.source_only else 'source-bound and current-native matches', flush=True)
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['input_hashes_after'] = {str(p): digest(p) for p in inputs}
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print(len(contracts), 'original-execution bindings verified;',
          'source-only' if args.source_only else str(sum(c['count'] for c in contracts)) + ' current native outputs match',
          '; complete SQ02 remains open')


if __name__ == '__main__':
    main()
