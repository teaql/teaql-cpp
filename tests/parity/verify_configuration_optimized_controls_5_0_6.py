#!/usr/bin/env python3
"""Exercise source and observer checks in normal and optimized Python."""

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
MANIFEST = ROOT / 'docs/parity/configuration_optimized_assert_rewrite_5_0_6.json.gz'
SCRIPTS = {
    'scripts/verify_runtime_configuration_kernel_5_0_6.py',
    'scripts/verify_business_runtime_configuration_5_0_6.py',
    'scripts/verify_generated_registry_schema_5_0_6.py',
    'scripts/verify_legacy_observation_identity_5_0_6.py',
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
            and manifest['source_commit'] == 'f1d2f70a9f60e6d846ad05ff50cc8271968ca5e9'
            and manifest['assertions'] == 35,
            'Configuration source transition changed')
    rows = manifest['rows']
    require(len(rows) == 4 and {row['path'] for row in rows} == SCRIPTS,
            'Configuration source selection changed')
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
    require(total == 35, 'Configuration assertion inventory changed')


def run(optimized, arguments, directory):
    command = [sys.executable]
    if optimized:
        command.append('-O')
    command.extend(arguments)
    environment = dict(os.environ)
    environment.pop('PYTHONOPTIMIZE', None)
    environment['PYTHONDONTWRITEBYTECODE'] = '1'
    environment['TEAQL_LOG_ENDPOINT'] = 'off'
    environment['TEAQL_CONTROL_TMPDIR'] = str(directory)
    environment['PYTHONPATH'] = os.pathsep.join((
        str(ROOT / 'scripts'), str(ROOT / 'tests/parity'),
        environment.get('PYTHONPATH', '')))
    return subprocess.run(command, cwd=ROOT, env=environment,
                          capture_output=True, text=True)


def check_result(result, accepted, label, diagnostic=''):
    require((result.returncode == 0) is accepted,
            f'{label}: exit {result.returncode}; stdout={result.stdout}; stderr={result.stderr}')
    if diagnostic:
        require(diagnostic in result.stderr,
                f'{label}: rejected for an unrelated reason: {result.stderr}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kernel', required=True, type=Path)
    parser.add_argument('--business', required=True, type=Path)
    parser.add_argument('--registry', required=True, type=Path)
    args = parser.parse_args()
    executables = {name: str(getattr(args, name).resolve(strict=True))
                   for name in ('kernel', 'business', 'registry')}
    check_sources()

    with tempfile.TemporaryDirectory(prefix='teaql_configuration_optimized_506_') as temporary:
        directory = Path(temporary)
        altered = directory / 'docs/parity/business_runtime_configuration_original_5_0_6.json'
        altered.parent.mkdir(parents=True)
        altered.write_bytes((ROOT / 'docs/parity/business_runtime_configuration_original_5_0_6.json').read_bytes()
                            + b'\n')
        empty = directory / 'empty_observer'
        empty.write_text('#!/usr/bin/env python3\n')
        empty.chmod(0o755)

        source_record = (
            "import json; from pathlib import Path; import verify_runtime_configuration_kernel_5_0_6 as m; "
            "p=m.ROOT/'docs/parity/generated_registry_schema_original_5_0_6.json'; "
            "r=json.loads(p.read_text()); r['source_inputs'][next(iter(r['source_inputs']))]='0'*64; "
            "m.verify_inputs(r,p.name)")
        business_source = (
            "import verify_business_runtime_configuration_5_0_6 as m; "
            "m.digest=lambda path:'0'*64; m.source_inputs()")
        retained_record = (
            "import os; from pathlib import Path; "
            "import verify_legacy_observation_identity_5_0_6 as m; "
            "m.ROOT=Path(os.environ['TEAQL_CONTROL_TMPDIR']); "
            "m.verify_record(m.ROOT/'docs/parity/business_runtime_configuration_original_5_0_6.json')")
        valid_record = (
            "import verify_legacy_observation_identity_5_0_6 as m; "
            "m.verify_record(m.ROOT/'docs/parity/business_runtime_configuration_original_5_0_6.json')")

        for optimized in (False, True):
            mode = 'optimized' if optimized else 'normal'
            valid = [
                ('kernel', [str(ROOT / 'scripts/verify_runtime_configuration_kernel_5_0_6.py'),
                            '--cpp', executables['kernel']]),
                ('business', [str(ROOT / 'scripts/verify_business_runtime_configuration_5_0_6.py'),
                              '--cpp', executables['business']]),
                ('registry', [str(ROOT / 'scripts/verify_generated_registry_schema_5_0_6.py'),
                              '--cpp', executables['registry']]),
                ('retained record', ['-c', valid_record]),
            ]
            for label, command in valid:
                check_result(run(optimized, command, directory), True, mode + ' ' + label)

            rejected = [
                ('altered source record', ['-c', source_record], 'loaded record changed'),
                ('wrong business source hash', ['-c', business_source], 'lib/Cargo.toml'),
                ('altered retained record', ['-c', retained_record], 'original observation record changed'),
                ('empty registry observer',
                 [str(ROOT / 'scripts/verify_generated_registry_schema_5_0_6.py'),
                  '--cpp', str(empty)], 'Incomplete registry/schema observations'),
            ]
            for label, command, diagnostic in rejected:
                check_result(run(optimized, command, directory), False,
                             mode + ' ' + label, diagnostic)

    print('Configuration source checks: 35 exact rewrites and 16 normal/optimized controls passed')


if __name__ == '__main__':
    main()
