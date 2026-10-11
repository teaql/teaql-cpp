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
        for model, destination in [('examples/generator/model.xml', 'examples/generated'),
                                   ('tests/generator/types.xml', 'tests/generated_types')]:
            output = work / destination
            subprocess.run(['java', '-cp', cp,
                            'com.skynet.codegenerator.cpp.CppCoreGenerator',
                            str(ROOT / model), str(output)], cwd=generator, check=True)
            files = sorted(path for path in output.rglob('*') if path.is_file())
            headers = [str(path) for path in files if path.suffix == '.hpp']
            subprocess.run([str(formatter), f'--style=file:{ROOT / ".clang-format"}',
                            '--Werror', '-i', *headers], check=True)
            lock = json.loads((generator / 'cpp_generator.lock.json').read_text())
            checkpoint = lock['generator_baseline_commit']
            implementation = ['generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java',
                              'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template']
            manifest = {
                'scope': 'cpp-lib-core', 'generator_base_commit': checkpoint,
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
