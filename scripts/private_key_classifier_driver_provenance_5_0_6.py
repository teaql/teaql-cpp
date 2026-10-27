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

ROOT = archive.ROOT
RUNTIME_PACKAGES = {
    'teaql-core', 'teaql-runtime', 'teaql-data-service', 'teaql-sql',
    'teaql-macros', 'teaql-provider-sqlite',
}


def inputs(contract):
    builder = contract['input_builder']
    path = ROOT / builder['path']
    assert archive.digest(path) == builder['sha256'] == contract['input_recipe_sha256']
    if builder['kind'] == 'fixture':
        result = archive.read(path)[builder['name']]
        assert type(result) is list and len(result) == contract['count']
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
            assert len(values) == 1
            expression = values[0]
            assert hashlib.sha256(ast.dump(expression, include_attributes=False).encode()).hexdigest() == builder['ast_sha256']
            result = list(eval(compile(ast.Expression(expression), str(path), 'eval'), vars(module)))
    else:
        assert builder['kind'] == 'statements'
        function = next(n for n in tree.body if isinstance(n, ast.FunctionDef)
                        and n.name == builder['function'])
        selected = function.body[builder['start']:builder['stop']]
        assert selected and all(isinstance(n, (ast.Assign, ast.AugAssign, ast.Expr))
                                for n in selected)
        selected = ast.Module(body=selected, type_ignores=[])
        assert hashlib.sha256(ast.dump(selected, include_attributes=False).encode()).hexdigest() == builder['ast_sha256']
        namespace = {}
        exec(compile(selected, str(path), 'exec'), namespace)
        result = namespace[builder['name']]
    assert type(result) is list and len(result) == contract['count']
    return result



def check_source_recipes(registry):
    """Bind private source slices and generated artifacts without broadening their scope."""
    assert registry['source_recipes'] and len(registry['source_recipes']) == 2
    support = set(registry['support_files'])
    artifact_provenances = []
    assert registry['generated_fixtures'] == []
    for fixture in registry['artifact_fixtures']:
        provenance = archive.read(ROOT / fixture / 'PROVENANCE.json')
        assert provenance['runtime_commit'] == registry['runtime_commit']
        assert provenance['generator_commit'] == registry['generator_commit']
        assert provenance['hand_edited'] is False
        assert archive.digest(ROOT / provenance['model']) == provenance['model_sha256']
        required = {fixture + '/' + name for name in provenance['files']}
        required.update([fixture + '/PROVENANCE.json', provenance['model']])
        assert required <= support
        for name, digest in provenance['files'].items():
            assert archive.digest(ROOT / fixture / name) == digest
        artifact_provenances.append(provenance)
    provenances = []
    kinds = []
    for recipe in registry['source_recipes']:
        kinds.append(recipe['kind'])
        assert recipe['driver'] in {c['driver'] for c in registry['contracts']}
        assert recipe['scope'] == 'private_function_only'
        if recipe['kind'] == 'build_script_slices':
            assert recipe['build_script'] == registry['build_script']['path']
            assert recipe['build_script'] in support
            assert archive.digest(ROOT / recipe['build_script']) == registry['build_script']['sha256']
            boundaries = [(piece['source'], piece['start'], piece['stop'],
                           piece['prefix'], piece['suffix']) for piece in recipe['pieces']]
            assert boundaries == [
                ('teaql-runtime/src/data_service/helpers.rs',
                 'pub(super) fn aggregation_cache_namespace(',
                 'pub(super) fn invalidate_aggregation_cache_namespace(', '', ''),
                ('teaql-runtime/src/data_service/helpers.rs',
                 'pub(super) fn aggregation_cache_key(',
                 'pub(super) fn ensure_projection(', '', ''),
                ('teaql-runtime/src/data_service/resolved.rs',
                 '    fn continuous_page_query_key(',
                 '    async fn prepare_id_set_page(', 'impl KeyRepo {\n', '}\n'),
            ]
            text = ''
            for piece in recipe['pieces']:
                source = ROOT / '.reference/teaql-rs' / piece['source']
                assert archive.digest(source) == piece['source_sha256']
                value = source.read_text()
                start = value.index(piece['start'])
                stop = value.index(piece['stop'], start)
                text += piece['prefix'] + value[start:stop] + piece['suffix']
            assert text == recipe['compiled_source']
            assert hashlib.sha256(text.encode()).hexdigest() == recipe['compiled_source_sha256']
            driver = (ROOT / recipe['driver']).read_text()
            assert 'include!(concat!(env!("OUT_DIR"),"/query_keys.rs"));' in driver
            assert recipe['abi_bits'] == 64
            assert recipe['identity_fixture'] == 'Original Root/Context/Metadata/Service/KeyRepo shells'
        else:
            assert recipe['kind'] == 'retained_generated_function'
            assert recipe['start'] == 'fn is_generated_bootstrap_retryable_conflict('
            assert recipe['stop'] == '\n}\n'
            source = recipe['source']
            assert any(source.startswith(fixture + '/') for fixture in registry['artifact_fixtures'])
            assert archive.digest(ROOT / source) == recipe['source_sha256']
            text = (ROOT / source).read_text()
            start = text.index(recipe['start'])
            stop = text.index(recipe['stop'], start) + len(recipe['stop'])
            extracted = text[start:stop] + '\n'
            assert recipe['retained'] in support and recipe['provenance'] in support
            assert (ROOT / recipe['retained']).read_text() == extracted
            assert hashlib.sha256(extracted.encode()).hexdigest() == recipe['extracted_sha256']
            provenance = archive.read(ROOT / recipe['provenance'])
            assert provenance['source'] == source
            assert provenance['source_sha256'] == recipe['source_sha256']
            assert provenance['extracted_sha256'] == recipe['extracted_sha256']
            assert provenance['runtime_commit'] == registry['runtime_commit']
            assert provenance['generator_commit'] == registry['generator_commit']
            assert 'include!("../bootstrap_classifier_original.rs");' in (ROOT / recipe['driver']).read_text()
            provenances.append(provenance)
    assert kinds == ['build_script_slices', 'retained_generated_function']
    return provenances


def check_inputs(registry):
    assert registry['schema'] == 5
    assert registry['base_verifier_sha256'] == archive.digest(ROOT / 'scripts/generated_driver_provenance_5_0_6.py')
    assert set(registry['runtime_packages']) == RUNTIME_PACKAGES
    assert registry['input_helpers'] == {}  # The shared runner's SQL recipe mode is unused.
    support = set(registry['support_files'])
    assert len(support) == len(registry['support_files'])
    assert registry['support_file_hashes'] == {n: archive.digest(ROOT / n) for n in support}
    for name in [registry['project'], *support, *(c['driver'] for c in registry['contracts'])]:
        path = Path(name)
        assert not path.is_absolute() and '..' not in path.parts
        assert (ROOT / path).is_file()
    provenances = check_source_recipes(registry)
    for fixture in registry['generated_fixtures']:
        path = ROOT / fixture
        provenance = archive.read(path / 'PROVENANCE.json')
        assert provenance['runtime_commit'] == registry['runtime_commit']
        assert provenance['generator_commit'] == registry['generator_commit']
        assert provenance['hand_edited'] is False
        assert archive.digest(ROOT / provenance['model']) == provenance['model_sha256']
        required = {fixture + '/' + n for n in provenance['files']}
        required.update([fixture + '/PROVENANCE.json', provenance['model']])
        assert required <= support
        for name, expected in provenance['files'].items():
            assert archive.digest(path / name) == expected
        provenances.append(provenance)
    contracts = registry['contracts']
    assert contracts and len(contracts) == len({c['name'] for c in contracts})
    assert len({c['native_test'] for c in contracts}) == len(contracts)
    assert len({c['corpus'] for c in contracts}) == len(contracts)
    for contract in contracts:
        assert type(contract['count']) is int and contract['count'] > 0
        assert contract['input_builder']['path'] in support
        corpus = archive.read(ROOT / contract['corpus'])
        assert archive.digest(ROOT / contract['corpus']) == contract['corpus_sha256']
        assert corpus['baseline'] == registry['runtime_commit']
        assert archive.digest(ROOT / contract['driver']) == contract['driver_sha256']
        assert contract['native_command'][0] == str(ROOT / 'build_native_current' / contract['native_target'])
        assert archive.same(inputs(contract), [r['input'] for r in corpus['cases']])
        if 'producer' in corpus:
            assert corpus['producer'] in provenances
        source_root = contract.get('corpus_source_root', 'runtime')
        assert source_root in ['runtime', 'project']
        source_root = ROOT / '.reference/teaql-rs' if source_root == 'runtime' else ROOT
        for key in ['sources', 'source_hashes', 'source_sha256']:
            hashes = corpus.get(key)
            if isinstance(hashes, dict):
                for name, expected in hashes.items():
                    assert not Path(name).is_absolute() and '..' not in Path(name).parts
                    assert archive.digest(source_root / name) == expected
        if isinstance(corpus.get('source'), str) and isinstance(corpus.get('source_sha256'), str):
            assert archive.digest(source_root / corpus['source']) == corpus['source_sha256']
        for key in ['fixtures', 'fixture_hashes']:
            if isinstance(corpus.get(key), dict):
                fixture_root = ROOT / contract.get('corpus_fixture_root', 'tests/support')
                for name, expected in corpus[key].items():
                    assert archive.digest(fixture_root / name) == expected

def validate(record, registry):
    check_inputs(registry)
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry
    assert registry['runtime_commit'] == archive.read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == archive.read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    assert record['input_hashes_before'] == record['input_hashes_after']
    for script in [__file__, archive.__file__, ROOT / 'scripts/generated_driver_provenance_5_0_6.py']:
        path = str(Path(script).relative_to(ROOT))
        assert record['input_hashes_after']['runtime:' + path] == archive.digest(ROOT / path)
    assert record['source_files'] == archive.source_files(registry)
    assert record['input_helpers'] == {}
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_archive_driver_506_')
    assert record['owned_directory_removed'] is True and not owned.exists()
    assert record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'))
    steps = record['steps']
    assert steps and all(s['exit_code'] == 0 for s in steps)
    data = archive.archive_bytes(registry)
    assert record['archive_sha256'] == hashlib.sha256(data).hexdigest()
    originals = archive.archive_files(data)
    assert record['archive_files'] == {n: hashlib.sha256(v).hexdigest() for n, v in originals.items()}
    assert all((ROOT / '.reference/teaql-rs' / n).read_bytes() == v for n, v in originals.items())
    project = owned / registry['project']
    metadata = steps[record['metadata_step']]
    assert metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version',
                                   '1', '--manifest-path', str(project)]
    assert metadata['stdout'] == record['project_metadata']
    packages = json.loads(metadata['stdout'])['packages']
    package = next(p for p in packages if p['manifest_path'] == str(project))
    assert package['name'] == registry['package']
    contracts = registry['contracts']
    bins = [t for t in package['targets'] if t['kind'] == ['bin']]
    builds = [t for t in package['targets'] if t['kind'] == ['custom-build']]
    assert {t['name'] for t in bins} == {c['target'] for c in contracts}
    assert len(package['targets']) == len(bins) + len(builds) and len(builds) == 1
    assert Path(builds[0]['src_path']).resolve() == (owned / registry['build_script']['path']).resolve()
    assert record['source_files'][registry['build_script']['path']] == registry['build_script']['sha256']
    for contract in contracts:
        target = next(t for t in package['targets'] if t['name'] == contract['target'])
        assert Path(target['src_path']).resolve() == (owned / contract['driver']).resolve()
    allowed = {project} | {owned / fixture / 'lib/Cargo.toml' for fixture in registry['generated_fixtures']}
    allowed |= {owned / '.reference/teaql-rs' / path for path in registry['archive_manifests']}
    assert {n + '/Cargo.toml' for n in RUNTIME_PACKAGES} <= set(registry['archive_manifests'])
    assert {Path(p['manifest_path']) for p in packages if p['source'] is None} == allowed
    assert all(p['version'] == '5.0.6' for p in packages if p['name'] in RUNTIME_PACKAGES)
    lock = (ROOT / registry['project']).with_name('Cargo.lock')
    assert record['compiled_lock'] == lock.read_text()
    identities = archive.registry_identities(lock)
    assert identities <= archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    assert {(p['name'], p['version'], p['source']) for p in packages if p['source']} <= {
        (n, v, s) for n, v, s, checksum in identities}
    build = steps[record['build_step']]
    assert build['command'] == ['cargo', 'build', '--offline', '--locked', '--manifest-path',
                                str(project), '--bins', '-j2']
    assert len(record['results']) == len(contracts)
    assert [r['name'] for r in record['results']] == [c['name'] for c in contracts]
    for contract, result in zip(contracts, record['results']):
        corpus = archive.read(ROOT / contract['corpus'])
        assert len(corpus['cases']) == contract['count'] == result['count']
        payload = ''.join(json.dumps(row['input'], ensure_ascii=True) + '\n' for row in corpus['cases'])
        expected = [row['expected'] for row in corpus['cases']]
        native = str(ROOT / 'build_native_current' / contract['native_target'])
        assert contract['registered_command'][contract['registered_command'].index('--cpp') + 1] == native
        for key, command in [('source_step', [str(owned / 'target/debug' / contract['target'])]),
                             ('native_step', contract['native_command'])]:
            step = steps[result[key]]
            assert step['command'] == command
            assert step['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
            assert step['stdin_bytes'] == len(payload.encode())
            assert archive.same(archive.json_lines(step['stdout']), expected)


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
        assert args.record is not None
        archive.replay(registry, args.record)
        validate(archive.read(args.record), registry)
    else:
        assert args.execution is not None
        assert args.record is None or not args.record.exists()
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
