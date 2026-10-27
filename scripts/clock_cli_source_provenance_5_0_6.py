#!/usr/bin/env python3
"""Verify actual clock processes through unchanged single-process source checks.

Each view retains the three real Cargo steps and one real JSONL process. Only
step indices are remapped. The outer validator proves complete wall partitions,
raw trace coverage, the independently built clock and all boundary processes.
"""

import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import resource
import subprocess
import sys

import clock_cli_protocol_5_0_6 as protocol
from custom_public_cli_source_provenance_5_0_6 import digest, require
from expression_cli_source_provenance_5_0_6 import Validator as ProcessValidator, read

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from expression_cli_cases import require_cases

REGISTRY = ROOT / 'tests/parity/clock_cli_source_contracts_5_0_6.json'


class ProcessView(ProcessValidator):
    def __init__(self, root, registry, source, rows):
        super().__init__(root, registry)
        self.scope = dict(prefix='teaql_clock_cli_original_506_', datasets=1,
                          fixture='clock_cli_source', library=None)
        self.source = source
        self.rows = rows

    def selected_rows(self, contract, corpus):
        return self.rows


class Validator:
    def __init__(self, root, registry):
        self.root = root.resolve()
        self.registry = registry

    def load(self):
        entry = self.registry['source_execution']
        require(digest(self.root / entry['path']) == entry['sha256'], 'complete clock source identity')
        self.source = read(self.root / entry['path'])
        return self

    def view(self, source, contract, result, run, rows):
        original = source['steps'][run['step']]
        selected = [rows[i] for i in run['indices']]
        bounded = copy.copy(source)
        bounded['input_hashes_before'] = source['source_input_hashes_before']
        bounded['input_hashes_after'] = source['source_input_hashes_after']
        bounded['steps'] = source['steps'][:3] + [original]
        bounded['results'] = [dict(name=contract['name'], target=contract['target'],
                                   corpus=contract['corpus'], corpus_sha256=contract['corpus_sha256'],
                                   inputs=[v['input'] for v in selected],
                                   expected=[v['expected'] for v in selected],
                                   observed=[result['observed'][i] for i in run['indices']],
                                   step=3, selection=None, binary_sha256=result['binary_sha256'])]
        bounded['dataset_contracts'] = [dict(name=contract['name'], target=contract['target'],
                                            corpus=contract['corpus'], selection=None)]
        registry = dict(self.registry['process_contract'], kind='query', contracts=[dict(
            contract, count=len(selected), corpus_count=len(rows), selection=None)])
        return ProcessView(self.root, registry, bounded, selected)

    def validate_files(self):
        r, root = self.registry, self.root
        require(r['kind'] == 'clock_cli' and r['drivers'] == protocol.DRIVERS, 'clock driver selection')
        for name, wanted in r['support_files'].items():
            require(digest(root / name) == wanted, 'current clock helper identity: ' + name)
        require(same(r['component_refusal_reuse'], self.source['component_refusal_reuse']),
                'explicit historical component refusal references')
        for path, wanted in r['component_refusal_reuse'].items():
            require(digest(root / path) == wanted, 'historical complete-domain failure identity')
            failure = read(root / path)
            require(type(failure['exit_code']) is int and failure['exit_code'] != 0,
                    'complete-domain compilation remains refused')
            for name, value in failure['hashes'].items():
                require(digest(root / name) == value, 'unchanged refused model/generated provenance')
        first, result = r['contracts'][0], self.source['results'][0]
        for fixture in self.source['generated_fixtures']:
            path = root / fixture['path']
            require({p.relative_to(path).as_posix(): digest(p) for p in path.rglob('*')
                     if p.is_file() and p.name != 'PROVENANCE.json'} == fixture['provenance']['files'],
                    'complete original generated fixture file set')
        rows = self.rows(first)
        self.view(self.source, first, result, result['runs'][0], rows).validate_files()

    def rows(self, contract):
        corpus = read(self.root / contract['corpus'])
        require(digest(self.root / contract['corpus']) == contract['corpus_sha256'], 'complete clock corpus')
        require(corpus['baseline'] == self.registry['runtime_commit'], 'clock corpus baseline')
        values = protocol.inputs(self.root, contract['name'])
        return require_cases(corpus, len(values), values)

    def validate_source(self, source):
        root, r = self.root, self.registry
        require(source['complete'] is True and source['whole_goal_complete'] is False, 'bounded clock execution')
        require(same(source['generated_fixtures'], self.source['generated_fixtures']),
                'complete declared original fixture inventory')
        require(source['dataset_contracts'] == [dict(name=c['name'], target=c['target'],
                    corpus=c['corpus'], selection=None) for c in r['contracts']],
                'complete original clock dataset declarations')
        require(source['source_input_hashes_before'] == source['source_input_hashes_after']
                == source['source_files'] == r['source_files'], 'complete frozen source/recipe input scope')
        require(source['references_before'] == source['references_after']
                == {'teaql-rs': r['runtime_commit'], 'teaql_code_gen': r['generator_commit']}, 'clock references')
        require(source['drivers'] == protocol.DRIVERS and same(source['core_dump_limit'], [0, 0]),
                'original drivers and task-owned failure cleanup policy')
        require(len(source['results']) == len(r['contracts']) == 6 and len(source['steps']) == 55,
                'complete physical clock trace')
        require(source['clock_build_step'] == 3 and source['clock_binary_sha256'] == r['clock_binary_sha256'],
                'independent clock binary identity')
        owned = Path(source['owned_directory'])
        repository = owned / 'repository'
        command = [r['clock_compiler'], '-std=c++20', '-fPIC', '-shared', '-O0', '-g0',
                   str(repository / 'tests/support/fixed_sample_clock.cpp'), '-o', str(owned / 'clock.so')]
        require(source['steps'][3]['command'] == command and type(source['steps'][3]['exit_code']) is int
                and source['steps'][3]['exit_code'] == 0, 'actual independent clock compilation')
        require(source['derived_observer']['manifest'] == protocol.manifest(root), 'exact derived clock manifest')
        metadata = json.loads(source['derived_observer']['metadata'])
        packages = {p['id']: p for p in metadata['packages']
                    if p['source'] is None and p['name'] != protocol.PACKAGE}
        compiled = {v['package_id']: v for v in json_lines(source['steps'][2]['stdout'])
                    if v.get('reason') == 'compiler-artifact' and v['package_id'] in packages
                    and v['target']['kind'] in (['lib'], ['proc-macro'])}
        require(set(compiled) == set(packages), 'all actual runtime/domain/component libraries')
        for identity, artifact in compiled.items():
            targets = [t for t in packages[identity]['targets'] if t['kind'] in (['lib'], ['proc-macro'])]
            require(len(targets) == 1 and same(artifact['target'], targets[0]), 'original library source path')
            profile = artifact['profile']
            require(profile['debug_assertions'] is True and profile['overflow_checks'] is True
                    and profile['test'] is False and profile['opt_level'] == '0', 'ordinary Debug libraries')
        cursor = 4
        for contract, result in zip(r['contracts'], source['results'], strict=True):
            rows = self.rows(contract)
            inputs, expected = [v['input'] for v in rows], [v['expected'] for v in rows]
            require(result['name'] == contract['name'] and result['target'] == contract['target']
                    and result['corpus'] == contract['corpus']
                    and result['corpus_sha256'] == contract['corpus_sha256']
                    and result['binary_sha256'] == contract['binary_sha256'], 'original clock dataset identity')
            require(same(result['inputs'], inputs) and same(result['expected'], expected)
                    and same(result['observed'], expected), 'complete exact clock matrix')
            partitions = protocol.groups(inputs)
            require(len(result['runs']) == len(partitions), 'all wall partitions')
            for run, (millis, indices) in zip(result['runs'], partitions, strict=True):
                require(type(run['wall_millis']) is int and run['wall_millis'] == millis
                        and same(run['indices'], indices) and type(run['step']) is int
                        and run['step'] == cursor, 'complete ordered physical clock partition')
                step = source['steps'][cursor]
                require(step['clock_environment'] == dict(LD_PRELOAD=str(owned / 'clock.so'),
                            TEAQL_TEST_SAMPLE_WALL_MILLIS=str(millis)), 'actual process clock binding')
                self.view(source, contract, result, run, rows).validate_source(
                    self.view(source, contract, result, run, rows).source)
                cursor += 1
        boundary = source['boundary']
        require(boundary['name'] == 'clock_epoch_boundary'
                and boundary['binary_sha256'] == r['boundary_binary_sha256'], 'original boundary executable')
        values = protocol.inputs(root, 'clock_epoch_boundary')
        require(len(boundary['cases']) == len(values) == 36, 'complete original epoch boundaries')
        for case, value in zip(boundary['cases'], values, strict=True):
            require(same(case['input'], value) and type(case['step']) is int and case['step'] == cursor,
                    'exact original boundary selection')
            step = source['steps'][cursor]
            require(type(step['exit_code']) is int
                    and step['command'] == [str(owned / 'target/debug/clock_epoch_boundary'), value['mode']]
                    and step['clock_environment'] == dict(LD_PRELOAD=str(owned / 'clock.so'),
                        TEAQL_TEST_SAMPLE_WALL_MILLIS=str(value['wall_millis'])), 'actual boundary process')
            require(same(protocol.boundary(step, value), case['observed']), 'original explicit value/failure')
            cursor += 1
        require(cursor == len(source['steps']) and source['owned_directory_removed'] is True
                and not owned.exists() and type(source['owned_bytes_removed']) is int
                and source['owned_bytes_removed'] > 0, 'complete source trace and cleanup')

    def validate_commands(self, tests, build):
        by_name = {t['name']: t for t in tests}
        for name in protocol.DRIVERS:
            test_name = name + '_rust_parity' if name != 'clock_epoch_boundary' else 'clock_epoch_boundary_contracts'
            test = by_name[test_name]
            script = ('tests/parity/run_clock_boundary_5_0_6.py' if name == 'clock_epoch_boundary'
                      else protocol.comparator(name))
            expected = [str(self.root / script), '--cpp',
                        str(build.resolve() / ('teaql_' + name + '_probe')),
                        '--clock', str(build.resolve() / 'libteaql_sample_fixed_clock.so')]
            require(Path(test['command'][0]).resolve() == Path(sys.executable).resolve()
                    and test['command'][1:] == expected, 'actual clock CTest arguments')
            properties = {p['name']: p['value'] for p in test.get('properties', [])}
            require('clock_cli_source' in properties.get('FIXTURES_REQUIRED', []), 'clock source prerequisite')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(['ctest', '--test-dir', str(build),
                                                   '--show-only=json-v1'], text=True))['tests']
        self.validate_commands(tests, build)

    def validate_native(self, build):
        from sample_clock_driver import observe_fixed
        from compare_clock_epoch_boundary_5_0_6 import observe as observe_boundary
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        clock = build / 'libteaql_sample_fixed_clock.so'
        results = []
        for contract, original in zip(self.registry['contracts'], self.source['results'], strict=True):
            binary = build / ('teaql_' + contract['name'] + '_probe')
            actual = observe_fixed(binary, original['inputs'], clock)
            require(same(actual, original['observed']), 'exact current native clock binding')
            results.append(dict(name=contract['name'], inputs=original['inputs'], observed=actual,
                                binary_sha256=digest(binary)))
        cases = []
        for original in self.source['boundary']['cases']:
            value = original['input']
            actual = observe_boundary(build / 'teaql_clock_epoch_boundary_probe', clock,
                                      value['wall_millis'], value['mode'])
            projected = {k: actual[k] for k in ('outcome', 'output') if k in actual}
            if actual['outcome'] == 'failure':
                projected['reason'] = actual['reason']
            require(same(projected, original['observed']), 'exact boundary value/failure class')
            cases.append(dict(input=value, native=actual))
        return dict(jsonl=results, boundary=cases, clock_sha256=digest(clock))


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, default=REGISTRY)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--ctest-build-dir', type=Path)
    parser.add_argument('--cpp-build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.verify_only or args.cpp_build_dir is not None, 'select source or native validation')
    require(args.record is None or not args.record.exists(), 'preserve historical evidence')
    validator = Validator(ROOT, read(args.contracts)).load()
    validator.validate_files()
    validator.validate_source(validator.source)
    if args.ctest_build_dir:
        validator.validate_ctest(args.ctest_build_dir)
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else None
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, source_jsonl_outcomes=2593,
            source_boundary_outcomes=36, native=native, registry_sha256=digest(args.contracts)),
            indent=2) + '\n').encode(), mtime=0))
    print('Verified clock source: 2593 JSONL outcomes and 36 explicit boundary outcomes'
          + (' with current native execution' if native else ''))


if __name__ == '__main__':
    main()
