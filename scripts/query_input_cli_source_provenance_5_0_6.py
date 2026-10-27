#!/usr/bin/env python3
"""Bind eight complete Rust query-input drivers to exact current native CLIs."""

import argparse
import gzip
import json
import os
from pathlib import Path
import resource
import subprocess
import sys

import query_input_cli_protocol_5_0_6 as protocol
from custom_public_cli_source_provenance_5_0_6 import digest, require
from expression_cli_source_provenance_5_0_6 import Validator as SourceValidator, read

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / 'tests/parity/query_input_cli_source_contracts_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from expression_cli_cases import require_cases


class Validator(SourceValidator):
    def __init__(self, root, registry):
        super().__init__(root, dict(registry, kind='query'))
        self.scope = dict(prefix='teaql_query_input_cli_original_506_', datasets=8,
                          fixture='query_input_cli_source', library=None)

    def selected_rows(self, contract, corpus):
        require(corpus['baseline'] == self.registry['runtime_commit'], 'source corpus baseline')
        values = protocol.inputs(self.root, contract['name'])
        require(contract['count'] == len(values), 'complete canonical query-input recipe')
        return require_cases(corpus, len(values), values)

    def validate_files(self):
        super().validate_files()
        require(self.registry['drivers'] == protocol.DRIVERS,
                'complete unchanged query-input driver selection')
        require([contract['name'] for contract in self.registry['contracts']]
                == list(protocol.NAMES), 'complete actual query-input contracts')
        require({(item['name'], item['test']) for item in self.registry['additional_consumers']}
                == {('matching_statistics_inputs',
                     'generated_matching_statistics_adapters_rust_parity'),
                    ('relation_analysis_inputs',
                     'generated_analysis_adapter_inputs_rust_parity')},
                'complete generated adapter consumer selection')
        require([item['path'] for item in self.source['generated_fixtures']]
                == [item[0] for item in protocol.FIXTURE_ALIASES],
                'three complete generated domain selections')
        for fixture in self.source['generated_fixtures']:
            path = self.root / fixture['path']
            actual = {source.relative_to(path).as_posix(): digest(source)
                      for source in path.rglob('*')
                      if source.is_file() and source.name != 'PROVENANCE.json'}
            require(actual == fixture['provenance']['files'],
                    'complete original generated fixture set')

    def validate_source(self, source):
        require(source['derived_observer']['manifest'] == protocol.manifest(self.root),
                'exact reference-seeded query-input observer')
        super().validate_source(source)
        metadata = json.loads(source['derived_observer']['metadata'])
        libraries = {package['id']: package for package in metadata['packages']
                     if package['source'] is None and package['name'] != protocol.PACKAGE}
        compiled = {row['package_id']: row for row in json_lines(source['steps'][2]['stdout'])
                    if row.get('reason') == 'compiler-artifact' and row['package_id'] in libraries
                    and row['target']['kind'] in (['lib'], ['proc-macro'])}
        require(set(compiled) == set(libraries), 'complete runtime/domain compilation')
        for identity, artifact in compiled.items():
            targets = [target for target in libraries[identity]['targets']
                       if target['kind'] in (['lib'], ['proc-macro'])]
            require(len(targets) == 1 and same(artifact['target'], targets[0]),
                    'original library source paths')
            profile = artifact['profile']
            require(profile['debug_assertions'] is True and profile['overflow_checks'] is True
                    and profile['test'] is False and profile['opt_level'] == '0',
                    'ordinary runtime/domain Debug semantics')
        require(same(source['core_dump_limit'], [0, 0]), 'owned observer failure policy')

    def validate_commands(self, tests, build):
        by_name = {test['name']: test for test in tests}
        scripts = {str(self.root / protocol.comparator(name)) for name in protocol.NAMES}
        consumers = self.registry['contracts'] + self.registry['additional_consumers']
        require({test['name'] for test in tests if len(test.get('command', [])) > 1
                 and test['command'][1] in scripts}
                == {contract['test'] for contract in consumers},
                'complete actual query-input CTest entry selection')
        for contract in consumers:
            test = by_name[contract['test']]
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == [str(self.root / contract['comparator']),
                                                '--cpp', str(build.resolve() / contract['binary'])],
                    'actual full query-input CTest arguments')
            properties = {property['name']: property['value']
                          for property in test.get('properties', [])}
            require('query_input_cli_source' in properties.get('FIXTURES_REQUIRED', []),
                    'required query-input source prerequisite')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(['ctest', '--test-dir', str(build),
                                                    '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        from compare_values import observe
        results = super().validate_native(build)
        source = {row['name']: row for row in self.source['results']}
        for consumer in self.registry['additional_consumers']:
            original = source[consumer['name']]
            binary = build / consumer['binary']
            actual = observe(str(binary), original['inputs'])
            require(same(actual, original['observed']),
                    'exact generated adapter consumer: ' + consumer['test'])
            results.append(dict(name=consumer['test'], count=len(actual),
                                binary_sha256=digest(binary),
                                inputs=original['inputs'], observed=actual))
        return results


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
            'select query-input source or native validation')
    require(args.record is None or not args.record.exists(), 'preserve historical evidence')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    validator = Validator(ROOT, read(args.contracts)).load()
    validator.validate_files()
    validator.validate_source(validator.source)
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else []
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, source_datasets=8,
            source_observations=sum(len(row['inputs']) for row in validator.source['results']),
            native=native, registry_sha256=digest(args.contracts)), indent=2) + '\n').encode(),
                                         mtime=0, compresslevel=3))
    print('Verified query-input source: 8 complete datasets / 13656 outcomes'
          + (' with 10 exact current native consumers' if native else ''))


if __name__ == '__main__':
    main()
