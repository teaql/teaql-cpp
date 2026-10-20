#!/usr/bin/env python3
"""Regenerate complete SDKs and retain earlier execution only for identical bytes.

This proof does not compile or execute a new observer. Every input pair must
already contain successful actual original/native compilation and equal results.
Both complete generated file sets must reproduce, including all source files.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from verify_generated_privacy_metadata import CPP, LAUNCHER, REFERENCE, classpath

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiled-pairs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a fresh evidence path')
    previous = json.loads(gzip.decompress(args.compiled_pairs.read_bytes()))
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if previous['runtime_commit'] != baseline or not previous['cases']:
        parser.error('A nonempty matching-baseline compiled receipt is required')
    for case in previous['cases']:
        if (case['source_compile'] != 0 or case['cpp_compile'] != 0
                or not case['matching'] or case['source'] != case['cpp']):
            parser.error('All prior pairs must have compiled, executed and matched')
        for source in case['files'].values():
            if hashlib.sha256(source['text'].encode()).hexdigest() != source['sha256']:
                parser.error('Archived generated source identity differs')
    record = {'complete': False, 'steps': [], 'cases': [], 'baseline': baseline,
              'compiled_receipt_sha256': digest(args.compiled_pairs),
              'scope': 'Current complete original/C++ SDK bytes reproduce earlier '
                       'successful compiled/executed pairs; no new execution is claimed.',
              'full_goal_accepted': False}

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        record['steps'].append({'command': command, 'exit_code': result.returncode,
                                'stdout': result.stdout, 'stderr': result.stderr})
        save()
        result.check_returncode()

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    for reference in (ROOT / '.reference/teaql-rs', REFERENCE):
        status = subprocess.check_output(['git', '-C', str(reference), 'status', '--porcelain'], text=True)
        if status:
            raise RuntimeError('Original source reference must remain unchanged')
    lock = json.loads((CPP / 'cpp_generator.lock.json').read_text())
    original_head = subprocess.check_output(['git', '-C', str(REFERENCE), 'rev-parse', 'HEAD'], text=True).strip()
    if original_head != lock['generator_baseline_commit']:
        raise RuntimeError('Original generator baseline differs')
    sources = [CPP / 'generator/src/main/java/com/skynet/codegenerator/cpp' / (name + '.java')
               for name in ('CppRustLiteralMetadata', 'CppRustXidMetadata',
                            'CppJsonInputMetadata', 'CppFieldLayoutMetadata', 'CppCoreGenerator')]
    record['producer_sources'] = {str(path): digest(path) for path in sources}
    record['support_sources'] = {
        str(ROOT / 'scripts/verify_generated_privacy_metadata.py'):
            digest(ROOT / 'scripts/verify_generated_privacy_metadata.py')}
    record['verifier_sha256'] = digest(Path(__file__))
    with tempfile.TemporaryDirectory(prefix='teaql_privacy_reproduce_506_') as temporary:
        work = Path(temporary)
        original_cp = classpath(REFERENCE, work / 'original_cp.txt')
        cpp_cp = classpath(CPP, work / 'cpp_cp.txt')
        launcher = work / 'FrozenPrivacy.java'
        launcher.write_text(LAUNCHER)
        run(['javac', '-cp', original_cp, launcher])
        for index, case in enumerate(previous['cases']):
            directory = work / str(index)
            directory.mkdir()
            model = directory / 'model.xml'
            model.write_text(case['model'])
            run(['java', '-cp', str(work) + ':' + original_cp, 'FrozenPrivacy',
                 model, directory / 'original'])
            run(['java', '-cp', cpp_cp, 'com.skynet.codegenerator.cpp.CppCoreGenerator',
                 model, directory / 'cpp'])
            checks = {}
            for prefix in ('original', 'cpp'):
                expected = {name[len(prefix) + 1:]: value['sha256']
                            for name, value in case['files'].items()
                            if name.startswith(prefix + '/')}
                actual = {str(path.relative_to(directory / prefix)): digest(path)
                          for path in (directory / prefix).rglob('*') if path.is_file()}
                checks[prefix] = {'before': expected, 'after': actual, 'matching': expected == actual}
                if not expected or actual != expected:
                    record['cases'].append({'name': case['case'], 'trees': checks})
                    save()
                    raise RuntimeError('Complete generated SDK changed: ' + case['case'] + '/' + prefix)
            record['cases'].append({'name': case['case'], 'trees': checks})
            save()
            print('Reproduced compiled pair ' + case['case'], flush=True)
    if any(digest(path) != value for path, value in record['producer_sources'].items()):
        raise RuntimeError('Producer changed during reproduction')
    record['complete'] = True
    save()
    print(f"Reproduced both complete SDKs for {len(record['cases'])} compiled pairs; no repeated execution.")


if __name__ == '__main__':
    main()
