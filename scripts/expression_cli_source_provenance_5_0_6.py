#!/usr/bin/env python3
"""Verify complete original generated CLI sources and native presentation contracts."""
import argparse
import gzip
import hashlib
import importlib
import json
import os
from pathlib import Path
import subprocess
import sys

from custom_public_cli_source_provenance_5_0_6 import digest, require

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / 'tests/parity/expression_cli_source_contracts_5_0_6.json'
SCOPES = {
    'expression': dict(prefix='teaql_expression_cli_original_506_', datasets=6,
                       fixture='expression_cli_source', library=(
                           'teaql_expression_model_types_506',
                           'tests/parity/rust_expression_model_types_5_0_6/src/lib.rs')),
    'query': dict(prefix='teaql_query_cli_original_506_', datasets=11,
                  fixture='query_cli_source', library=None),
    'dynamic_json': dict(prefix='teaql_dynamic_json_cli_original_506_', datasets=11,
                         fixture='dynamic_json_cli_source', library=None),
}
DYNAMIC_JSON_ADAPTERS = {
    'generated_dynamic_json': 'column_debug', 'dynamic_json_execution': 'column_debug',
    'dynamic_json_policy': 'policy_column_debug',
    'dynamic_json_composition': 'policy_column_debug',
    'dynamic_json_persistent': 'policy_column_debug', 'dynamic_json_facets': 'policy_column_debug',
    'dynamic_json_scalar': 'identity', 'dynamic_json_domains': 'identity',
    'dynamic_json_inheritance': 'identity', 'dynamic_json_inherited_sql': 'identity',
    'dynamic_json_privacy': 'identity',
}


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def lock_identities(text):
    from profile_public_cli_source_provenance_5_0_6 import lock_identities as identities
    return identities(text)


class Validator:
    def __init__(self, root, registry):
        self.root = root.resolve()
        self.registry = registry
        self.kind = registry.get('kind', 'expression')
        require(self.kind in SCOPES, 'qualified source protocol')
        self.scope = SCOPES[self.kind]
        sys.path.insert(0, str(self.root / 'tests/parity'))
        from compare_values import json_lines, same
        self.lines, self.same = json_lines, same

    def load(self):
        entry = self.registry['source_execution']
        require(digest(self.root / entry['path']) == entry['sha256'], 'source execution identity')
        self.source = read(self.root / entry['path'])
        return self

    def validate_files(self):
        r, root = self.registry, self.root
        require(read(root / 'upstream.lock.json')['commit'] == r['runtime_commit'], 'runtime baseline')
        for name, commit in [('teaql-rs', r['runtime_commit']),
                             ('teaql_code_gen', r['generator_commit'])]:
            require(subprocess.check_output(['git', '-C', str(root / '.reference' / name),
                                             'rev-parse', 'HEAD'], text=True).strip() == commit,
                    'locked reference')
        for path, wanted in r['support_files'].items():
            require(digest(root / path) == wanted, 'current helper/observer identity: ' + path)
        for path, wanted in r['source_files'].items():
            require(digest(root / path) == wanted, 'unchanged original input: ' + path)
        for fixture in self.source['generated_fixtures']:
            path = root / fixture['path']
            manifest = read(path / 'PROVENANCE.json')
            require(self.same(manifest, fixture['provenance']), 'original generated provenance')
            require(manifest['hand_edited'] is False
                    and manifest['runtime_commit'] == r['runtime_commit']
                    and manifest['generator_commit'] == r['generator_commit'], 'original model baseline')
            require(digest(root / manifest['model']) == manifest['model_sha256'], 'original model')
            for name, wanted in manifest['files'].items():
                require(digest(path / name) == wanted, 'complete original generated library')

    def validate_source(self, source):
        r = self.registry
        require(source['complete'] is True and source['whole_goal_complete'] is False,
                'complete bounded source execution')
        require(source['runtime_commit'] == r['runtime_commit']
                and source['generator_commit'] == r['generator_commit'], 'source baseline')
        require(source['input_hashes_before'] == source['input_hashes_after']
                and source['references_before'] == source['references_after'], 'source input epoch')
        require(source['references_before'] == {'teaql-rs': r['runtime_commit'],
                                                'teaql_code_gen': r['generator_commit']},
                'source reference identities')
        require(self.same(source['source_files'], r['source_files']), 'all copied original inputs')
        require(self.same(source['generated_fixtures'], self.source['generated_fixtures']),
                'complete original library set')
        require(source['drivers'] == r['drivers'], 'all original driver paths')
        selections = ([dict(name=c['name'],target=c['target'],corpus=c['corpus'],
                            selection=c.get('selection')) for c in r['contracts']]
                      if self.kind != 'expression' else
                      [[c['name'], c['target'], c['corpus']] for c in r['contracts']])
        require(source['dataset_contracts'] == selections, 'actual source dataset selection')
        import archive_driver_provenance_5_0_6 as archive
        data = subprocess.check_output(['git', '-C', str(self.root / '.reference/teaql-rs'),
                                        'archive', r['runtime_commit']])
        require(hashlib.sha256(data).hexdigest() == source['archive_sha256'], 'runtime source archive')
        require(source['archive_files'] == {n: hashlib.sha256(v).hexdigest()
                                            for n, v in archive.archive_files(data).items()},
                'complete archived source members')
        project = source['derived_observer']
        require(digest(self.root / '.reference/teaql-rs/Cargo.lock') == project['seed_lock_sha256'],
                'reference dependency seed')
        require(hashlib.sha256(project['manifest'].encode()).hexdigest() == r['manifest_sha256'],
                'derived observer manifest')
        require(hashlib.sha256(project['compiled_lock'].encode()).hexdigest() == r['lock_sha256'],
                'compiled dependency lock')
        identities = lock_identities(project['compiled_lock'])
        require(identities <= lock_identities((self.root / '.reference/teaql-rs/Cargo.lock').read_text()),
                'reference registry identities/checksums')
        require(source['registry_identity_differences'] == [], 'reference dependency closure')
        owned = Path(source['owned_directory'])
        require(owned.parent == Path('/tmp') and owned.name.startswith(self.scope['prefix']),
                'owned source workspace')
        repository = owned / 'repository'
        expected_environment = dict(
            CARGO_TARGET_DIR=str(owned / 'target'), CARGO_INCREMENTAL='0',
            CARGO_PROFILE_DEV_DEBUG='0', CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
            TMPDIR=str(owned / 'children'), TEAQL_LOG_ENDPOINT='off')
        if self.kind == 'dynamic_json':
            expected_environment.update(TEAQL_BUSINESS_ID_KEY_V1_HEX='00' * 32,
                                        TEAQL_BUSINESS_ID_KEY_VERSION='1')
        require(source['cargo_environment'] == expected_environment, 'source build environment')
        manifest = repository / r['project']
        metadata = json.loads(project['metadata'])
        require({p['name']: str(Path(p['manifest_path']).relative_to(repository))
                 for p in metadata['packages'] if p['source'] is None} == r['local_packages'],
                'complete local package identities')
        require({(p['name'], p['version'], p['source']) for p in metadata['packages'] if p['source']}
                <= {p[:3] for p in identities}, 'resolved package identities')
        for package in metadata['packages']:
            if package['name'] in r['runtime_packages']:
                require(package['version'] == '5.0.6', 'original runtime version')
        observer = next(p for p in metadata['packages'] if p['manifest_path'] == str(manifest))
        targets = {t['name']: t for t in observer['targets']}
        library_target = self.scope['library']
        require(set(targets) == set(r['drivers']) | ({library_target[0]} if library_target else set()),
                'complete source target set')
        for target, driver in r['drivers'].items():
            require(targets[target]['kind'] == ['bin']
                    and Path(targets[target]['src_path']).resolve() == repository / driver,
                    'complete original driver target')
        if library_target:
            library = targets[library_target[0]]
            require(library['kind'] == ['lib'] and Path(library['src_path']).resolve()
                    == repository / library_target[1], 'original bounded entity/E library')
        steps = source['steps']
        dataset_count = self.scope['datasets']
        require(len(steps) == 3 + dataset_count and all(type(s['exit_code']) is int and s['exit_code'] == 0
                                       for s in steps), 'complete successful source steps')
        common = ['cargo', 'metadata', '--offline']
        require(steps[0]['command'] == common + ['--format-version', '1', '--manifest-path', str(manifest)],
                'dependency resolution command')
        require(steps[1]['command'] == common + ['--locked', '--format-version', '1',
                                                '--manifest-path', str(manifest)]
                and steps[1]['stdout'] == project['metadata'], 'actual locked metadata')
        require(source['build_step'] == 2 and steps[2]['command']
                == ['cargo', 'build', '--offline', '--locked', '--bins', '--message-format=json',
                    '--manifest-path', str(manifest), '-j2'], 'actual locked source build')
        artifacts = {v['target']['name']: v for v in self.lines(steps[2]['stdout'])
                     if v.get('reason') == 'compiler-artifact' and v.get('executable') is not None}
        require(self.same(artifacts, source['compiler_artifacts']) and set(artifacts) == set(r['drivers']),
                'complete actual compiler artifacts')
        for name, artifact in artifacts.items():
            profile = artifact['profile']
            require(profile['debug_assertions'] is True and profile['overflow_checks'] is True
                    and profile['test'] is False and profile['opt_level'] == '0', 'original Debug profile')
            require(artifact['target']['kind'] == ['bin']
                    and Path(artifact['target']['src_path']).resolve() == repository / r['drivers'][name],
                    'compiled original source path')
            require(artifact['package_id'] == observer['id']
                    and Path(artifact['executable']) == owned / 'target/debug' / name,
                    'original compiler package/executable')
        require(len(source['results']) == len(r['contracts']) == dataset_count, 'complete dataset count')
        for contract, result in zip(r['contracts'], source['results']):
            if self.kind == 'dynamic_json':
                require(contract['adapter'] == DYNAMIC_JSON_ADAPTERS[contract['name']],
                        'explicit dynamic JSON presentation contract')
            corpus = read(self.root / contract['corpus'])
            require(digest(self.root / contract['corpus']) == contract['corpus_sha256'], 'source corpus identity')
            rows = self.selected_rows(contract, corpus)
            require(result['name'] == contract['name'] and result['target'] == contract['target']
                    and result['corpus'] == contract['corpus']
                    and result['corpus_sha256'] == contract['corpus_sha256'], 'selected original dataset')
            require(result['binary_sha256'] == contract['binary_sha256'], 'original observer executable identity')
            require(self.same(result.get('selection'), contract.get('selection')), 'original dataset partition')
            require(self.same(result['inputs'], [c['input'] for c in rows])
                    and self.same(result['expected'], [c['expected'] for c in rows]), 'complete source inputs/expected')
            require(type(result['step']) is int and 3 <= result['step'] < 3 + dataset_count, 'original observation step')
            step = steps[result['step']]
            require(step['command'] == [artifacts[contract['target']]['executable']], 'actual original execution')
            require(self.same(self.lines(step['stdout']), result['observed'])
                    and self.same(result['observed'], result['expected']), 'complete exact original raw stream')
        require({v['step'] for v in source['results']} == set(range(3, 3 + dataset_count)), 'all distinct original executions')
        require(source['owned_directory_removed'] is True and not owned.exists()
                and type(source['owned_bytes_removed']) is int and source['owned_bytes_removed'] > 0,
                'owned source cleanup')

    def validate_ctest(self, build):
        tests = json.loads(subprocess.check_output(['ctest', '--test-dir', str(build),
                            '--show-only=json-v1'], text=True))['tests']
        by_name = {t['name']: t for t in tests}
        for contract in self.registry['contracts']:
            test = by_name[contract['test']]
            command = test['command']
            require(Path(command[0]).resolve() == Path(sys.executable).resolve()
                    and command[1:] == [str(self.root / contract['comparator']), '--cpp',
                                       str(build.resolve() / contract['binary'])], 'actual CTest invocation')
            properties = {p['name']: p['value'] for p in test.get('properties', [])}
            require(self.scope['fixture'] in properties.get('FIXTURES_REQUIRED', []),
                    'registered source prerequisite')

    def native_projection(self, contract, expected, actual, inputs):
        if self.kind == 'query':
            return actual, expected
        if self.kind == 'dynamic_json':
            return self.dynamic_projection(contract, expected, actual)
        name = contract['name']
        if name == 'framing':
            from expression_diagnostic_framing import to_source
            return [dict(raw=to_source(v)) for v in actual], expected
        if name == 'text':
            module = importlib.import_module('compare_expression_diagnostic_text_5_0_6')
            return ([module.native_fields(v) for v in actual],
                    [module.adapt(e['raw'], x) for x, e in zip(inputs, expected)])
        if name == 'generated':
            module = importlib.import_module('compare_generated_expression_diagnostics')
            return actual, [module.adapt(v) for v in expected]
        if name == 'model_types':
            module = importlib.import_module('compare_expression_model_types_5_0_6')
            return [module.adapted(v) for v in actual], expected
        if name == 'roots':
            module = importlib.import_module('compare_expression_roots_5_0_6')
            entries = read(self.root / contract['corpus'])['native_entries']
            return actual, [module.adapted(v, entries) for v in expected]
        require(name == 'scalar', 'known presentation adapter')
        module = importlib.import_module('compare_scalar_diagnostic_repair_5_0_6')
        return actual, [module.adapted(v) for v in expected]

    def validate_native(self, build):
        from compare_values import observe
        if self.kind == 'dynamic_json':
            os.environ.update(TEAQL_BUSINESS_ID_KEY_V1_HEX='00' * 32,
                              TEAQL_BUSINESS_ID_KEY_VERSION='1')
        results = []
        for contract, source in zip(self.registry['contracts'], self.source['results']):
            binary = build / contract['binary']
            actual = observe(str(binary), source['inputs'])
            left, right = self.native_projection(contract, source['observed'], actual, source['inputs'])
            require(self.same(left, right), 'exact current native binding: ' + contract['name'])
            results.append(dict(name=contract['name'],count=len(actual),binary_sha256=digest(binary),
                                inputs=source['inputs'],observed=actual))
        return results

    def dynamic_projection(self, contract, expected, actual):
        adapter = DYNAMIC_JSON_ADAPTERS[contract['name']]
        if adapter == 'column_debug':
            from compare_generated_dynamic_json import cpp_observation
            return actual, [cpp_observation(v) for v in expected]
        if adapter == 'policy_column_debug':
            from generate_dynamic_json_policy_5_0_6 import adapted
            return actual, [adapted(v) for v in expected]
        require(adapter == 'identity', 'known dynamic JSON adapter')
        return actual, expected

    def selected_rows(self, contract, corpus):
        rows = corpus['cases']
        require(type(rows) is list and len(rows) == contract.get('corpus_count', contract['count']),
                'complete retained matrix')
        selection = contract.get('selection')
        if selection is not None:
            require(type(selection) is dict and set(selection) == {'field', 'value', 'complete_values'}
                    and selection['field'] == 'domain'
                    and selection['complete_values'] == ['dynamic', 'inherited']
                    and selection['value'] in selection['complete_values'], 'known original domain partition')
            require({r['input']['domain'] for r in rows} == set(selection['complete_values']),
                    'complete original domain inventory')
            rows = [r for r in rows if r['input']['domain'] == selection['value']]
        require(len(rows) == contract['count'] and rows, 'complete selected source matrix')
        return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--ctest-build-dir', type=Path)
    parser.add_argument('--cpp-build-dir', type=Path)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path, default=REGISTRY)
    args = parser.parse_args()
    require(args.verify_only or args.cpp_build_dir is not None, 'select source or native validation')
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
            complete=True,whole_goal_complete=False,source_datasets=len(validator.registry['contracts']),
            source_observations=sum(len(r['inputs']) for r in validator.source['results']),
            native=native,registry_sha256=digest(args.contracts)),indent=2)+'\n').encode(),mtime=0))
    print('Verified',len(validator.registry['contracts']),'complete original',validator.kind,
          'datasets /',sum(len(r['inputs']) for r in validator.source['results']),'outcomes'
          + (' and exact current native presentations' if native else ''))


if __name__ == '__main__':
    main()

