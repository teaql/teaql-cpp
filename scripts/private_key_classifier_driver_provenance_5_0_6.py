#!/usr/bin/env python3
"""Bind unchanged private key/classifier functions and their explicit fixture scopes."""
import argparse
import ast
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path

import archive_driver_provenance_5_0_6 as archive
import private_key_classifier_source_transition_5_0_6 as transition

ROOT = archive.ROOT
RUNTIME_PACKAGES = {
    'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
    'teaql-macros', 'teaql-provider-sqlite',
}


def inputs(contract):
    builder = contract['input_builder']
    path = ROOT / builder['path']
    if not (archive.digest(path) == builder['sha256'] == contract['input_recipe_sha256']):
        raise AssertionError()
    if builder['kind'] == 'fixture':
        result = archive.read(path)[builder['name']]
        if not (type(result) is list and len(result) == contract['count']):
            raise AssertionError()
        return result
    source = path.read_text()
    tree = ast.parse(source)
    if builder['kind'] in ['function', 'expression']:
        spec = importlib.util.spec_from_file_location('_source_inputs_' + contract['name'], path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        if builder['kind'] == 'function':
            result = list(getattr(module, builder['name'])())
        else:
            function = next(n for n in tree.body if isinstance(n, ast.FunctionDef)
                            and n.name == builder['function'])
            values = [n.value for n in ast.walk(function) if isinstance(n, ast.Assign)
                      and any(isinstance(t, ast.Name) and t.id == builder['name']
                              for t in n.targets)]
            if not (len(values) == 1):
                raise AssertionError()
            expression = values[0]
            if not (hashlib.sha256(ast.dump(expression, include_attributes=False).encode()).hexdigest() == builder['ast_sha256']):
                raise AssertionError()
            result = list(eval(compile(ast.Expression(expression), str(path), 'eval'), vars(module)))
    else:
        if not (builder['kind'] == 'statements'):
            raise AssertionError()
        function = next(n for n in tree.body if isinstance(n, ast.FunctionDef)
                        and n.name == builder['function'])
        selected = function.body[builder['start']:builder['stop']]
        if not (selected and all(isinstance(n, (ast.Assign, ast.AugAssign, ast.Expr))
                                for n in selected)):
            raise AssertionError()
        selected = ast.Module(body=selected, type_ignores=[])
        if not (hashlib.sha256(ast.dump(selected, include_attributes=False).encode()).hexdigest() == builder['ast_sha256']):
            raise AssertionError()
        namespace = {}
        exec(compile(selected, str(path), 'exec'), namespace)
        result = namespace[builder['name']]
    if not (type(result) is list and len(result) == contract['count']):
        raise AssertionError()
    return result



def check_source_recipes(registry):
    """Bind private source slices and generated artifacts without broadening their scope."""
    if not (registry['source_recipes'] and len(registry['source_recipes']) == 2):
        raise AssertionError()
    support = set(registry['support_files'])
    artifact_provenances = []
    if not (registry['generated_fixtures'] == []):
        raise AssertionError()
    for fixture in registry['artifact_fixtures']:
        provenance = archive.read(ROOT / fixture / 'PROVENANCE.json')
        if not (provenance['runtime_commit'] == registry['runtime_commit']):
            raise AssertionError()
        if not (provenance['generator_commit'] == registry['generator_commit']):
            raise AssertionError()
        if not (provenance['hand_edited'] is False):
            raise AssertionError()
        if not (archive.digest(ROOT / provenance['model']) == provenance['model_sha256']):
            raise AssertionError()
        required = {fixture + '/' + name for name in provenance['files']}
        required.update([fixture + '/PROVENANCE.json', provenance['model']])
        if not (required <= support):
            raise AssertionError()
        for name, digest in provenance['files'].items():
            if not (archive.digest(ROOT / fixture / name) == digest):
                raise AssertionError()
        artifact_provenances.append(provenance)
    provenances = []
    kinds = []
    for recipe in registry['source_recipes']:
        kinds.append(recipe['kind'])
        if not (recipe['driver'] in {c['driver'] for c in registry['contracts']}):
            raise AssertionError()
        if not (recipe['scope'] == 'private_function_only'):
            raise AssertionError()
        if recipe['kind'] == 'build_script_slices':
            if not (recipe['build_script'] == registry['build_script']['path']):
                raise AssertionError()
            if not (recipe['build_script'] in support):
                raise AssertionError()
            if not (archive.digest(ROOT / recipe['build_script']) == registry['build_script']['sha256']):
                raise AssertionError()
            boundaries = [(piece['source'], piece['start'], piece['stop'],
                           piece['prefix'], piece['suffix']) for piece in recipe['pieces']]
            if not (boundaries == [
                ('teaql-runtime/src/data_service/helpers.rs',
                 'pub(super) fn aggregation_cache_namespace(',
                 'pub(super) fn invalidate_aggregation_cache_namespace(', '', ''),
                ('teaql-runtime/src/data_service/helpers.rs',
                 'pub(super) fn aggregation_cache_key(',
                 'pub(super) fn ensure_projection(', '', ''),
                ('teaql-runtime/src/data_service/resolved.rs',
                 '    fn continuous_page_query_key(',
                 '    async fn prepare_id_set_page(', 'impl KeyRepo {\n', '}\n'),
            ]):
                raise AssertionError()
            text = ''
            for piece in recipe['pieces']:
                source = ROOT / '.reference/teaql-rs' / piece['source']
                if not (archive.digest(source) == piece['source_sha256']):
                    raise AssertionError()
                value = source.read_text()
                start = value.index(piece['start'])
                stop = value.index(piece['stop'], start)
                text += piece['prefix'] + value[start:stop] + piece['suffix']
            if not (text == recipe['compiled_source']):
                raise AssertionError()
            if not (hashlib.sha256(text.encode()).hexdigest() == recipe['compiled_source_sha256']):
                raise AssertionError()
            driver = (ROOT / recipe['driver']).read_text()
            if not ('include!(concat!(env!("OUT_DIR"),"/query_keys.rs"));' in driver):
                raise AssertionError()
            if not (recipe['abi_bits'] == 64):
                raise AssertionError()
            if not (recipe['identity_fixture'] == 'Original Root/Context/Metadata/Service/KeyRepo shells'):
                raise AssertionError()
        else:
            if not (recipe['kind'] == 'retained_generated_function'):
                raise AssertionError()
            if not (recipe['start'] == 'fn is_generated_bootstrap_retryable_conflict('):
                raise AssertionError()
            if not (recipe['stop'] == '\n}\n'):
                raise AssertionError()
            source = recipe['source']
            if not (any(source.startswith(fixture + '/') for fixture in registry['artifact_fixtures'])):
                raise AssertionError()
            if not (archive.digest(ROOT / source) == recipe['source_sha256']):
                raise AssertionError()
            text = (ROOT / source).read_text()
            start = text.index(recipe['start'])
            stop = text.index(recipe['stop'], start) + len(recipe['stop'])
            extracted = text[start:stop] + '\n'
            if not (recipe['retained'] in support and recipe['provenance'] in support):
                raise AssertionError()
            if not ((ROOT / recipe['retained']).read_text() == extracted):
                raise AssertionError()
            if not (hashlib.sha256(extracted.encode()).hexdigest() == recipe['extracted_sha256']):
                raise AssertionError()
            provenance = archive.read(ROOT / recipe['provenance'])
            if not (provenance['source'] == source):
                raise AssertionError()
            if not (provenance['source_sha256'] == recipe['source_sha256']):
                raise AssertionError()
            if not (provenance['extracted_sha256'] == recipe['extracted_sha256']):
                raise AssertionError()
            if not (provenance['runtime_commit'] == registry['runtime_commit']):
                raise AssertionError()
            if not (provenance['generator_commit'] == registry['generator_commit']):
                raise AssertionError()
            if not ('include!("../bootstrap_classifier_original.rs");' in (ROOT / recipe['driver']).read_text()):
                raise AssertionError()
            provenances.append(provenance)
    if not (kinds == ['build_script_slices', 'retained_generated_function']):
        raise AssertionError()
    return provenances


def check_inputs(registry):
    if not (registry['schema'] == 5):
        raise AssertionError()
    if not (registry['base_verifier_sha256'] == archive.digest(ROOT / 'scripts/generated_driver_provenance_5_0_6.py')):
        raise AssertionError()
    if not (set(registry['runtime_packages']) == RUNTIME_PACKAGES):
        raise AssertionError()
    if not (registry['input_helpers'] == {}):
        raise AssertionError()
    support = set(registry['support_files'])
    if not (len(support) == len(registry['support_files'])):
        raise AssertionError()
    transition.verify_support(registry['support_file_hashes'],
                              {n: archive.digest(ROOT / n) for n in support})
    for name in [registry['project'], *support, *(c['driver'] for c in registry['contracts'])]:
        path = Path(name)
        if not (not path.is_absolute() and '..' not in path.parts):
            raise AssertionError()
        if not ((ROOT / path).is_file()):
            raise AssertionError()
    provenances = check_source_recipes(registry)
    for fixture in registry['generated_fixtures']:
        path = ROOT / fixture
        provenance = archive.read(path / 'PROVENANCE.json')
        if not (provenance['runtime_commit'] == registry['runtime_commit']):
            raise AssertionError()
        if not (provenance['generator_commit'] == registry['generator_commit']):
            raise AssertionError()
        if not (provenance['hand_edited'] is False):
            raise AssertionError()
        if not (archive.digest(ROOT / provenance['model']) == provenance['model_sha256']):
            raise AssertionError()
        required = {fixture + '/' + n for n in provenance['files']}
        required.update([fixture + '/PROVENANCE.json', provenance['model']])
        if not (required <= support):
            raise AssertionError()
        for name, expected in provenance['files'].items():
            if not (archive.digest(path / name) == expected):
                raise AssertionError()
        provenances.append(provenance)
    contracts = registry['contracts']
    if not (contracts and len(contracts) == len({c['name'] for c in contracts})):
        raise AssertionError()
    if not (len({c['native_test'] for c in contracts}) == len(contracts)):
        raise AssertionError()
    if not (len({c['corpus'] for c in contracts}) == len(contracts)):
        raise AssertionError()
    for contract in contracts:
        if not (type(contract['count']) is int and contract['count'] > 0):
            raise AssertionError()
        if not (contract['input_builder']['path'] in support):
            raise AssertionError()
        corpus = archive.read(ROOT / contract['corpus'])
        if not (archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']):
            raise AssertionError()
        if not (corpus['baseline'] == registry['runtime_commit']):
            raise AssertionError()
        if not (archive.digest(ROOT / contract['driver']) == contract['driver_sha256']):
            raise AssertionError()
        if not (contract['native_command'][0] == str(ROOT / 'build_native_current' / contract['native_target'])):
            raise AssertionError()
        if not (archive.same(inputs(contract), [r['input'] for r in corpus['cases']])):
            raise AssertionError()
        if 'producer' in corpus:
            if not (corpus['producer'] in provenances):
                raise AssertionError()
        source_root = contract.get('corpus_source_root', 'runtime')
        if not (source_root in ['runtime', 'project']):
            raise AssertionError()
        source_root = ROOT / '.reference/teaql-rs' if source_root == 'runtime' else ROOT
        for key in ['sources', 'source_hashes', 'source_sha256']:
            hashes = corpus.get(key)
            if isinstance(hashes, dict):
                for name, expected in hashes.items():
                    if not (not Path(name).is_absolute() and '..' not in Path(name).parts):
                        raise AssertionError()
                    if not (archive.digest(source_root / name) == expected):
                        raise AssertionError()
        if isinstance(corpus.get('source'), str) and isinstance(corpus.get('source_sha256'), str):
            if not (archive.digest(source_root / corpus['source']) == corpus['source_sha256']):
                raise AssertionError()
        for key in ['fixtures', 'fixture_hashes']:
            if isinstance(corpus.get(key), dict):
                fixture_root = ROOT / contract.get('corpus_fixture_root', 'tests/support')
                for name, expected in corpus[key].items():
                    if not (archive.digest(fixture_root / name) == expected):
                        raise AssertionError()

def validate(record, registry):
    check_inputs(registry)
    if not (record['complete'] is True and record['whole_goal_complete'] is False):
        raise AssertionError()
    if not (record['registry'] == registry):
        raise AssertionError()
    if not (registry['runtime_commit'] == archive.read(ROOT / 'upstream.lock.json')['commit']):
        raise AssertionError()
    if not (registry['generator_commit'] == archive.read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']):
        raise AssertionError()
    if not (record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}):
        raise AssertionError()
    if not (record['input_hashes_before'] == record['input_hashes_after']):
        raise AssertionError()
    receipt_mode = transition.verify_source_files(record['source_files'],
                                                  archive.source_files(registry))
    for script in [__file__, archive.__file__, ROOT / 'scripts/generated_driver_provenance_5_0_6.py']:
        path = str(Path(script).relative_to(ROOT))
        transition.verify_runtime_hash(path, record['input_hashes_after']['runtime:' + path],
                                       archive.digest(ROOT / path), receipt_mode)
    if not (record['input_helpers'] == {}):
        raise AssertionError()
    owned = Path(record['owned_directory'])
    if not (owned.parent == Path('/tmp') and owned.name.startswith('teaql_archive_driver_506_')):
        raise AssertionError()
    if not (record['owned_directory_removed'] is True and not owned.exists()):
        raise AssertionError()
    if not (record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'))):
        raise AssertionError()
    steps = record['steps']
    if not (steps and all(s['exit_code'] == 0 for s in steps)):
        raise AssertionError()
    data = archive.archive_bytes(registry)
    if not (record['archive_sha256'] == hashlib.sha256(data).hexdigest()):
        raise AssertionError()
    originals = archive.archive_files(data)
    if not (record['archive_files'] == {n: hashlib.sha256(v).hexdigest() for n, v in originals.items()}):
        raise AssertionError()
    if not (all((ROOT / '.reference/teaql-rs' / n).read_bytes() == v for n, v in originals.items())):
        raise AssertionError()
    project = owned / registry['project']
    metadata = steps[record['metadata_step']]
    if not (metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                   '1', '--manifest-path', str(project)]):
        raise AssertionError()
    if not (metadata['stdout'] == record['project_metadata']):
        raise AssertionError()
    packages = json.loads(metadata['stdout'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    if not (package['name'] == registry['package']):
        raise AssertionError()
    contracts = registry['contracts']
    bins = [t for t in package['targets'] if t['kind'] == ['bin']]
    builds = [t for t in package['targets'] if t['kind'] == ['custom-build']]
    if not ({t['name'] for t in bins} == {c['target'] for c in contracts}):
        raise AssertionError()
    if not (len(package['targets']) == len(bins) + len(builds) and len(builds) == 1):
        raise AssertionError()
    if not (Path(builds[0]['src_path']).resolve() == (owned / registry['build_script']['path']).resolve()):
        raise AssertionError()
    if not (record['source_files'][registry['build_script']['path']] == registry['build_script']['sha256']):
        raise AssertionError()
    for contract in contracts:
        target = next(t for t in package['targets'] if t['name'] == contract['target'])
        if not (Path(target['src_path']).resolve() == (owned / contract['driver']).resolve()):
            raise AssertionError()
    allowed = {project} | {owned / fixture / 'lib/Cargo.toml' for fixture in registry['generated_fixtures']}
    allowed |= {owned / '.reference/teaql-rs' / path for path in registry['archive_manifests']}
    if not ({n + '/Cargo.toml' for n in RUNTIME_PACKAGES} <= set(registry['archive_manifests'])):
        raise AssertionError()
    if not ({Path(p['manifest_path']) for p in packages if p['source'] is None} == allowed):
        raise AssertionError()
    if not (all(p['version'] == '5.0.6' for p in packages if p['name'] in RUNTIME_PACKAGES)):
        raise AssertionError()
    lock = (ROOT / registry['project']).with_name('Cargo.lock')
    if not (record['compiled_lock'] == lock.read_text()):
        raise AssertionError()
    identities = archive.registry_identities(lock)
    if not (identities <= archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')):
        raise AssertionError()
    if not ({(p['name'], p['version'], p['source']) for p in packages if p['source']} <= {
        (n, v, s) for n, v, s, checksum in identities}):
        raise AssertionError()
    build = steps[record['build_step']]
    if not (build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project), '--bins', '-j2']):
        raise AssertionError()
    if not (len(record['results']) == len(contracts)):
        raise AssertionError()
    if not ([r['name'] for r in record['results']] == [c['name'] for c in contracts]):
        raise AssertionError()
    for contract, result in zip(contracts, record['results']):
        corpus = archive.read(ROOT / contract['corpus'])
        if not (len(corpus['cases']) == contract['count'] == result['count']):
            raise AssertionError()
        payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n' for row in corpus['cases'])
        expected = [row['expected'] for row in corpus['cases']]
        native = str(ROOT / 'build_native_current' / contract['native_target'])
        if not (contract['registered_command'][contract['registered_command'].index('--cpp') + 1] == native):
            raise AssertionError()
        for key, command in [('source_step', [str(owned / 'target/debug' / contract['target'])]),
                             ('native_step', contract['native_command'])]:
            step = steps[result[key]]
            if not (step['command'] == command):
                raise AssertionError()
            if not (step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()):
                raise AssertionError()
            if not (step['stdin_bytes'] == len(payload.encode())):
                raise AssertionError()
            if not (archive.same(archive.json_lines(step['stdout']), expected)):
                raise AssertionError()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['replay', 'verify'])
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    registry = archive.read(args.contracts)
    check_inputs(registry)
    if args.mode == 'replay':
        transition.require_replay_mode()
        if not (args.record is not None):
            raise AssertionError()
        archive.replay(registry, args.record)
        validate(archive.read(args.record), registry)
    else:
        if not (args.execution is not None):
            raise AssertionError()
        if not (args.record is None or not args.record.exists()):
            raise AssertionError()
        validate(archive.read(args.execution), registry)
        if args.record:
            result = dict(complete=True, whole_goal_complete=False,
                source_contracts=len(registry['contracts']),
                compiled_targets=len({c['target'] for c in registry['contracts']}),
                observations=sum(c['count'] for c in registry['contracts']),
                execution_sha256=archive.digest(args.execution),
                registry_sha256=archive.digest(args.contracts))
            args.record.write_bytes(gzip.compress(json.dumps(result).encode(), mtime=0))
    print('Complete private-function source/input/dependency/result bindings verify.')


if __name__ == '__main__':
    main()
