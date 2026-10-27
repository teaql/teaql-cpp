#!/usr/bin/env python3
"""Execute focused generator Java suites after the sequential producer build."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
SUITES = ['com.skynet.codegenerator.cpp.CppFieldLayoutMetadataTest',
          'com.skynet.codegenerator.cpp.CppJsonInputMetadataTest',
          'com.skynet.codegenerator.cpp.CppAuditMetadataCorpusTest',
          'com.skynet.codegenerator.CppGenerationTest',
          'com.skynet.codegenerator.CppIdentityGenerationTest']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve earlier evidence'
    sources = [PRODUCER / 'generator/src/test/java' / (suite.replace('.', '/') + '.java') for suite in SUITES]
    tracked = sources + sorted((PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp').glob('*.java'))
    record = dict(complete=False, suites=SUITES, steps=[], source_hashes={str(p): digest(p) for p in tracked})
    def run(label, command, cwd=PRODUCER):
        result = subprocess.run(list(map(str, command)), cwd=cwd, text=True, capture_output=True)
        record['steps'].append(dict(label=label, command=list(map(str, command)), exit_code=result.returncode,
                                    cwd=str(cwd), stdout=result.stdout, stderr=result.stderr))
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_expression_identity_java_') as temporary:
            work = Path(temporary)
            record['owned_directory'] = str(work)
            cp_file = work / 'classpath.txt'
            run('classpath', ['mvn', '-q', '-o', '-pl', 'generator', 'dependency:build-classpath',
                              '-Dmdep.outputFile=' + str(cp_file)])
            cp = ':'.join(str(PRODUCER / name) for name in ['generator/target/classes', 'model/target/classes'])
            cp += ':' + cp_file.read_text().strip()
            run('compile_suites', ['javac', '-cp', cp, '-d', work, *sources])
            test_cp = str(work) + ':' + str(PRODUCER / 'generator/src/test/resources') + ':' + cp
            output = run('JUnit', ['java', '-cp', test_cp, 'org.junit.runner.JUnitCore', *SUITES],
                         cwd=PRODUCER / 'generator')
            match = re.search(r'OK \((\d+) tests?\)', output)
            assert match, 'JUnit must actually report successful tests'
            record['test_count'] = int(match[1])
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        assert record['source_hashes'] == {str(p): digest(p) for p in tracked}
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Successful Java tests:', record['test_count'])


if __name__ == '__main__':
    main()
