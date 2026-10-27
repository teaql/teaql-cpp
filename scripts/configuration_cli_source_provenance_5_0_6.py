#!/usr/bin/env python3
"""Bind complete configuration source projects, retained matrices and native CLIs."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from configuration_cli_observers_5_0_6 import COLLECTORS, RUNTIME_PACKAGES, helpers, manifest, projects
from custom_public_cli_source_provenance_5_0_6 import digest, require
from profile_public_cli_source_provenance_5_0_6 import lock_identities

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / 'tests/parity/configuration_cli_source_contracts_5_0_6.json'
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, observe, same
from configuration_cli_cases_5_0_6 import bootstrap_inputs, scalar_candidate_inputs
from expression_cli_cases import require_cases
from verify_configuration_cli_recipes_5_0_6 import pure_function


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def selected_rows(root, project, target):
    corpus = read(root / target['corpus'])
    require(corpus['case'] == project['case'], 'original model variant')
    require(corpus['baseline'] == read(root / 'upstream.lock.json')['commit'], 'corpus baseline')
    fixture = root / project['fixture']
    if target['family'] == 'scalar_candidate':
        field = next(f for f in read(fixture / 'OBSERVATION.json')['fields'] if f['field'] == 'choice')
        inputs = scalar_candidate_inputs(len(field['candidates']))
        sql = pure_function(root / COLLECTORS['scalar_candidate'], 'candidate_sql')(project['case'])
        require(corpus['sql'] == sql
                and corpus['sql_sha256'] == hashlib.sha256(sql.encode()).hexdigest(), 'original candidate SQL')
    elif target['family'] == 'constant_topology_extensions':
        inputs = list(pure_function(root / target['comparator'], 'cases')())
    else:
        inputs = bootstrap_inputs()
    if target['family'] != 'constant_topology_extensions':
        require(same(corpus['original_provenance'], read(fixture / 'PROVENANCE.json')),
                'corpus original library provenance')
    return require_cases(corpus, len(inputs), inputs)


class Validator:
    def __init__(self, root, registry):
        self.root = root.resolve()
        self.registry = registry
        self.specs = projects()

    def load(self):
        entry = self.registry['source_execution']
        require(digest(self.root / entry['path']) == entry['sha256'], 'original source receipt identity')
        self.source = read(self.root / entry['path'])
        return self

    def validate_files(self):
        root, r = self.root, self.registry
        require(r['kind'] == 'configuration_cli' and r['projects'] == self.specs,
                'complete original model/driver selection')
        require(read(root / 'upstream.lock.json')['commit'] == r['runtime_commit'], 'runtime baseline')
        for name, commit in [('teaql-rs', r['runtime_commit']), ('teaql_code_gen', r['generator_commit'])]:
            require(subprocess.check_output(['git', '-C', str(root / '.reference' / name),
                                             'rev-parse', 'HEAD'], text=True).strip() == commit,
                    'locked reference identity')
        for name, wanted in {**r['source_files'], **r['support_files']}.items():
            require(digest(root / name) == wanted, 'current original/helper input: ' + name)
        self.generated = []
        for project in self.specs:
            path = root / project['fixture']
            provenance = read(path / 'PROVENANCE.json')
            require(provenance['hand_edited'] is False
                    and provenance['runtime_commit'] == r['runtime_commit']
                    and provenance['generator_commit'] == r['generator_commit'], 'original generated baseline')
            require(digest(root / provenance['model']) == provenance['model_sha256'], 'original model identity')
            actual = {p.relative_to(path).as_posix(): digest(p) for p in path.rglob('*')
                      if p.is_file() and p.name != 'PROVENANCE.json'}
            require(actual == provenance['files'], 'complete original generated file set')
            self.generated.append(dict(path=project['fixture'], provenance=provenance))
        import archive_driver_provenance_5_0_6 as archive
        data = subprocess.check_output(['git', '-C', str(root / '.reference/teaql-rs'),
                                        'archive', r['runtime_commit']])
        self.archive_sha256 = hashlib.sha256(data).hexdigest()
        self.archive_files = {n: hashlib.sha256(v).hexdigest() for n, v in archive.archive_files(data).items()}
        self.reference_lock = (root / '.reference/teaql-rs/Cargo.lock').read_text()
        self.identities = lock_identities(self.reference_lock)
        self.rows = {(p['name'], t['name']): selected_rows(root, p, t)
                     for p in self.specs for t in p['targets']}

    def validate_source(self, source):
        root, r = self.root, self.registry
        require(source['complete'] is True and source['whole_goal_complete'] is False, 'bounded source execution')
        require(source['runtime_commit'] == r['runtime_commit']
                and source['generator_commit'] == r['generator_commit'], 'source execution baseline')
        require(source['input_hashes_before'] == source['input_hashes_after']
                and source['references_before'] == source['references_after']
                and source['references_before'] == {'teaql-rs': r['runtime_commit'],
                                                    'teaql_code_gen': r['generator_commit']}, 'source input epoch')
        require(hashlib.sha256(json.dumps(source['input_hashes_before'], sort_keys=True).encode()).hexdigest()
                == r['input_epoch_sha256'], 'complete recorded input epoch')
        require(source['source_files'] == r['source_files'] and source['projects'] == self.specs,
                'complete copied original input/project selection')
        require(same(source['generated_fixtures'], self.generated), 'complete original generated libraries')
        require(source['archive_sha256'] == self.archive_sha256
                and source['archive_files'] == self.archive_files, 'complete original runtime archive')
        require(source['seed_lock_sha256'] == hashlib.sha256(self.reference_lock.encode()).hexdigest(),
                'reference dependency seed')
        owned = Path(source['owned_directory'])
        require(owned.parent == Path('/tmp') and owned.name.startswith('teaql_configuration_cli_original_506_'),
                'task-owned source directory')
        repository = owned / 'repository'
        require(source['cargo_environment'] == dict(
            CARGO_TARGET_DIR=str(owned / 'target'), CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
            TMPDIR=str(owned / 'children'), TEAQL_LOG_ENDPOINT='off'), 'controlled source build environment')
        require(source['ambient_rust_overrides'] == [], 'no ambient compiler/profile overrides')
        require(len(source['observers']) == 27 and len(source['results']) == 30
                and len(source['steps']) == 111, 'complete original execution cardinality')
        require(all(type(s['exit_code']) is int and s['exit_code'] == 0 for s in source['steps']),
                'successful original steps')
        cursor = result_index = 0
        contracts = self.registry['contracts']
        require(len(contracts) == 30, 'complete source/native bindings')
        for spec, observer in zip(self.specs, source['observers'], strict=True):
            require(observer['name'] == spec['name'], 'original observer model identity')
            project = repository / spec['project']
            cargo_manifest = project / 'Cargo.toml'
            expected_manifest = manifest(root, spec)
            require(observer['manifest'] == expected_manifest, 'exact derived observer manifest')
            require(observer['helpers'] == helpers(root, spec), 'authentic historical call helpers')
            require(observer['driver_copies'] == {t['copy']: r['source_files'][t['driver']]
                                                  for t in spec['targets']}, 'unchanged original drivers')
            pin = r['observer_pins'][spec['name']]
            require(hashlib.sha256(observer['compiled_lock'].encode()).hexdigest() == pin['lock_sha256'],
                    'compiled observer dependency lock')
            identities = lock_identities(observer['compiled_lock'])
            require(identities <= self.identities and observer['registry_identity_differences'] == [],
                    'reference registry identities/checksums')
            require(all(type(observer[k]) is int for k in ('resolve_step', 'metadata_step', 'build_step'))
                    and observer['resolve_step'] == cursor and observer['metadata_step'] == cursor + 1
                    and observer['build_step'] == cursor + 2, 'original observer step order')
            resolve, metadata_step, build_step = source['steps'][cursor:cursor + 3]
            common = ['cargo', 'metadata', '--offline']
            require(resolve['command'] == common + ['--format-version', '1', '--manifest-path', str(cargo_manifest)],
                    'reference-seeded resolution command')
            require(metadata_step['command'] == common + ['--locked', '--format-version', '1',
                                                          '--manifest-path', str(cargo_manifest)]
                    and metadata_step['stdout'] == observer['metadata'], 'actual locked metadata')
            metadata = json.loads(observer['metadata'])
            require(hashlib.sha256(observer['metadata'].encode()).hexdigest() == pin['metadata_sha256'],
                    'complete compiled dependency graph identity')
            domain = next(p for p in metadata['packages']
                          if p['manifest_path'] == str(repository / spec['fixture'] / 'lib/Cargo.toml'))
            expected_local = {name: str(Path('.reference/teaql-rs') / name / 'Cargo.toml')
                              for name in RUNTIME_PACKAGES}
            expected_local.update({domain['name']: spec['fixture'] + '/lib/Cargo.toml',
                                   spec['package']: spec['project'] + '/Cargo.toml'})
            require({p['name']: str(Path(p['manifest_path']).relative_to(repository))
                     for p in metadata['packages'] if p['source'] is None} == expected_local,
                    'complete local runtime/domain/observer packages')
            require({(p['name'], p['version'], p['source']) for p in metadata['packages'] if p['source']}
                    <= {p[:3] for p in identities}, 'resolved registry packages')
            require(all(p['version'] == '5.0.6' for p in metadata['packages']
                        if p['name'] in RUNTIME_PACKAGES), 'original runtime package versions')
            package = next(p for p in metadata['packages'] if p['name'] == spec['package'])
            targets = {t['name']: t for t in package['targets']}
            require(set(targets) == {t['name'] for t in spec['targets']}, 'complete original target set')
            for target in spec['targets']:
                require(targets[target['name']]['kind'] == ['bin']
                        and targets[target['name']]['src_path'] == str(project / target['copy']),
                        'original target source path')
            require(build_step['command'] == ['cargo', 'build', '--offline', '--locked', '--bins',
                                              '--message-format=json', '--manifest-path', str(cargo_manifest), '-j2'],
                    'actual locked original build')
            artifacts = {row['target']['name']: row for row in json_lines(build_step['stdout'])
                         if row.get('reason') == 'compiler-artifact' and row.get('executable') is not None}
            require(same(artifacts, observer['compiler_artifacts']) and set(artifacts) == set(targets),
                    'complete actual compiler artifacts')
            libraries = {p['id']: p for p in metadata['packages']
                         if p['name'] in RUNTIME_PACKAGES or p['id'] == domain['id']}
            compiled = {row['package_id']: row for row in json_lines(build_step['stdout'])
                        if row.get('reason') == 'compiler-artifact'
                        and row['target']['kind'] in (['lib'], ['proc-macro'])
                        and row['package_id'] in libraries}
            require(set(compiled) == set(libraries), 'complete original runtime/domain compilation')
            for identity, row in compiled.items():
                expected_targets = [t for t in libraries[identity]['targets']
                                    if t['kind'] in (['lib'], ['proc-macro'])]
                require(len(expected_targets) == 1 and same(row['target'], expected_targets[0]),
                        'compiled original library source')
                profile = row['profile']
                require(profile['debug_assertions'] is True and profile['overflow_checks'] is True
                        and profile['test'] is False and profile['opt_level'] == '0',
                        'original runtime/domain Debug semantics')
            cursor += 3
            for target in spec['targets']:
                artifact = artifacts[target['name']]
                profile = artifact['profile']
                require(profile['debug_assertions'] is True and profile['overflow_checks'] is True
                        and profile['test'] is False and profile['opt_level'] == '0', 'original Debug semantics')
                require(artifact['package_id'] == package['id'] and artifact['target']['kind'] == ['bin']
                        and artifact['target']['src_path'] == str(project / target['copy'])
                        and artifact['executable'] == str(owned / 'target/debug' / target['name']),
                        'compiled original driver identity')
                result, contract = source['results'][result_index], contracts[result_index]
                result_index += 1
                require({k: contract[k] for k in target} == target
                        and contract['project_name'] == spec['name'], 'exact selected source contract')
                rows = self.rows[(spec['name'], target['name'])]
                require(contract['count'] == len(rows)
                        and digest(root / target['corpus']) == contract['corpus_sha256'], 'complete retained corpus')
                require(result['name'] == target['name'] and result['project_name'] == spec['name']
                        and result['corpus'] == target['corpus']
                        and result['corpus_sha256'] == contract['corpus_sha256'], 'selected original dataset')
                require(same(result['inputs'], [v['input'] for v in rows])
                        and same(result['expected'], [v['expected'] for v in rows]), 'complete original matrix')
                require(result['binary_sha256'] == contract['binary_sha256'], 'original executable identity')
                require(type(result['step']) is int and result['step'] == cursor, 'ordered original execution')
                step = source['steps'][cursor]
                require(step['command'] == [artifact['executable']], 'actual original execution command')
                require(same(json_lines(step['stdout']), result['observed'])
                        and same(result['observed'], result['expected']), 'complete exact original raw stream')
                cursor += 1
        require(cursor == len(source['steps']) and result_index == 30, 'all source executions consumed')
        require(source['owned_directory_removed'] is True and not owned.exists()
                and type(source['owned_bytes_removed']) is int and source['owned_bytes_removed'] > 0,
                'completed source cleanup')

    def validate_commands(self, registry, build):
        expected_names = {c['test'] for c in self.registry['contracts']}
        scripts = {str(self.root / c['comparator']) for c in self.registry['contracts']}
        selected = {t['name']: t for t in registry['tests']
                    if len(t.get('command', [])) > 1 and t['command'][1] in scripts}
        require(set(selected) == expected_names, 'complete actual CTest configuration selection')
        for contract in self.registry['contracts']:
            test = selected[contract['test']]
            command = test['command']
            expected = [str(self.root / contract['comparator']), '--case', contract['case'],
                        '--cpp', str(build.resolve() / contract['binary'])]
            if contract['family'] == 'scalar_candidate':
                expected.append('--cpp-emulator=')
            require(Path(command[0]).resolve() == Path(sys.executable).resolve() and command[1:] == expected,
                    'actual selected CTest arguments')
            properties = {p['name']: p['value'] for p in test.get('properties', [])}
            require('configuration_cli_source' in properties.get('FIXTURES_REQUIRED', []),
                    'required original source prerequisite')

    def validate_ctest(self, build):
        registry = json.loads(subprocess.check_output(['ctest', '--test-dir', str(build),
                                                       '--show-only=json-v1'], text=True))
        self.validate_commands(registry, build)

    def validate_native(self, build):
        os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
        results = []
        for contract, source in zip(self.registry['contracts'], self.source['results'], strict=True):
            binary = build / contract['binary']
            actual = observe(str(binary), source['inputs'])
            require(same(actual, source['observed']), 'complete exact native binding: ' + contract['name'])
            results.append(dict(name=contract['name'], binary_sha256=digest(binary),
                                inputs=source['inputs'], observed=actual))
        return results


def main():
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
    native = validator.validate_native(args.cpp_build_dir) if args.cpp_build_dir else []
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, source_projects=27, source_datasets=30,
            source_observations=5080, native=native, registry_sha256=digest(args.contracts)),
            indent=2) + '\n').encode(), mtime=0))
    print('Verified 27 original configuration projects / 30 datasets / 5080 outcomes'
          + (' and exact current native binding' if native else ''))


if __name__ == '__main__':
    main()
