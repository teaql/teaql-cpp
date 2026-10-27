#!/usr/bin/env python3
"""Qualify complete lifecycle CLI source and its explicit resource/log adapters."""

import argparse
import gzip
import json
import os
from pathlib import Path
import resource
import subprocess
import sys

import lifecycle_cli_protocol_5_0_6 as protocol
from expression_cli_source_provenance_5_0_6 import Validator as SourceValidator, read
from custom_public_cli_source_provenance_5_0_6 import digest, require
from optional_source_guard_transition_5_0_6 import reconcile_source_files

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, observe, same
from expression_cli_cases import require_cases

REGISTRY = ROOT / 'tests/parity/lifecycle_cli_source_contracts_5_0_6.json'


class Validator(SourceValidator):
    def __init__(self, root, registry):
        # Keep the established source validator's identity protocol unchanged.
        # Lifecycle-specific scope, recipes and native ownership/log checks follow.
        super().__init__(root, dict(registry, kind='query'))
        self.scope = dict(prefix='teaql_lifecycle_cli_original_506_', datasets=5,
                          fixture='lifecycle_cli_source', library=None)

    def selected_rows(self, contract, corpus):
        require(corpus['baseline'] == self.registry['runtime_commit'], 'lifecycle corpus baseline')
        values = protocol.inputs(self.root, contract['name'])
        require(contract['count'] == len(values), 'complete canonical lifecycle inventory')
        return require_cases(corpus, len(values), values)

    def validate_files(self):
        super().validate_files()
        require(self.registry['drivers'] == protocol.DRIVERS, 'complete original lifecycle driver selection')
        require([c['name'] for c in self.registry['contracts']] == list(protocol.NAMES),
                'complete actual lifecycle contracts')
        for fixture in self.source['generated_fixtures']:
            path = self.root / fixture['path']
            actual = {p.relative_to(path).as_posix(): digest(p) for p in path.rglob('*')
                      if p.is_file() and p.name != 'PROVENANCE.json'}
            require(actual == fixture['provenance']['files'], 'complete original generated fixture set')

    def validate_source(self, source):
        require(source['derived_observer']['manifest'] == protocol.manifest(self.root),
                'exact reference-seeded lifecycle observer')
        current_files = reconcile_source_files(self.root, source['source_files'],
                                               self.registry['source_files'],
                                               self.registry['source_guard_transition'])
        super().validate_source(dict(source, source_files=current_files))
        metadata = json.loads(source['derived_observer']['metadata'])
        libraries = {p['id']: p for p in metadata['packages']
                     if p['source'] is None and p['name'] != protocol.PACKAGE}
        compiled = {row['package_id']: row for row in json_lines(source['steps'][2]['stdout'])
                    if row.get('reason') == 'compiler-artifact' and row['package_id'] in libraries
                    and row['target']['kind'] in (['lib'], ['proc-macro'])}
        require(set(compiled) == set(libraries), 'complete runtime/domain compilation')
        for identity, artifact in compiled.items():
            targets = [t for t in libraries[identity]['targets'] if t['kind'] in (['lib'], ['proc-macro'])]
            require(len(targets) == 1 and same(artifact['target'], targets[0]), 'original library source paths')
            profile = artifact['profile']
            require(profile['debug_assertions'] is True and profile['overflow_checks'] is True
                    and profile['test'] is False and profile['opt_level'] == '0',
                    'ordinary runtime/domain Debug semantics')
        require(same(source['core_dump_limit'], [0, 0]), 'owned observer failure policy')

    def validate_commands(self, tests, build):
        by_name = {t['name']: t for t in tests}
        scripts = {str(self.root / protocol.comparator(name)) for name in protocol.NAMES}
        require({t['name'] for t in tests if len(t.get('command', [])) > 1
                 and t['command'][1] in scripts} == {n + '_rust_parity' for n in protocol.NAMES},
                'complete actual lifecycle CTest entry selection')
        for name in protocol.NAMES:
            test = by_name[name + '_rust_parity']
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == [str(self.root / protocol.comparator(name)),
                        '--cpp', str(build.resolve() / ('teaql_' + name + '_probe'))],
                    'actual full lifecycle CTest arguments')
            properties = {p['name']: p['value'] for p in test.get('properties', [])}
            require('lifecycle_cli_source' in properties.get('FIXTURES_REQUIRED', []),
                    'required lifecycle source prerequisite')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(['ctest', '--test-dir', str(build),
                                                   '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        results = []
        for contract, source in zip(self.registry['contracts'], self.source['results'], strict=True):
            binary = build / contract['binary']
            actual = observe(str(binary), source['inputs'])
            require(protocol.compare(self.root, contract['name'], source['inputs'], source['observed'], actual),
                    'declared current native lifecycle comparison')
            results.append(dict(name=contract['name'], inputs=source['inputs'], observed=actual,
                                binary_sha256=digest(binary)))
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
    require(args.verify_only or args.cpp_build_dir is not None, 'select lifecycle source or native validation')
    require(args.record is None or not args.record.exists(), 'preserve historical evidence')
    validator = Validator(ROOT, read(args.contracts)).load()
    validator.validate_files()
    validator.validate_source(validator.source)
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else []
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, source_datasets=5, source_observations=3588,
            native=native, registry_sha256=digest(args.contracts)), indent=2) + '\n').encode(), mtime=0))
    print('Verified lifecycle source: 5 complete datasets / 3588 outcomes'
          + (' with declared native ownership/log comparison' if native else ''))


if __name__ == '__main__':
    main()
