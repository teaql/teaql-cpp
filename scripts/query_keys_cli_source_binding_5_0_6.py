#!/usr/bin/env python3
"""Bind query-key CLI to locked 64-bit and width-prepared 32-bit source."""

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
REGISTRY = ROOT / 'tests/parity/query_keys_cli_contract_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import observe, same


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
            require(digest(ROOT / path) == wanted, 'current support: ' + path)
        for key in ('corpus_64', 'corpus_32', 'historical_64', 'historical_32',
                    'source_64', 'source_32', 'mips_receipt'):
            require(digest(ROOT / contract[key]) == contract[key + '_sha256'],
                    'retained ' + key)
        source_64 = read(ROOT / contract['source_64'])
        require(source_64['baseline'] == contract['runtime_commit']
                and source_64['normalization'] == 'none',
                'original 64-bit qualification scope')
        matrix = next(row for row in source_64['matrices']
                      if row['stem'] == 'query_keys')
        require(matrix['inputs'] == contract['count']
                and matrix['source_changed'] == contract['count']
                and matrix['historical_sha256']
                == digest(ROOT / contract['historical_64']),
                'original 64-bit key matrix')
        for path, wanted in source_64['observer_source_hashes'].items():
            require(digest(ROOT / path) == wanted,
                    'original 64-bit observer: ' + path)
        for path, wanted in source_64['original_sources'].items():
            require(digest(ROOT / '.reference/teaql-rs/teaql-runtime/src' / path)
                    == wanted, 'locked runtime source: ' + path)
        source_32 = read(ROOT / contract['source_32'])
        require(source_32['complete'] is True
                and source_32['runtime_commit'] == contract['runtime_commit']
                and source_32['generator_commit'] == contract['generator_commit']
                and source_32['pointer_bytes'] == 4
                and source_32['observations'] == contract['count']
                and source_32['owned_directory_removed'] is True,
                'width-prepared original source receipt')
        for path, wanted in source_32['source_files'].items():
            if path == source_32['selector_path']:
                current = (ROOT / path).read_text()
                old = source_32['selector_replacement']['old']
                new = source_32['selector_replacement']['new']
                require(current.count(new) == 1 and old not in current
                        and hashlib.sha256(current.replace(new, old).encode()).hexdigest()
                        == wanted, 'installed one-line width selector repair')
                continue
            require(digest(ROOT / path) == wanted,
                    'width-prepared source file: ' + path)
        mips = read(ROOT / contract['mips_receipt'])
        require(mips['complete'] is True and mips['parity_passed'] is True
                and mips['source_refresh_sha256'] == digest(ROOT / contract['source_32'])
                and mips['runtime_commit'] == contract['runtime_commit']
                and mips['mips_runtime_execution'] is True
                and mips['physical_router_execution'] is False
                and next(row for row in mips['matrices']
                         if row['name'] == 'width_32_legacy_inputs')['matched']
                == contract['count'], 'retained actual C++ MIPS execution scope')

    def validate_source(self, override_64=None, override_32=None):
        contract = self.registry
        corpus_64 = override_64 if override_64 is not None else read(ROOT / contract['corpus_64'])
        corpus_32 = override_32 if override_32 is not None else read(ROOT / contract['corpus_32'])
        historical_64 = read(ROOT / contract['historical_64'])
        historical_32 = read(ROOT / contract['historical_32'])
        source_32 = read(ROOT / contract['source_32'])
        require(corpus_64['baseline'] == corpus_32['baseline']
                == contract['runtime_commit'], 'current corpus baselines')
        require(corpus_64['historical_sha256']
                == digest(ROOT / contract['historical_64'])
                and corpus_32['historical_corpus_sha256']
                == digest(ROOT / contract['historical_32']),
                'historical corpus identities')
        require(corpus_32['observer_sha256']
                == digest(ROOT / corpus_32['observer'])
                and same(corpus_32, source_32['proposed_corpus']),
                'installed width-prepared source corpus')
        rows_64, rows_32 = corpus_64['cases'], corpus_32['cases']
        require(len(rows_64) == len(rows_32) == contract['count'],
                'complete key input matrices')
        inputs_64 = [row['input'] for row in rows_64]
        inputs_32 = [row['input'] for row in rows_32]
        require(same(inputs_64, inputs_32)
                and same(inputs_64, [row['input'] for row in historical_64['cases']])
                and same(inputs_32, [row['input'] for row in historical_32['cases']]),
                'same complete original key inputs at both widths')
        results_64 = [row['expected'] for row in rows_64]
        results_32 = [row['expected'] for row in rows_32]
        require(sum(not same(a, b) for a, b in zip(results_64, results_32))
                == contract['width_differences'], 'declared 32/64 key differences')
        require(sum(not same(a, b) for a, b in zip(
                    results_64, [row['expected'] for row in historical_64['cases']]))
                == contract['count'], '5.0.6 64-bit source refresh')
        require(sum(not same(a, b) for a, b in zip(
                    results_32, [row['expected'] for row in historical_32['cases']]))
                == contract['count'], '5.0.6 32-bit source refresh')

    def validate_commands(self, tests, build):
        contract = self.registry
        names = [row['name'] for row in tests]
        require(len(names) == len(set(names)), 'unique CTest names')
        script = str(ROOT / contract['comparator'])
        require({row['name'] for row in tests
                 if len(row.get('command', [])) > 1 and row['command'][1] == script}
                == {contract['test']}, 'complete actual query-key comparison selection')
        test = next(row for row in tests if row['name'] == contract['test'])
        require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                and test['command'][1:] == [script, '--cpp',
                                           str(build.resolve() / contract['binary']),
                                           '--cpp-emulator=', '--pointer-bytes=8'],
                'exact current native key CTest command')
        properties = {prop['name']: prop['value']
                      for prop in test.get('properties', [])}
        require('query_keys_cli' in properties.get('FIXTURES_REQUIRED', []),
                'source fixture on query-key comparison')
        width = next(row for row in tests
                     if row['name'] == 'parity_query_key_width_cli')
        require(width['command'][1:] == [str(ROOT / 'tests/parity/verify_query_key_width_cli_5_0_6.py')],
                'installed two-stage width selector control')

    def validate_ctest(self, build):
        self.validate_commands(read_ctest(build), build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = read(ROOT / self.registry['corpus_64'])
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        binary = build.resolve() / self.registry['binary']
        actual = observe(str(binary), inputs)
        require(same(actual, expected), 'fresh complete native 64-bit keys')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    binary_sha256=digest(binary))

    def validate_mips(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        corpus = read(ROOT / self.registry['corpus_32'])
        inputs = [row['input'] for row in corpus['cases']]
        expected = [row['expected'] for row in corpus['cases']]
        mips = read(ROOT / self.registry['mips_receipt'])
        binary = build.resolve() / self.registry['binary']
        require(digest(binary) == mips['binary_sha256'],
                'unchanged qualified MIPS binary')
        actual = observe(str(binary), inputs, emulator=mips['emulator'])
        require(same(actual, expected), 'fresh complete QEMU 32-bit keys')
        return dict(observations=len(actual), exact_equalities=len(actual),
                    binary_sha256=digest(binary), emulator=mips['emulator'],
                    rust_mips_execution=False)


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
    parser.add_argument('--mips-build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.verify_only or args.cpp_build_dir or args.mips_build_dir,
            'select source or current native/MIPS validation')
    require(args.record is None or not args.record.exists(),
            'preserve historical evidence')
    validator = Validator(read(args.contracts))
    validator.validate_files()
    validator.validate_source()
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    mips = validator.validate_mips(args.mips_build_dir) if args.mips_build_dir else None
    if args.record:
        result = dict(complete=True, whole_goal_complete=False,
                      original_observations_per_width=451,
                      width_differences=46, native_64=native, cpp_mips_32=mips,
                      registry_sha256=digest(args.contracts))
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Verified query-key source: 451 inputs at both widths; 46 distinct results'
          + (' with fresh native 64-bit keys' if native else '')
          + (' and fresh C++ MIPS/QEMU 32-bit keys' if mips else ''))


if __name__ == '__main__':
    main()
