#!/usr/bin/env python3
"""Generate or verify checked-in C++ example APIs with the local Java generator."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generator-dir', type=Path,
                        default=ROOT.parent / 'teaql_code_gen_cpp')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--fixtures', nargs='+',
                        help='Generate only the named model file stems; unknown names fail')
    args = parser.parse_args()
    generator = args.generator_dir.resolve()
    if not (generator / 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java').is_file():
        raise SystemExit('Use the independent teaql-code-gen cpp_support workspace')
    formatter = ROOT / '.tools/clang-format/bin/clang-format'
    version = subprocess.check_output([str(formatter), '--version'], text=True)
    if 'version 18.1.8' not in version:
        raise SystemExit('Pinned clang-format18.1.8 is required')
    subprocess.run(['mvn', '-q', '-pl', 'generator', '-am', '-DskipTests', 'package'],
                   cwd=generator, check=True)
    with tempfile.TemporaryDirectory(prefix='teaql_cpp_generation_') as temporary:
        work = Path(temporary)
        classpath = work / 'classpath.txt'
        subprocess.run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath',
                        f'-Dmdep.outputFile={classpath}'], cwd=generator, check=True)
        entries = [generator / 'generator/target/classes', generator / 'model/target/classes']
        cp = ':'.join(map(str, entries)) + ':' + classpath.read_text().strip()
        fixtures = [('examples/generator/model.xml', 'examples/generated'),
                                   ('tests/generator/typed_member_aliases.xml', 'tests/generated_typed_member_aliases'),
                                   ('tests/generator/types.xml', 'tests/generated_types'),
                                   ('tests/generator/checkers.xml', 'tests/generated_checkers'),
                                   ('tests/generator/checker_unique_string.xml', 'tests/generated_checker_unique_string'),
                                   ('tests/generator/checker_clock_scalars.xml', 'tests/generated_checker_clock_scalars'),
                                   ('tests/generator/checker_clock_composition.xml', 'tests/generated_checker_clock_composition'),
                                   ('tests/generator/relations.xml', 'tests/generated_relations'),
                                   ('tests/generator/business_ids.xml', 'tests/generated_business_ids'),
                                   ('tests/generator/optional_types.xml', 'tests/generated_optional_types'),
                                   ('tests/generator/query_field_branches.xml', 'tests/generated_query_field_branches'),
                                   ('tests/generator/inheritance.xml', 'tests/generated_inheritance'),
                                   ('tests/generator/inheritance_graph.xml', 'tests/generated_inheritance_graph'),
                                   ('tests/generator/inheritance_variants.xml', 'tests/generated_inheritance_variants'),
                                   ('tests/generator/inheritance_business_ids.xml', 'tests/generated_inheritance_business_ids'),
                                   ('tests/generator/candidates.xml', 'tests/generated_candidates'),
                                   ('tests/generator/constants_scalar.xml', 'tests/generated_constants_scalar'),
                                   ('tests/generator/constants_numeric.xml', 'tests/generated_constants_numeric'),
                                   ('tests/generator/constants_catalog.xml', 'tests/generated_constants_catalog'),
                                   ('tests/generator/constants_temporal.xml', 'tests/generated_constants_temporal'),
                                   ('tests/generator/constants_temporal_required.xml', 'tests/generated_constants_temporal_required'),
                                   ('tests/generator/dynamic_json.xml', 'tests/generated_dynamic_json'),
                                   ('tests/generator/sample_fields.xml', 'tests/generated_sample_fields'),
                                   ('tests/generator/sample_fields_runtime.xml', 'tests/generated_sample_fields_runtime'),
                                   ('tests/generator/sample_fields_temporal.xml', 'tests/generated_sample_fields_temporal')]
        if args.fixtures and not set(args.fixtures) <= {Path(model).stem for model, _ in fixtures}:
            parser.error('Unknown fixture names: ' + ', '.join(sorted(
                set(args.fixtures) - {Path(model).stem for model, _ in fixtures})))
        for model, destination in fixtures:
            if args.fixtures and Path(model).stem not in args.fixtures:
                continue
            output = work / destination
            command = ['java', '-cp', cp,
                            'com.skynet.codegenerator.cpp.CppCoreGenerator',
                            str(ROOT / model), str(output)]
            subprocess.run(command, cwd=generator, check=True)
            files = sorted(path for path in output.rglob('*') if path.is_file())
            headers = [str(path) for path in files if path.suffix == '.hpp']
            subprocess.run([str(formatter), f'--style=file:{ROOT / ".clang-format"}',
                            '--Werror', '-i', *headers], check=True)
            lock = json.loads((generator / 'cpp_generator.lock.json').read_text())
            checkpoint = lock['generator_baseline_commit']
            implementation = ['generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java',
                              'generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistWorkflows.java',
                              'generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistSupport.java',
                              'generator/src/main/java/com/skynet/codegenerator/workspace/JavaWorkspaceI18nSampleGenerator.java',
                              'generator/src/main/java/com/skynet/codegenerator/cpp/CppFieldLayoutMetadata.java',
                              'generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java',
                              'generator/src/main/java/com/skynet/codegenerator/cpp/CppRustXidMetadata.java',
                              'generator/src/main/java/com/skynet/codegenerator/cpp/CppBusinessIdMetadata.java',
                              'generator/src/main/java/com/skynet/codegenerator/cpp/CppJsonInputMetadata.java',
                              'model/src/main/resources/models/infra/system-types.xml',
                              'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template']
            manifest = {
                'scope': 'cpp-lib-core',
                'qualification_only': False,
                'generator_base_commit': lock.get('generator_workspace_base_commit', checkpoint),
                'generator_reference_commit': checkpoint,
                'generator_implementation_sha256': {
                    path: hashlib.sha256((generator / path).read_bytes()).hexdigest()
                    for path in implementation},
                'model': model, 'model_sha256': hashlib.sha256((ROOT / model).read_bytes()).hexdigest(),
                'runtime_rust_baseline': json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                'formatter': '18.1.8',
                'files': {path.relative_to(output).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                          for path in files}}
            (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
            target = ROOT / destination
            expected = {path.relative_to(output) for path in output.rglob('*') if path.is_file()}
            actual = {path.relative_to(target) for path in target.rglob('*') if path.is_file()}
            if args.check:
                if expected != actual:
                    raise SystemExit(f'Generated file set differs: {destination}')
                for path in expected:
                    if (output / path).read_bytes() != (target / path).read_bytes():
                        raise SystemExit(f'Stale generated file: {destination}/{path}')
            else:
                for path in actual - expected:
                    (target / path).unlink()
                for path in expected:
                    (target / path).parent.mkdir(parents=True, exist_ok=True)
                    if not (target / path).is_file() or (target / path).read_bytes() != (output / path).read_bytes():
                        shutil.copyfile(output / path, target / path)
            print(f'{"Verified" if args.check else "Generated"} {len(expected)} files: {destination}')


if __name__ == '__main__':
    main()
