#!/usr/bin/env python3
"""Bind typed Business-ID E CTest to its complete original source matrix."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import resource
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / 'tests/parity/expression_business_ids_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
sys.path.insert(0, str(ROOT / 'scripts'))
from compare_expression_business_ids_5_0_6 import adapted, inputs
from compare_values import observe, same
from expression_diagnostic_framing import verify_negative_controls
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions


def read(path):
    path = Path(path)
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, label):
    if not value:
        raise RuntimeError(label)


class Validator:
    def __init__(self, registry):
        self.registry = registry

    def validate_files(self):
        contract = self.registry
        require(read(ROOT / 'upstream.lock.json')['commit'] == contract['runtime_commit'],
                'locked runtime')
        for checkout, commit in [('teaql-rs', contract['runtime_commit']),
                                 ('teaql_code_gen', contract['generator_commit'])]:
            actual = subprocess.check_output(
                ['git', '-C', str(ROOT / '.reference' / checkout), 'rev-parse', 'HEAD'],
                text=True).strip()
            require(actual == commit, 'locked checkout: ' + checkout)
        for path, wanted in contract['support_files'].items():
            require(digest(ROOT / path) == wanted, 'current source: ' + path)
        reference = ROOT / '.reference/teaql-rs/Cargo.lock'
        observer = ROOT / 'tests/parity/rust_expression_business_ids_5_0_6/Cargo.lock'
        require(registry_identities(observer) <= registry_identities(reference),
                'reference-seeded observer registry closure')
        wanted_versions = versions(reference)
        local_packages = {'cpp-business-ids-core',
                          'teaql_expression_business_ids_506'}
        for package, values in versions(observer).items():
            if package not in local_packages:
                require(values <= wanted_versions.get(package, set()),
                        'reference-seeded observer version: ' + package)
        for key in ('corpus', 'historical_pair'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        corpus = read(ROOT / contract['corpus'])
        require(corpus['baseline'] == contract['runtime_commit'],
                'original corpus baseline')
        producer = read(ROOT / 'tests/generator/parity/business_ids_rust/PROVENANCE.json')
        require(producer == corpus['producer']
                and producer['runtime_commit'] == contract['runtime_commit']
                and producer['generator_commit'] == contract['generator_commit'],
                'complete generated SDK identity')
        require(digest(ROOT / producer['model']) == producer['model_sha256'],
                'generated SDK model')
        sdk = ROOT / 'tests/generator/parity/business_ids_rust'
        for path, wanted in producer['files'].items():
            require(digest(sdk / path) == wanted, 'generated SDK file: ' + path)

    def validate_source(self, override=None):
        contract = self.registry
        corpus = read(ROOT / contract['corpus'])
        pair = override if override is not None else read(ROOT / contract['historical_pair'])
        cases = corpus['cases']
        source_inputs = [row['input'] for row in cases]
        expected = [row['expected'] for row in cases]
        require(len(cases) == contract['count'] and same(source_inputs, inputs()),
                'complete original E input matrix')
        require(pair['baseline'] == contract['runtime_commit']
                and pair['adaptation'] == contract['adaptation']
                and pair['failures'] == []
                and len(pair['original']) == len(pair['cpp']) == contract['count']
                and same(pair['original'], expected),
                'complete retained original E results')
        require(sum(same(a, b) for a, b in zip(expected, pair['cpp']))
                == contract['historical_raw_equalities'],
                'historical raw E difference count')
        require(all(same(a, adapted(b)) for a, b in zip(expected, pair['cpp'])),
                'complete retained adapted E results')
        require(sum('decode_error' in row for row in expected)
                == contract['decoder_rejections'], 'decoder rejection count')
        require(sum(12 for row in expected if 'decode_error' not in row)
                == contract['resolutions'], 'actual snapshot resolution count')
        diagnostic_index = next(
            index for index, row in enumerate(cases)
            if row['input']['id'] == 7 and not row['input']['loaded'])
        source_panic = expected[diagnostic_index]['before']['eval']['panic']
        native_panic = pair['cpp'][diagnostic_index]['before']['eval']['panic']
        require(verify_negative_controls(native_panic, source_panic) == 7,
                'diagnostic adapter negative controls')

    def validate_commands(self, tests, build):
        contract = self.registry
        names = [row['name'] for row in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        script = str(ROOT / contract['comparator'])
        require({row['name'] for row in tests
                 if len(row.get('command', [])) > 1 and row['command'][1] == script}
                == {contract['test']}, 'complete actual E comparison selection')
        test = next(row for row in tests if row['name'] == contract['test'])
        require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                and test['command'][1:] == [script, '--cpp',
                                           str(build.resolve() / contract['binary'])],
                'exact current E CTest command')
        properties = {prop['name']: prop['value']
                      for prop in test.get('properties', [])}
        require('expression_business_ids_cli' in properties.get('FIXTURES_REQUIRED', []),
                'source fixture on E comparison')

    def validate_ctest(self, build):
        self.validate_commands(read_ctest(build), build)

    def validate_native(self, build):
        contract = self.registry
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = read(ROOT / contract['corpus'])
        cases = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / contract['binary']
        actual = observe(str(binary), cases)
        require(all(same(a, adapted(b)) for a, b in zip(expected, actual)),
                'fresh complete adapted E outcomes')
        raw_equalities = sum(same(a, b) for a, b in zip(expected, actual))
        require(raw_equalities == contract['historical_raw_equalities'],
                'fresh raw E difference count')
        return dict(observations=len(actual), adapted_equalities=len(actual),
                    raw_equalities=raw_equalities,
                    diagnostic_adaptations=len(actual) - raw_equalities,
                    binary_sha256=digest(binary))


def read_ctest(build):
    return json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
        text=True))['tests']


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, default=REGISTRY)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--ctest-build-dir', type=Path)
    parser.add_argument('--cpp-build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.verify_only or args.cpp_build_dir is not None,
            'select source or current native validation')
    require(args.record is None or not args.record.exists(),
            'preserve historical evidence')
    validator = Validator(read(args.contracts))
    validator.validate_files()
    validator.validate_source()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      original_observations=96, current_adapted_equalities=96,
                      historical_raw_equalities=48, native=native,
                      registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified typed Business-ID E source: 96 adapted outcomes; 48 raw equalities'
          + (' with fresh native results' if native else ''))


if __name__ == '__main__':
    main()
