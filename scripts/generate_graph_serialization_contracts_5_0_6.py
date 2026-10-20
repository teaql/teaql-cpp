#!/usr/bin/env python3
"""Generate or reproduce complete Rust/C++ graph-serialization fixture SDKs."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'tests/generator/parity/graph_serialization_contracts'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files(directory):
    return {str(p.relative_to(directory)): digest(p) for p in sorted(directory.rglob('*'))
            if p.is_file() and p.name != 'PROVENANCE.json'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--write', action='store_true')
    mode.add_argument('--check', action='store_true')
    args = parser.parse_args()
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    reference = ROOT / '.reference/teaql_code_gen'
    lock = json.loads((producer / 'cpp_generator.lock.json').read_text())
    subprocess.run(['python3', ROOT / 'scripts/verify_upstream.py'], check=True)
    assert subprocess.check_output(['git', '-C', producer, 'branch', '--show-current'], text=True).strip() == 'cpp_support'
    assert subprocess.check_output(['git', '-C', reference, 'rev-parse', 'HEAD'], text=True).strip() == lock['generator_baseline_commit']
    assert not subprocess.check_output(['git', '-C', reference, 'status', '--porcelain'])
    if args.write:
        assert not (BASE / 'cpp').exists() and not (BASE / 'rust').exists(), 'Initial generation only; preserve existing artifacts'
    staged = []
    with tempfile.TemporaryDirectory(prefix='teaql_graph_serialization_sdk_') as temporary:
        work = Path(temporary)
        for language, repo in [('rust', reference), ('cpp', producer)]:
            subprocess.run(['mvn', '-q', '-o', '-pl', 'generator', '-am', 'package', '-DskipTests'], cwd=repo, check=True)
            classpath = work / (language + '_classpath.txt')
            subprocess.run(['mvn', '-q', '-o', '-pl', 'generator', 'dependency:build-classpath',
                            f'-Dmdep.outputFile={classpath}'], cwd=repo, check=True)
            cp = ':'.join(str(repo / p) for p in ['generator/target/classes', 'model/target/classes']) + ':' + classpath.read_text().strip()
            subprocess.run(['javac', '-cp', cp, '-d', work, ROOT / 'tests/generator/TypeConfigurationProbe.java'], check=True)
            output = work / language
            subprocess.run(['java', '-cp', str(work) + ':' + cp, 'TypeConfigurationProbe', language,
                            BASE / 'model.xml', output], check=True)
            observation = json.loads((output / 'OBSERVATION.json').read_text())
            assert observation['generated'], observation
            if language == 'cpp':
                subprocess.run([ROOT / '.tools/clang-format/bin/clang-format',
                                f'--style=file:{ROOT / ".clang-format"}', '--Werror', '-i',
                                *sorted(output.rglob('*.hpp'))], check=True)
            provenance = dict(generator_commit=lock['generator_baseline_commit'],
                              runtime_commit=lock['runtime_rust_baseline'], hand_edited=False,
                              model=str((BASE / 'model.xml').relative_to(ROOT)),
                              model_sha256=digest(BASE / 'model.xml'), files=files(output))
            if language == 'cpp':
                paths = ['generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java',
                         'generator/src/main/resources/generator/cpp_lib_core/support.hpp.template',
                         'generator/src/main/java/com/skynet/codegenerator/workspace/JavaWorkspaceI18nSampleGenerator.java',
                         'generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistWorkflows.java',
                         'generator/src/main/java/com/skynet/codegenerator/cpp/CppAssistSupport.java']
                provenance.update(formatter='18.1.8', generator_implementation_sha256={p: digest(producer / p) for p in paths})
            target = BASE / language
            if args.check:
                assert files(target) == provenance['files'], language
                assert json.loads((target / 'PROVENANCE.json').read_text()) == provenance, language
            else:
                (output / 'PROVENANCE.json').write_text(json.dumps(provenance, indent=2) + '\n')
                staged.append((output, target))
        for output, target in staged:
            shutil.copytree(output, target)
    print('Complete graph serialization SDKs ' + ('reproduced' if args.check else 'generated'))


if __name__ == '__main__':
    main()
