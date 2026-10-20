#!/usr/bin/env python3
"""Reproduce complete relationship/alias SDKs, permitting only reviewed identity repairs."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from relation_identity_qualification import metadata, verify_file

ROOT = Path(__file__).resolve().parents[1]
FAMILIES = ('relation_identity_contracts', 'query_alias_contracts')
SOURCE = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
SUPPORT = 'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files(directory):
    return {str(p.relative_to(directory)): digest(p) for p in directory.rglob('*')
            if p.is_file() and p.name != 'PROVENANCE.json'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--refresh-cpp', action='store_true')
    parser.add_argument('--refresh-ledger-composition', action='store_true')
    parser.add_argument('--refresh-scalar-diagnostics', action='store_true')
    parser.add_argument('--refresh-expression-entries', action='store_true')
    parser.add_argument('--refresh-result-markers', action='store_true')
    parser.add_argument('--refresh-business-id-lookups', action='store_true')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    assert sum([args.check, args.refresh_cpp, args.refresh_ledger_composition, args.refresh_scalar_diagnostics, args.refresh_expression_entries, args.refresh_result_markers, args.refresh_business_id_lookups]) == 1
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    reference = ROOT / '.reference/teaql_code_gen'
    lock = json.loads((producer / 'cpp_generator.lock.json').read_text())
    assert subprocess.check_output(['git', '-C', str(producer), 'branch', '--show-current'], text=True).strip() == 'cpp_support'
    assert subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'], text=True).strip() == lock['generator_baseline_commit']
    assert not subprocess.check_output(['git', '-C', str(reference), 'status', '--porcelain'])
    records, staged = [], []
    with tempfile.TemporaryDirectory(prefix='teaql_relation_identity_generation_') as temporary:
        work = Path(temporary)
        for language, repo in (('rust', reference), ('cpp', producer)):
            classpath_file = work / (language + '_classpath.txt')
            subprocess.run(['mvn', '-q', '-o', '-pl', 'generator',
                            'dependency:build-classpath', f'-Dmdep.outputFile={classpath_file}'],
                           cwd=repo, check=True)
            cp = ':'.join(str(repo / path) for path in ('generator/target/classes', 'model/target/classes')) + ':' + classpath_file.read_text().strip()
            probes = ['TypeConfigurationProbe'] + (['RelationIdentityMetadataProbe', 'ScalarDiagnosticMetadataProbe', 'BusinessIdLookupMetadataProbe'] if language == 'cpp' else [])
            subprocess.run(['javac', '-cp', cp, '-d', str(work),
                            *(str(ROOT / 'tests/generator' / (probe + '.java')) for probe in probes)], check=True)
            for family in FAMILIES:
                base = ROOT / 'tests/generator/parity' / family
                model = base / 'model.xml'
                output = work / language / family
                subprocess.run(['java', '-cp', str(work) + ':' + cp, 'TypeConfigurationProbe',
                                language, str(model), str(output)], check=True, stdout=subprocess.DEVNULL)
                assert json.loads((output / 'OBSERVATION.json').read_text())['generated']
                if language == 'cpp':
                    subprocess.run([str(ROOT / '.tools/clang-format/bin/clang-format'),
                                    f'--style=file:{ROOT / ".clang-format"}', '--Werror', '-i',
                                    *(str(p) for p in output.rglob('*.hpp'))], check=True)
                provenance = dict(generator_commit=lock['generator_baseline_commit'],
                                  runtime_commit=lock['runtime_rust_baseline'], hand_edited=False,
                                  model=str(model.relative_to(ROOT)), model_sha256=digest(model), files=files(output))
                if language == 'cpp':
                    provenance.update(formatter='18.1.8', generator_implementation_sha256={SOURCE: digest(producer / SOURCE), SUPPORT: digest(producer / SUPPORT)})
                    catalog_source='generator/src/main/java/com/skynet/codegenerator/workspace/JavaWorkspaceI18nSampleGenerator.java'
                    provenance['generator_implementation_sha256'][catalog_source]=digest(producer/catalog_source)
                    workflow_source='generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistWorkflows.java'
                    provenance['generator_implementation_sha256'][workflow_source]=digest(producer/workflow_source)
                    support_source='generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistSupport.java'
                    provenance['generator_implementation_sha256'][support_source]=digest(producer/support_source)
                target = base / language
                if args.check or language == 'rust':
                    assert files(target) == provenance['files'], (family, language)
                    assert json.loads((target / 'PROVENANCE.json').read_text()) == provenance, (family, language)
                else:
                    current = json.loads((target / 'PROVENANCE.json').read_text())
                    assert files(target) == current['files']
                    history = base / 'cpp_before_relation_fix'
                    previous = current if args.refresh_ledger_composition or args.refresh_scalar_diagnostics or args.refresh_expression_entries or args.refresh_result_markers or args.refresh_business_id_lookups else json.loads((history / 'PROVENANCE.json').read_text())
                    assert previous['files'].keys() == provenance['files'].keys()
                    rows = metadata(model, work / (family + '.json'), cp, work)
                    if args.refresh_business_id_lookups:
                        from business_id_lookup_qualification import metadata as lookup_metadata, verify_file as verify_lookup
                        lookups = lookup_metadata(model, work / 'lookup_metadata.json', cp, work)
                        changes = sum(verify_lookup(ROOT, name, (target / name).read_text(),
                                                   (output / name).read_text(), lookups) for name in provenance['files'])
                    elif args.refresh_result_markers:
                        from query_result_marker_qualification import verify_file as verify_marker
                        changes = sum(verify_marker(ROOT, name, (target / name).read_text(),
                                                   (output / name).read_text()) for name in provenance['files'])
                    elif args.refresh_expression_entries:
                        from expression_entry_qualification import verify_file as verify_entry
                        changes = sum(verify_entry(ROOT, name, (target / name).read_text(),
                                                    (output / name).read_text()) for name in provenance['files'])
                    elif args.refresh_scalar_diagnostics:
                        from scalar_diagnostic_qualification import metadata as scalar_metadata, verify_file as verify_scalar
                        scalars = scalar_metadata(model, work / (family + '_scalars.json'), cp, work)
                        changes = sum(verify_scalar(ROOT, name, (target / name).read_text(),
                                                    (output / name).read_text(), scalars) for name in provenance['files'])
                    elif args.refresh_ledger_composition:
                        from ledger_composition_qualification import verify_file as verify_composition_file
                        changes = sum(verify_composition_file(ROOT, name, (target / name).read_text(),
                                                             (output / name).read_text()) for name in provenance['files'])
                    else:
                        changes = sum(verify_file(ROOT, name, (history / name).read_text(),
                                                  (output / name).read_text(), rows) for name in provenance['files'])
                        assert files(history) == previous['files']
                    staged.append((output, target, provenance))
                    records.append(dict(family=family, changed_files=[name for name in provenance['files'] if previous['files'][name] != provenance['files'][name]],
                                        reviewed_identity_locations=changes, relationships=rows,
                                        producer_before=previous['generator_implementation_sha256'],
                                        producer_after=provenance['generator_implementation_sha256']))
                print('Verified', family, language, len(provenance['files']), 'complete files')
        # Validate every output before applying any regeneration.
        for output, target, provenance in staged:
            for source in output.rglob('*'):
                if source.is_file():
                    destination = target / source.relative_to(output)
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    if not destination.exists() or destination.read_bytes() != source.read_bytes():
                        shutil.copyfile(source, destination)
            (target / 'PROVENANCE.json').write_text(json.dumps(provenance, indent=2) + '\n')
    if records:
        report = args.report or ROOT / 'docs/parity/relation_identity_sdk_update_5_0_6.json'
        assert not report.exists()
        report.write_text(json.dumps(dict(schema=1, records=records,
                                         original_rust_unchanged=True, historical_cpp_unchanged=True), indent=2) + '\n')


if __name__ == '__main__':
    main()
