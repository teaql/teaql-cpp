#!/usr/bin/env python3
"""Reject wrong connection results and empty selections with Python -O."""

import argparse
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
SOURCE = ROOT / 'scripts/verify_runtime_connection_5_0_6.py'
PREEDIT = ROOT / 'docs/parity/runtime_connection_optimized_preedit_5_0_6.json.gz'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_transition(before, after):
    replacements = (
        ("            assert same(result, case['source_expected']) and files == case['created_files'], case['name'] + ': original changed'\n",
         "            if not (same(result, case['source_expected']) and files == case['created_files']):\n"
         "                raise AssertionError(case['name'] + ': original changed')\n"),
        ("        assert same(result, connection_outcome) and files == case['created_files'], (case['name'], result, connection_outcome, files)\n",
         "        if not (same(result, connection_outcome) and files == case['created_files']):\n"
         "            raise AssertionError((case['name'], result, connection_outcome, files))\n"),
        ("    assert total == 13\n",
         "    if total != 13:\n"
         "        raise AssertionError('Expected 13 connection observations, got ' + str(total))\n"),
    )
    expected = before
    for old, new in replacements:
        require(expected.count(old) == 1, 'Exact predecessor assertion')
        expected = expected.replace(old, new)
    require(after == expected, 'Only three bounded source checks changed')
    require(sum(isinstance(node, ast.Assert) for node in ast.walk(ast.parse(after))) == 0,
            'No optimized-away assertions in the active connection verifier')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None:
        require(not args.record.exists(), 'Existing control record')
    before = json.loads(gzip.decompress(PREEDIT.read_bytes()))
    require(before['complete'] is True and before['owned_directory_removed'] is True
            and before['source_path'] == str(SOURCE.relative_to(ROOT)), 'Reproduced predecessor')
    require(hashlib.sha256(before['source_before'].encode()).hexdigest()
            == before['source_sha256'] and digest(SOURCE) != before['source_sha256'],
            'Frozen predecessor and changed current source')
    require(before['results'][0]['optimized'] is False
            and before['results'][0]['exit_code'] != 0
            and before['results'][1]['optimized'] is True
            and before['results'][1]['exit_code'] == 0, 'Actual false acceptance')
    source_transition(before['source_before'], SOURCE.read_text())
    source_sha256 = digest(SOURCE)
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    environment.pop('PYTHONOPTIMIZE', None)
    results = []
    fake_source = before['fake_source']

    with tempfile.TemporaryDirectory(prefix='teaql_runtime_connection_optimized_controls_') as name:
        owned = Path(name)
        fake = owned / 'fake_cpp'
        fake.write_text(fake_source)
        fake.chmod(0o700)
        require(digest(fake) == before['fake_sha256'], 'Same wrong-output executable')

        def run(label, optimized, extra, expected_error):
            command = [sys.executable]
            if optimized:
                command.append('-O')
            command.extend([str(SOURCE), '--cpp', str(fake), *extra])
            result = subprocess.run(command, cwd=ROOT, env=environment,
                                    capture_output=True, text=True)
            require(result.returncode != 0 and expected_error in result.stderr,
                    'Wrong connection result accepted: ' + label)
            results.append(dict(label=label, optimized=optimized,
                                exit_code=result.returncode,
                                error=expected_error))

        for optimized in (False, True):
            run('wrong_connection', optimized, [], 'AssertionError')
            run('wrong_assembly', optimized, ['--assembly'], 'AssertionError')
        run('wrong_original', True, ['--rust', str(fake)], 'original changed')

        count_program = (
            'import sys\n'
            'sys.path.insert(0, sys.argv[1])\n'
            'import verify_runtime_connection_5_0_6 as verifier\n'
            'verifier.load_original_record = lambda _: {"cases": []}\n'
            'sys.argv = ["connection", "--cpp", sys.argv[2]]\n'
            'verifier.main()\n'
        )
        for optimized in (False, True):
            command = [sys.executable]
            if optimized:
                command.append('-O')
            command.extend(['-c', count_program, str(ROOT / 'scripts'), str(fake)])
            result = subprocess.run(command, cwd=ROOT, env=environment,
                                    capture_output=True, text=True)
            require(result.returncode != 0 and 'Expected 13 connection observations'
                    in result.stderr, 'Empty connection selection accepted')
            results.append(dict(label='empty_selection', optimized=optimized,
                                exit_code=result.returncode,
                                error='Expected 13 connection observations'))
        owned_bytes_removed = sum(path.stat().st_size for path in owned.rglob('*')
                                  if path.is_file())

    require(not owned.exists() and digest(SOURCE) == source_sha256,
            'Owned control cleanup and unchanged current source')
    require(len(results) == 7, 'Seven actual negative processes')
    record = dict(complete=True, whole_goal_complete=False,
                  scope='Exact three-check source transition and seven normal/optimized wrong-output controls',
                  preedit_sha256=digest(PREEDIT), source_before_sha256=before['source_sha256'],
                  source_after_sha256=source_sha256, results=results,
                  owned_directory_removed=True, owned_bytes_removed=owned_bytes_removed)
    if args.record is not None:
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    print('7/7 wrong-output and empty-selection controls reject under normal/optimized Python')


if __name__ == '__main__':
    main()
