#!/usr/bin/env python3
"""Guard generated sample-field source checks under optimized Python."""

import ast
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
PREEDIT = ROOT / 'docs/parity/generated_sample_fields_optimized_preedit_5_0_6.json.gz'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ReplaceAssertions(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        error = ast.Name(id='AssertionError', ctx=ast.Load())
        if node.msg is not None:
            error = ast.Call(func=error, args=[node.msg], keywords=[])
        return ast.copy_location(ast.If(
            test=ast.UnaryOp(op=ast.Not(), operand=node.test),
            body=[ast.Raise(exc=error, cause=None)], orelse=[]), node)


def check_transition(before, after, count, label):
    original = ast.parse(before)
    current = ast.parse(after)
    require(sum(isinstance(node, ast.Assert) for node in ast.walk(original)) == count,
            'Original assertion count: ' + label)
    require(not any(isinstance(node, ast.Assert) for node in ast.walk(current)),
            'No remaining removable checks: ' + label)
    expected = ast.fix_missing_locations(ReplaceAssertions().visit(original))
    require(ast.dump(expected, include_attributes=False)
            == ast.dump(current, include_attributes=False),
            'Only unconditional check transition allowed: ' + label)


def run(command, environment, label, failure):
    process = subprocess.run(command, cwd=ROOT, env=environment,
                             capture_output=True, text=True, timeout=120)
    require(process.returncode != 0 and failure in process.stderr
            and '85/85 observations match' not in process.stdout,
            'Wrong generated sample-field comparison accepted: ' + label)
    return {'case': label, 'exit_code': process.returncode}


def main():
    before = json.loads(gzip.decompress(PREEDIT.read_bytes()))
    require(before['complete'] is True and before['whole_goal_complete'] is False
            and before['program_sha256'] == hashlib.sha256(
                before['program'].encode()).hexdigest()
            and len(before['results']) == 2
            and before['results'][0]['optimized'] is False
            and before['results'][0]['exit_code'] != 0
            and 'AssertionError' in before['results'][0]['stderr']
            and before['results'][1]['optimized'] is True
            and before['results'][1]['exit_code'] == 0
            and '85/85 observations match' in before['results'][1]['stdout'],
            'Reproduced former baseline-substitution false pass')
    paths = [(before['source_path'], before['source_before'], before['source_sha256'], 5),
             (before['admission_path'], before['admission_before'],
              before['admission_sha256'], 3)]
    current_hashes = {}
    for path, original, original_hash, count in paths:
        require(hashlib.sha256(original.encode()).hexdigest() == original_hash,
                'Frozen original source hash: ' + path)
        source = ROOT / path
        check_transition(original, source.read_text(), count, path)
        current_hashes[path] = digest(source)

    binary = ROOT / 'build_native_current/teaql_generated_sample_fields_probe'
    clock = ROOT / 'build_native_current/libteaql_sample_fixed_clock.so'
    require(binary.is_file() and clock.is_file(), 'Registered sample-field observer and clock')
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', TEAQL_LOG_ENDPOINT='off')
    environment.pop('PYTHONOPTIMIZE', None)
    arguments = [str(ROOT / before['source_path']), str(binary), str(clock)]
    results = []
    with tempfile.TemporaryDirectory(prefix='teaql_sample_fields_optimized_control_') as owned:
        fake = Path(owned) / 'empty_observer'
        fake.write_text('#!' + sys.executable + '\nimport sys\nsys.stdin.read()\n')
        fake.chmod(0o700)
        environment['TMPDIR'] = owned
        for optimized in (False, True):
            command = [sys.executable] + (['-O'] if optimized else [])
            command += ['-c', before['program'], *arguments]
            results.append(run(command, environment,
                               'wrong_baseline/' + ('optimized' if optimized else 'normal'),
                               'AssertionError'))
        fixture_program = '''import pathlib,runpy,sys
original=pathlib.Path.read_bytes
def altered(path):
    value=original(path)
    return value+b'\\n' if path.name=='fixed_sample_clock.cpp' else value
pathlib.Path.read_bytes=altered
sys.path.insert(0,str(pathlib.Path(sys.argv[1]).parent))
sys.argv=[sys.argv[1],'--cpp',sys.argv[2],'--clock',sys.argv[3]]
runpy.run_path(sys.argv[0],run_name='__main__')
'''
        results.append(run([sys.executable, '-O', '-c', fixture_program, *arguments],
                           environment, 'wrong_clock_fixture/optimized', 'AssertionError'))
        results.append(run([sys.executable, '-O', arguments[0], '--cpp', str(fake),
                            '--clock', str(clock)], environment,
                           'empty_observer/optimized', 'incomplete observations'))
        removed_bytes = sum(path.stat().st_size for path in Path(owned).rglob('*')
                            if path.is_file())
    require(not Path(owned).exists() and len(results) == 4
            and all(digest(ROOT / path) == value for path, value in current_hashes.items()),
            'Four actual controls, stable sources and owned cleanup')
    print('8/8 unconditional source checks and 4/4 negative processes')


if __name__ == '__main__':
    main()
