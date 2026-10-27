#!/usr/bin/env python3
"""Verify locked generator references and ledger integrity, not behavioral parity."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
from reproduce_cpp_contract_fixtures import METADATA_HELPER_INPUTS, metadata_manifest_paths

ROOT = Path(__file__).resolve().parents[1]
GENERATED_FIXTURES = ['business_ids', 'types', 'relations', 'optional_types', 'query_field_branches',
                      'typed_member_aliases',
                      'checkers', 'checker_unique_string', 'checker_clock_scalars', 'checker_clock_composition',
                      'inheritance', 'inheritance_graph', 'inheritance_variants',
                      'inheritance_business_ids', 'candidates', 'constants_scalar', 'constants_root',
                      'constants_numeric', 'constants_catalog', 'constants_temporal', 'constants_temporal_required',
                      'dynamic_json', 'sample_fields', 'sample_fields_runtime', 'sample_fields_temporal']


def git(reference, *arguments):
    return subprocess.check_output(['git', '-C', str(reference), *arguments])


def verify_metadata_helper_inputs(manifest, source_directory, label):
    inputs = manifest.get('generator_implementation_sha256', {})
    missing = set(METADATA_HELPER_INPUTS) - inputs.keys()
    if missing:
        raise SystemExit(f'Missing shared metadata producer inputs in {label}: {sorted(missing)}')
    for path in METADATA_HELPER_INPUTS:
        if inputs[path] != hashlib.sha256((source_directory / path).read_bytes()).hexdigest():
            raise SystemExit(f'Stale shared metadata producer input in {label}: {path}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generator-dir', type=Path,
                        default=ROOT.parent / 'teaql_code_gen_cpp')
    parser.add_argument('--reference', type=Path,
                        default=ROOT / '.reference/teaql_code_gen')
    parser.add_argument('--require-complete', action='store_true')
    parser.add_argument('--skip-business-id-fixture', action='store_true',
                        help='For controlled original-fixture regeneration only')
    parser.add_argument('--regenerating-fixtures', nargs='+', choices=GENERATED_FIXTURES,
                        default=[], help='Skip only the specified artifacts during controlled regeneration')
    parser.add_argument('--regenerating-cpp-scalar-fixtures', action='store_true',
                        help='Verify retained scalar bytes but defer producer hashes during controlled C++ regeneration')
    parser.add_argument('--regenerating-constant-metadata-fixtures', action='store_true',
                        help='Preserve original metadata checks but defer native artifacts during controlled regeneration')
    args = parser.parse_args()
    runtime = json.loads((ROOT / 'upstream.lock.json').read_text())
    producer = json.loads((args.generator_dir / 'cpp_generator.lock.json').read_text())
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    if ledger['runtime_commit'] != runtime['commit']:
        raise SystemExit('Generator ledger uses a different runtime baseline')
    if (ledger['generator_commit'] != producer['generator_baseline_commit'] or
            producer['runtime_rust_baseline'] != runtime['commit']):
        raise SystemExit('Generator lock and capability ledger disagree')
    if git(args.reference, 'rev-parse', 'HEAD').decode().strip() != ledger['generator_commit']:
        raise SystemExit('Generator reference HEAD differs from its lock')
    if git(args.reference, 'status', '--porcelain').strip():
        raise SystemExit('Generator reference must be clean')
    for path, entry in ledger['sources'].items():
        data = git(args.reference, 'show', ledger['generator_commit'] + ':' + path)
        if hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise SystemExit(f'Stale locked generator source: {path}')
        lines = data.decode().splitlines()
        for declaration in entry['public_declarations']:
            if lines[declaration['line'] - 1].strip() != declaration['source']:
                raise SystemExit(f'Stale source anchor: {path}:{declaration["line"]}')
    provenance = json.loads(git(ROOT / '.reference/teaql-rs', 'show',
                               runtime['commit'] + ':' + ledger['release_generated_fixture']))
    if (provenance['producerCommit'] != ledger['generator_commit'] or
            provenance['runtimeVersion'] != runtime['version'] or provenance['handEdited']):
        raise SystemExit('Release fixture provenance differs from the generator/runtime locks')
    allowed = {'missing', 'partial', 'rejected', 'pending_audit', 'pending_review',
               'implemented', 'qualified'}
    ids = set()
    pending = []
    for entry in ledger['capabilities']:
        if entry['id'] in ids or entry['status'] not in allowed:
            raise SystemExit(f'Duplicate or invalid capability: {entry["id"]}')
        ids.add(entry['id'])
        if not set(entry['rust_sources']) <= ledger['sources'].keys():
            raise SystemExit(f'Unindexed source reference: {entry["id"]}')
        for path, digest in entry.get('runtime_source_hashes', {}).items():
            data = git(ROOT / '.reference/teaql-rs', 'show', runtime['commit'] + ':' + path)
            if hashlib.sha256(data).hexdigest() != digest:
                raise SystemExit(f'Stale runtime dependency: {entry["id"]}:{path}')
        if entry['status'] == 'qualified':
            if not entry['cpp_api_mapping'] or not entry['evidence']:
                raise SystemExit(f'Qualification needs API mappings and evidence: {entry["id"]}')
            for evidence in entry['evidence']:
                path = evidence.get('path') if isinstance(evidence, dict) else evidence
                if not isinstance(path, str) or not (ROOT / path).is_file():
                    raise SystemExit(f'Missing capability evidence: {evidence}')
        else:
            pending.append(entry['id'])
    fixtures = json.loads((ROOT / 'tests/generator/parity/provenance.json').read_text())
    for entry in fixtures['fixtures']:
        path = ROOT / 'tests/generator/parity' / entry['fixture']
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise SystemExit(f'Changed locked model input: {entry["fixture"]}')
        reference = (ROOT / '.reference/teaql-rs' if entry['source_commit'] == runtime['commit']
                     else args.reference)
        original = git(reference, 'show', entry['source_commit'] + ':' + entry['source'])
        if data != original or not set(entry['capabilities']) <= ids:
            raise SystemExit(f'Invalid model input provenance: {entry["fixture"]}')
    generated = ROOT / 'tests/generator/parity/business_ids_rust'
    if (args.skip_business_id_fixture or args.regenerating_fixtures or args.regenerating_cpp_scalar_fixtures
            or args.regenerating_constant_metadata_fixtures) and args.require_complete:
        raise SystemExit('Completion cannot skip generated fixture verification')
    if args.skip_business_id_fixture:
        print(f'Verified locked sources; original Business-ID fixture is being regenerated')
        return
    for fixture in GENERATED_FIXTURES:
        if fixture in args.regenerating_fixtures:
            continue
        generated = ROOT / 'tests/generator/parity' / f'{fixture}_rust'
        provenance = json.loads((generated / 'PROVENANCE.json').read_text())
        if (provenance['generator_commit'] != ledger['generator_commit'] or
                provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited'] or
                hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() !=
                provenance['model_sha256']):
            raise SystemExit(f'{fixture} original generated fixture provenance differs')
        actual = {path.relative_to(generated).as_posix() for path in generated.rglob('*')
                  if path.is_file() and path.name != 'PROVENANCE.json'}
        if actual != set(provenance['files']):
            raise SystemExit(f'{fixture} original generated fixture file set differs')
        for path, digest in provenance['files'].items():
            if hashlib.sha256((generated / path).read_bytes()).hexdigest() != digest:
                raise SystemExit(f'Changed original generated {fixture} fixture: {path}')
    aliases = ROOT / 'tests/generator/parity/query_alias_contracts'
    for language in ('rust', 'cpp', 'cpp_before_alias_fix', 'cpp_before_identity_fix', 'cpp_before_relation_fix'):
        output = aliases / language
        manifest = json.loads((output / 'PROVENANCE.json').read_text())
        assert manifest['generator_commit'] == ledger['generator_commit']
        assert manifest['runtime_commit'] == runtime['commit']
        assert not manifest['hand_edited']
        assert manifest['model_sha256'] == hashlib.sha256((aliases / 'model.xml').read_bytes()).hexdigest()
        actual = {str(p.relative_to(output)): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
        assert actual == manifest['files'], language
        if language != 'rust':
            path = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
            if language == 'cpp':
                code = (args.generator_dir / path).read_bytes()
            else:
                snapshot = {'cpp_before_alias_fix': 'query_alias_generator_before_fix_5_0_6.java.gz',
                            'cpp_before_identity_fix': 'query_alias_generator_before_identity_fix_5_0_6.java.gz',
                            'cpp_before_relation_fix': 'relation_identity_generator_before_fix_5_0_6.java.gz'}[language]
                code = gzip.decompress((ROOT / 'docs/parity' / snapshot).read_bytes())
            assert manifest['generator_implementation_sha256'][path] == hashlib.sha256(code).hexdigest()
            if language == 'cpp':
                template = 'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template'
                assert manifest['generator_implementation_sha256'][template] == hashlib.sha256((args.generator_dir / template).read_bytes()).hexdigest()
    relationships = ROOT / 'tests/generator/parity/relation_identity_contracts'
    for language in ('rust', 'cpp', 'cpp_before_relation_fix', 'cpp_before_descriptor_order_fix'):
        output = relationships / language
        manifest = json.loads((output / 'PROVENANCE.json').read_text())
        assert manifest['generator_commit'] == ledger['generator_commit']
        assert manifest['runtime_commit'] == runtime['commit'] and not manifest['hand_edited']
        assert manifest['model_sha256'] == hashlib.sha256((relationships / 'model.xml').read_bytes()).hexdigest()
        actual = {str(p.relative_to(output)): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
        assert actual == manifest['files'], language
        if language != 'rust':
            path = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
            if language == 'cpp':
                code = (args.generator_dir / path).read_bytes()
            else:
                snapshot = ('relation_identity_generator_before_fix_5_0_6.java.gz'
                            if language == 'cpp_before_relation_fix' else
                            'relation_identity_generator_before_descriptor_order_5_0_6.java.gz')
                code = gzip.decompress((ROOT / 'docs/parity' / snapshot).read_bytes())
            assert manifest['generator_implementation_sha256'][path] == hashlib.sha256(code).hexdigest()
            if language == 'cpp':
                template = 'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template'
                assert manifest['generator_implementation_sha256'][template] == hashlib.sha256((args.generator_dir / template).read_bytes()).hexdigest()
    expression_roots = ROOT / 'tests/generator/parity/expression_root_contracts'
    retained_roots = json.loads(gzip.decompress((ROOT / 'docs/parity/expression_entry_sdks_before_fix_5_0_6.json.gz').read_bytes()))['tests/generator/parity/expression_root_contracts/cpp']
    for language in ('rust', 'cpp', 'cpp_before_entry_fix'):
        output = expression_roots / language
        provenance = json.loads((output / 'PROVENANCE.json').read_text())
        assert provenance['generator_commit'] == ledger['generator_commit']
        assert provenance['runtime_commit'] == runtime['commit'] and not provenance['hand_edited']
        assert provenance['model_sha256'] == hashlib.sha256((expression_roots / 'model.xml').read_bytes()).hexdigest()
        actual = {str(p.relative_to(output)): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
        assert actual == provenance['files'], language
        if language == 'cpp':
            for path, digest in provenance['generator_implementation_sha256'].items():
                assert hashlib.sha256((args.generator_dir / path).read_bytes()).hexdigest() == digest, path
        elif language == 'cpp_before_entry_fix':
            assert json.loads(retained_roots['PROVENANCE.json']) == provenance
            assert actual == {name: hashlib.sha256(text.encode()).hexdigest()
                              for name, text in retained_roots.items() if name != 'PROVENANCE.json'}
    scalar_candidates = ROOT / 'tests/generator/parity/scalar_candidate_contracts'
    candidate_cases = {'text', 'integer', 'nullable_integer', 'number', 'double',
                       'date', 'timestamp', 'boolean', 'bool_actual'}
    if {p.name for p in scalar_candidates.iterdir() if p.is_dir()} != candidate_cases:
        raise SystemExit('Scalar candidate fixture set differs')
    for case in sorted(candidate_cases):
        for language in ('rust', 'cpp'):
            output = scalar_candidates / case / language
            provenance = json.loads((output / 'PROVENANCE.json').read_text())
            model = scalar_candidates / case / 'model.xml'
            if (provenance['generator_commit'] != ledger['generator_commit']
                    or provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited']
                    or provenance['model'] != model.relative_to(ROOT).as_posix()
                    or provenance['model_sha256'] != hashlib.sha256(model.read_bytes()).hexdigest()):
                raise SystemExit(f'Scalar candidate {case}/{language} provenance differs')
            actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
            if actual != provenance['files']:
                raise SystemExit(f'Scalar candidate {case}/{language} file set or bytes differ')
            for path, digest in provenance.get('generator_implementation_sha256', {}).items():
                if language == 'cpp' and args.regenerating_cpp_scalar_fixtures:
                    continue
                if hashlib.sha256((args.generator_dir / path).read_bytes()).hexdigest() != digest:
                    raise SystemExit(f'Scalar candidate {case}/{language} producer implementation differs: {path}')
    configured_types = ROOT / 'tests/generator/parity/type_configuration_contracts'
    type_cases = {'unsigned_recipe', 'field_parameter', 'string_alias', 'numeric_alias',
                  'integer_as_unsigned', 'custom_unsigned', 'integer_as_string',
                  'equivalent_integer', 'equivalent_string_alias'}
    if {p.name for p in configured_types.iterdir() if p.is_dir()} != type_cases:
        raise SystemExit('Type configuration fixture set differs')
    for case in sorted(type_cases):
        languages = ['rust', 'cpp']
        if case in {'integer_as_unsigned', 'custom_unsigned', 'integer_as_string'}:
            languages.append('cpp_before_storage_guard')
        for language in languages:
            output = configured_types / case / language
            provenance = json.loads((output / 'PROVENANCE.json').read_text())
            model = configured_types / case / 'model.xml'
            if (provenance['generator_commit'] != ledger['generator_commit']
                    or provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited']
                    or provenance['model'] != model.relative_to(ROOT).as_posix()
                    or provenance['model_sha256'] != hashlib.sha256(model.read_bytes()).hexdigest()):
                raise SystemExit(f'Type configuration {case}/{language} provenance differs')
            actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
            if actual != provenance['files']:
                raise SystemExit(f'Type configuration {case}/{language} file set or bytes differ')
            for path, digest in provenance.get('generator_implementation_sha256', {}).items():
                if hashlib.sha256((args.generator_dir / path).read_bytes()).hexdigest() != digest:
                    raise SystemExit(f'Type configuration {case}/{language} producer differs: {path}')
    sample_types = ROOT / 'tests/generator/parity/sample_type_configuration_contracts'
    sample_cases = {'equivalent_integer', 'equivalent_string_alias', 'field_parameter',
                    'numeric_alias', 'string_alias', 'unsigned_recipe'}
    if set(json.loads((sample_types / 'cases.json').read_text())) != sample_cases:
        raise SystemExit('Configured sample type inventory differs')
    for case in sorted(sample_cases):
        for language in ('rust', 'cpp', 'cpp_before_operand_fix'):
            output = sample_types / case / language
            if language == 'cpp_before_operand_fix' and case != 'numeric_alias':
                continue
            provenance = json.loads((output / 'PROVENANCE.json').read_text())
            model = sample_types / case / 'model.xml'
            if (provenance['generator_commit'] != ledger['generator_commit']
                    or provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited']
                    or provenance['model'] != model.relative_to(ROOT).as_posix()
                    or provenance['model_sha256'] != hashlib.sha256(model.read_bytes()).hexdigest()):
                raise SystemExit(f'Sample type {case}/{language} provenance differs')
            actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
            if actual != provenance['files']:
                raise SystemExit(f'Sample type {case}/{language} bytes differ')
            if language == 'cpp_before_operand_fix':
                source = ROOT / 'docs/parity/sample_type_generator_before_operand_fix_5_0_6.java.gz'
                expected = hashlib.sha256(gzip.decompress(source.read_bytes())).hexdigest()
                path = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
                if provenance['generator_implementation_sha256'].get(path) != expected:
                    raise SystemExit('Historical sample operand producer fingerprint differs')
            if language != 'cpp_before_operand_fix':
                for path, digest in provenance.get('generator_implementation_sha256', {}).items():
                    if hashlib.sha256((args.generator_dir / path).read_bytes()).hexdigest() != digest:
                        raise SystemExit(f'Sample type {case}/{language} producer differs: {path}')
    metadata_contracts = ROOT / 'tests/generator/parity/constant_metadata_contracts'
    metadata_cases = set(json.loads((metadata_contracts / 'cases.json').read_text()))
    if {p.name for p in metadata_contracts.iterdir() if p.is_dir()} != metadata_cases:
        raise SystemExit('Constant metadata fixture set differs')
    for case in sorted(metadata_cases):
        model = metadata_contracts / case / 'model.xml'
        for language in ('rust', 'cpp'):
            if language == 'cpp' and args.regenerating_constant_metadata_fixtures:
                continue
            output = metadata_contracts / case / language
            provenance = json.loads((output / 'PROVENANCE.json').read_text())
            if (provenance['generator_commit'] != ledger['generator_commit']
                    or provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited']
                    or provenance['model'] != model.relative_to(ROOT).as_posix()
                    or provenance['model_sha256'] != hashlib.sha256(model.read_bytes()).hexdigest()
                    or provenance['observer_sha256'] != hashlib.sha256(
                        (ROOT / 'tests/generator/ConstantMetadataProbe.java').read_bytes()).hexdigest()):
                raise SystemExit(f'Constant metadata {case}/{language} provenance differs')
            actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
            if actual != provenance['files']:
                raise SystemExit(f'Constant metadata {case}/{language} file set or bytes differ')
            for path, digest in provenance.get('generator_implementation_sha256', {}).items():
                if hashlib.sha256((args.generator_dir / path).read_bytes()).hexdigest() != digest:
                    raise SystemExit(f'Constant metadata {case}/{language} producer differs: {path}')
    topology_contracts = ROOT / 'tests/generator/parity/constant_topology_contracts'
    topology_cases = set(json.loads((topology_contracts / 'cases.json').read_text()))
    if {p.name for p in topology_contracts.iterdir() if p.is_dir()} != topology_cases:
        raise SystemExit('Constant topology fixture set differs')
    for case in sorted(topology_cases):
        model = topology_contracts / case / 'model.xml'
        for language in ('rust', 'cpp'):
            output = topology_contracts / case / language
            provenance = json.loads((output / 'PROVENANCE.json').read_text())
            if (provenance['generator_commit'] != ledger['generator_commit']
                    or provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited']
                    or provenance['model'] != model.relative_to(ROOT).as_posix()
                    or provenance['model_sha256'] != hashlib.sha256(model.read_bytes()).hexdigest()
                    or provenance['observer_sha256'] != hashlib.sha256(
                        (ROOT / 'tests/generator/ConstantMetadataProbe.java').read_bytes()).hexdigest()):
                raise SystemExit(f'Constant topology {case}/{language} provenance differs')
            actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
            if actual != provenance['files']:
                raise SystemExit(f'Constant topology {case}/{language} file set or bytes differ')
            for path, digest in provenance.get('generator_implementation_sha256', {}).items():
                if hashlib.sha256((args.generator_dir / path).read_bytes()).hexdigest() != digest:
                    raise SystemExit(f'Constant topology {case}/{language} producer differs: {path}')
    opaque = ROOT / 'tests/generator/parity/opaque_type_configuration_contracts'
    if not opaque.is_dir():
        raise SystemExit('Opaque type qualification fixtures are missing')
    if opaque.exists():
        cases = {'named_opaque', 'wide_unsigned', 'byte_vector'}
        if {p.name for p in opaque.iterdir() if p.is_dir()} != cases:
            raise SystemExit('Opaque type fixture set differs')
        for case in sorted(cases):
            for language in ('rust', 'cpp'):
                output = opaque / case / language
                provenance = json.loads((output / 'PROVENANCE.json').read_text())
                model = opaque / case / 'model.xml'
                if (provenance['generator_commit'] != ledger['generator_commit'] or
                        provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited'] or
                        provenance['model_sha256'] != hashlib.sha256(model.read_bytes()).hexdigest()):
                    raise SystemExit(f'Opaque type provenance differs: {case}/{language}')
                files = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
                if files != provenance['files']:
                    raise SystemExit(f'Opaque type generated bytes differ: {case}/{language}')
                for path, sha in provenance.get('generator_implementation_sha256', {}).items():
                    if hashlib.sha256((args.generator_dir / path).read_bytes()).hexdigest() != sha:
                        raise SystemExit(f'Opaque type producer differs: {case}/{language}')
    manifests = metadata_manifest_paths(ROOT)
    assert len(manifests) == 80 and len(set(manifests)) == 80
    for manifest in manifests:
        verify_metadata_helper_inputs(json.loads(manifest.read_text()), args.generator_dir, manifest)
    if args.require_complete and pending:
        raise SystemExit('Unqualified generator capabilities: ' + ', '.join(pending))
    print(f'Verified {len(ledger["sources"])} locked generator sources and '
          f'{len(ids)} capability records; {len(pending)} remain unqualified')


if __name__ == '__main__':
    main()
