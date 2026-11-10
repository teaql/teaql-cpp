#!/usr/bin/env python3
"""Keep retained SQL executor source checks active under optimized Python."""

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
MANIFEST = ROOT / 'docs/parity/sql_executor_optimized_assert_rewrite_5_0_6.json.gz'
REGISTRY = ROOT / 'tests/parity/sql_executor_driver_contracts_5_0_6.json'
EXECUTION = ROOT / 'docs/parity/sql_executor_driver_execution_final_5_0_6.json.gz'
VERIFIER = ROOT / 'scripts/verify_sql_executor_driver_contracts_5_0_6.py'
CONTROLS = ROOT / 'tests/parity/verify_sql_executor_driver_controls_5_0_6.py'
SOURCES = {
    'scripts/verify_sql_executor_driver_contracts_5_0_6.py',
    'tests/parity/verify_sql_executor_driver_controls_5_0_6.py',
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(value):
    return hashlib.sha256(value).hexdigest()


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        failure = ast.Raise(
            exc=ast.Call(func=ast.Name(id='AssertionError', ctx=ast.Load()),
                         args=[node.msg] if node.msg else [], keywords=[]), cause=None)
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[failure], orelse=[])


def check_sources():
    manifest = json.loads(gzip.decompress(MANIFEST.read_bytes()))
    require(manifest['complete'] is True and manifest['whole_goal_complete'] is False
            and manifest['source_commit'] == 'f81ef1adf341afdcc6cff305895c16d4fdb164ab'
            and manifest['assertions'] == 60,
            'SQL executor source transition changed')
    rows = manifest['rows']
    require(len(rows) == 2 and {row['path'] for row in rows} == SOURCES,
            'SQL executor source selection changed')
    total = 0
    for row in rows:
        before = row['before_source'].encode()
        after = (ROOT / row['path']).read_bytes()
        require(digest(before) == row['before_sha256']
                and digest(after) == row['after_sha256'],
                row['path'] + ': exact source transition changed')
        original = ast.parse(before)
        rewritten = ast.parse(after)
        count = sum(isinstance(node, ast.Assert) for node in ast.walk(original))
        require(count == row['assertions']
                and not any(isinstance(node, ast.Assert) for node in ast.walk(rewritten))
                and ast.dump(ReplaceAssert().visit(original), include_attributes=False)
                == ast.dump(rewritten, include_attributes=False),
                row['path'] + ': change extends beyond unconditional checks')
        total += count
    require(total == 60, 'SQL executor assertion inventory changed')


def run(script, execution, optimized):
    command = [sys.executable]
    if optimized:
        command.append('-O')
    command.extend([str(script), '--contracts', str(REGISTRY),
                    '--execution', str(execution)])
    environment = dict(os.environ)
    environment.pop('PYTHONOPTIMIZE', None)
    environment['PYTHONDONTWRITEBYTECODE'] = '1'
    environment['TEAQL_LOG_ENDPOINT'] = 'off'
    return subprocess.run(command, cwd=ROOT, env=environment,
                          capture_output=True, text=True)


def main():
    check_sources()
    record = json.loads(gzip.decompress(EXECUTION.read_bytes()))
    require(record['complete'] is True, 'Retained SQL executor execution is incomplete')
    with tempfile.TemporaryDirectory(prefix='teaql_sql_executor_optimized_506_') as temporary:
        incomplete = Path(temporary) / 'incomplete.json.gz'
        record['complete'] = False
        incomplete.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

        for optimized in (False, True):
            mode = 'optimized' if optimized else 'normal'
            valid = run(VERIFIER, EXECUTION, optimized)
            require(valid.returncode == 0 and '9 complete source/corpus/native bindings' in valid.stdout,
                    mode + ': valid source execution failed: ' + valid.stderr)
            altered = run(VERIFIER, incomplete, optimized)
            require(altered.returncode != 0 and altered.stderr.strip().splitlines()[-1] == 'AssertionError',
                    mode + ': incomplete source execution accepted or failed elsewhere')
            controls = run(CONTROLS, EXECUTION, optimized)
            require(controls.returncode == 0
                    and '22 effective corruptions and two actual legacy locks reject' in controls.stdout,
                    mode + ': SQL executor source controls failed: ' + controls.stderr)

    print('SQL executor source: 60 exact rewrites and 6 normal/optimized checks passed')


if __name__ == '__main__':
    main()
