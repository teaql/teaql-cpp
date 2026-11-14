#!/usr/bin/env python3
"""Pin the active generated-model verifier without changing historical receipts."""

import argparse
import ast
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / 'docs/parity/generated_driver_hardened_source_transition_5_0_6.json.gz'
MANIFEST_SHA256 = 'c27fb1cdd8b58c8e7379ce1aaf11ac96b081d9ce9599ca51b0dffe43773cf054'
WITNESS = ROOT / 'docs/parity/generated_driver_optimized_preedit_5_0_6.json.gz'
HARDENED_DRIVER = ROOT / 'scripts/generated_driver_provenance_hardened_5_0_6.py'
HARDENED_CONTROLS = ROOT / 'tests/parity/verify_generated_driver_controls_hardened_5_0_6.py'


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def digest(value):
    return hashlib.sha256(value).hexdigest()


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        error = ast.Name(id='AssertionError', ctx=ast.Load())
        if node.msg is not None:
            error = ast.Call(func=error, args=[node.msg], keywords=[])
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[ast.Raise(exc=error, cause=None)], orelse=[])


def check_row(row, original, candidate=None):
    before = row['before_source']
    assert_only = row['assert_only_source']
    after = row['after_source'] if candidate is None else candidate
    require(before == original['source']
            and digest(before.encode()) == row['before_sha256'] == original['sha256']
            and digest(after.encode()) == row['after_sha256']
            and digest((ROOT / row['historical_path']).read_bytes()) == row['before_sha256']
            and digest((ROOT / row['current_path']).read_bytes()) == row['after_sha256'],
            'Historical or hardened source identity changed')
    old_tree, converted_tree = ast.parse(before), ast.parse(assert_only)
    require(sum(isinstance(node, ast.Assert) for node in ast.walk(old_tree)) == row['assertions']
            and not any(isinstance(node, ast.Assert) for node in ast.walk(converted_tree))
            and ast.dump(ReplaceAssert().visit(old_tree), include_attributes=False)
            == ast.dump(converted_tree, include_attributes=False),
            'Assertion-only source transition changed')
    expected = assert_only
    for edit in row['adaptations']:
        require(expected.count(edit['before']) == 1, 'Ambiguous declared adaptation')
        expected = expected.replace(edit['before'], edit['after'], 1)
    require(expected == after
            and not any(isinstance(node, ast.Assert) for node in ast.walk(ast.parse(after))),
            'Undeclared hardened source change')


def check_binding(binding, registered):
    name = binding['test']
    require(name in registered, 'Missing registered generated-model verifier')
    command = registered[name]['command']
    require(str(HARDENED_DRIVER) in command
            and command[command.index('--contracts') + 1] == str(ROOT / binding['contracts'])
            and command[command.index('--execution') + 1] == str(ROOT / binding['execution'])
            and digest((ROOT / binding['contracts']).read_bytes()) == binding['contract_sha256']
            and digest((ROOT / binding['execution']).read_bytes()) == binding['execution_sha256'],
            'Hardened verifier registration or evidence changed: ' + name)
    controls = name.removesuffix('_contracts') + '_controls'
    require(controls in registered and str(HARDENED_CONTROLS) in registered[controls]['command']
            and registered[controls]['command'][registered[controls]['command'].index('--contracts') + 1]
            == str(ROOT / binding['contracts'])
            and registered[controls]['command'][registered[controls]['command'].index('--execution') + 1]
            == str(ROOT / binding['execution']),
            'Hardened negative-control registration changed: ' + controls)


def invoke(script, optimized, *arguments):
    command = [sys.executable, '-B'] + (['-O'] if optimized else [])
    command.extend([str(script), *map(str, arguments)])
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    environment.pop('PYTHONOPTIMIZE', None)
    environment.pop('PYTHONPATH', None)
    return subprocess.run(command, cwd=ROOT, env=environment,
                          capture_output=True, text=True, timeout=180)


def reject(label, action, controls):
    try:
        action()
    except AssertionError:
        controls.append(label)
    else:
        raise AssertionError('Corrupt hardened source accepted: ' + label)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), 'Preserve existing evidence')
    require(digest(MANIFEST.read_bytes()) == MANIFEST_SHA256, 'Transition manifest changed')
    manifest = json.loads(gzip.decompress(MANIFEST.read_bytes()))
    witness = json.loads(gzip.decompress(WITNESS.read_bytes()))
    require(manifest['complete'] is True and manifest['installed'] is True
            and manifest['whole_goal_complete'] is False
            and manifest['scripts'] == 2 and manifest['assertions'] == 79
            and manifest['witness_sha256'] == digest(WITNESS.read_bytes())
            and witness['complete'] is True and witness['mutation'] == {'complete': False}
            and [(r['optimized'], r['exit_code'] == 0) for r in witness['results']]
            == [(False, False), (True, True)],
            'Pre-edit false acceptance or transition scope changed')
    originals = {row['path']: row for row in witness['source_rows']}
    rows = manifest['rows']
    require(len(rows) == 2 and len(originals) == 2
            and {row['historical_path'] for row in rows} == set(originals)
            and {row['current_path'] for row in rows}
            == {str(HARDENED_DRIVER.relative_to(ROOT)),
                str(HARDENED_CONTROLS.relative_to(ROOT))},
            'Original/hardened source pairs changed')
    for row in rows:
        check_row(row, originals[row['historical_path']])
    require(sum(row['assertions'] for row in rows) == 79, 'Incomplete check inventory')

    registry = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(ROOT / 'build_native_current'), '--show-only=json-v1'],
        text=True))['tests']
    registered = {item['name']: item for item in registry}
    bindings = manifest['bindings']
    require(len(bindings) == 8 and len({item['test'] for item in bindings}) == 8,
            'Eight active generated-model source registrations')
    for binding in bindings:
        check_binding(binding, registered)
    require(witness['execution_sha256'] == digest((ROOT / witness['execution_path']).read_bytes())
            and witness['contract_sha256'] == digest((ROOT / witness['contract_path']).read_bytes()),
            'Pre-edit evidence inputs changed')

    controls = []
    first = rows[0]
    reject('changed_old_source_hash', lambda: check_row(
        dict(first, before_sha256='0' * 64), originals[first['historical_path']]), controls)
    reject('changed_hardened_source', lambda: check_row(
        first, originals[first['historical_path']], first['after_source'] + '\n'), controls)
    reject('missing_adaptation', lambda: check_row(
        dict(first, adaptations=first['adaptations'][:-1]), originals[first['historical_path']]),
        controls)
    altered = copy.deepcopy(bindings[0])
    altered['contract_sha256'] = '0' * 64
    reject('changed_contract_hash', lambda: check_binding(altered, registered), controls)
    altered_registry = copy.deepcopy(registered)
    altered_registry[bindings[0]['test']]['command'][1] = '/tmp/other_verifier.py'
    reject('changed_active_verifier', lambda: check_binding(bindings[0], altered_registry), controls)

    observed = []
    with tempfile.TemporaryDirectory(prefix='teaql_generated_hardened_sources_506_') as owned:
        invalid = Path(owned) / 'incomplete.json.gz'
        original = json.loads(gzip.decompress((ROOT / witness['execution_path']).read_bytes()))
        original['complete'] = False
        invalid.write_bytes(gzip.compress(json.dumps(original).encode(), mtime=0))
        contract = ROOT / witness['contract_path']
        execution = ROOT / witness['execution_path']
        for optimized in (False, True):
            valid = invoke(HARDENED_DRIVER, optimized, 'verify', '--contracts', contract,
                           '--execution', execution)
            require(valid.returncode == 0, 'Valid execution rejected: ' + valid.stderr[-800:])
            incomplete = invoke(HARDENED_DRIVER, optimized, 'verify', '--contracts', contract,
                                '--execution', invalid)
            require(incomplete.returncode != 0 and 'AssertionError' in incomplete.stderr,
                    'Incomplete execution accepted in mode ' + str(int(optimized)))
            control = invoke(HARDENED_CONTROLS, optimized, '--contracts', contract,
                             '--execution', execution)
            require(control.returncode == 0
                    and '24 generated-model provenance/result controls reject' in control.stdout,
                    'Representative negative controls failed: ' + control.stderr[-800:])
            observed.append({'optimized': optimized, 'valid_exit_code': valid.returncode,
                             'incomplete_exit_code': incomplete.returncode,
                             'control_exit_code': control.returncode})
        destination = Path(owned) / 'replay.json.gz'
        replay = invoke(HARDENED_DRIVER, True, 'replay', '--contracts', contract,
                        '--record', destination)
        require(replay.returncode != 0
                and 'Generated archive replay requires assertions' in replay.stderr
                and not destination.exists(), 'Optimized archive replay was not refused')
        removed_bytes = sum(path.stat().st_size for path in Path(owned).rglob('*')
                            if path.is_file())
    require(not Path(owned).exists(), 'Owned incomplete receipt was not removed')
    if args.record is not None:
        record = {'complete': True, 'whole_goal_complete': False,
                  'manifest_sha256': digest(MANIFEST.read_bytes()),
                  'witness_sha256': digest(WITNESS.read_bytes()),
                  'source_controls': controls, 'registered_contracts': 8,
                  'observed': observed, 'optimized_replay_refused': True,
                  'owned_bytes_removed': removed_bytes, 'owned_directory_removed': True}
        args.record.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(),
                                              mtime=0))
    print('Generated-model hardened sources: 79 checks, eight registrations, '
          'five source controls and normal/optimized evidence pass')


if __name__ == '__main__':
    main()
