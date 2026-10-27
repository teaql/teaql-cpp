#!/usr/bin/env python3
"""Reject legacy locks and corruptions for a registered declared-driver cohort."""
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
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_declared_driver_contracts_5_0_6 import check_dependency_lock, digest, read, validate
from compare_values import json_lines

REWRITE = ROOT / 'docs/parity/declared_driver_assert_rewrite_5_0_6.json.gz'


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        failure = ast.Raise(
            exc=ast.Call(func=ast.Name(id='AssertionError', ctx=ast.Load()),
                         args=[node.msg] if node.msg else [], keywords=[]), cause=None)
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[failure], orelse=[])


def verify_assert_rewrite():
    manifest = read(REWRITE)
    if (manifest['complete'] is not True or manifest['ast_equivalent'] is not True
            or manifest['scripts'] != 2 or manifest['assertions'] != 44):
        raise RuntimeError('Declared-driver assertion rewrite is incomplete')
    if len(manifest['rows']) != 2:
        raise RuntimeError('Declared-driver rewrite paths are incomplete')
    count = 0
    for row in manifest['rows']:
        original = row['before_source']
        current = (ROOT / row['path']).read_text()
        if (hashlib.sha256(original.encode()).hexdigest() != row['before_sha256']
                or hashlib.sha256(current.encode()).hexdigest() != row['after_sha256']):
            raise RuntimeError('Declared-driver rewrite source changed: ' + row['path'])
        old_tree = ast.parse(original)
        new_tree = ast.parse(current)
        nodes = [node for node in ast.walk(old_tree) if isinstance(node, ast.Assert)]
        if len(nodes) != row['assertions'] or any(
                isinstance(node, ast.Assert) for node in ast.walk(new_tree)):
            raise RuntimeError('Declared-driver assertion coverage changed: ' + row['path'])
        if ast.dump(ReplaceAssert().visit(old_tree), include_attributes=False) != ast.dump(
                new_tree, include_attributes=False):
            raise RuntimeError('Declared-driver rewrite changed other behavior: ' + row['path'])
        count += len(nodes)
    if count != 44:
        raise RuntimeError('Declared-driver assertion count changed')
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path, required=True)
    args = parser.parse_args()
    if not (args.record is None or not args.record.exists()):
        raise AssertionError()
    execution_path, registry_path = args.execution.resolve(), args.contracts.resolve()
    execution, registry = read(execution_path), read(registry_path)
    rewrites = verify_assert_rewrite()
    paths = [Path(__file__), execution_path, registry_path, ROOT / 'scripts/verify_declared_driver_contracts_5_0_6.py']
    hashes = lambda: {str(p): digest(p) for p in paths}
    record = dict(complete=False, scope=__doc__, controls=[], optimized_controls=[],
                  assert_rewrite_count=rewrites, rewrite_manifest_sha256=digest(REWRITE),
                  input_hashes_before=hashes())
    validate(execution, registry)

    def reject(name, mutate):
        changed = copy.deepcopy(execution)
        mutate(changed)
        if not (changed != execution):
            raise AssertionError()
        try:
            validate(changed, registry)
        except (AssertionError, KeyError, StopIteration, ValueError):
            record['controls'].append(dict(name=name, rejected=True))
        else:
            raise AssertionError('Corrupt declared-driver evidence accepted: ' + name)

    reject('different_runtime_baseline', lambda r: r['registry'].update(runtime_commit='0' * 40))
    reject('missing_reference_source', lambda r: r['reference_source_files'].pop(next(iter(r['reference_source_files']))))
    reject('changed_reference_source', lambda r: r['reference_source_files'].update({next(iter(r['reference_source_files'])): '0' * 64}))
    reject('shared_target_is_not_owned_build', lambda r: r['cargo_environment'].update(CARGO_TARGET_DIR='/tmp/shared_target'))
    project = next(iter(execution['projects']))
    build = execution['projects'][project]['build_step']
    reject('unlocked_compilation', lambda r: r['steps'][build]['command'].remove('--locked'))
    reject('failed_compilation', lambda r: r['steps'][build].update(exit_code=1))
    reject('manifest_override', lambda r: r['projects'][project].update(manifest=r['projects'][project]['manifest'] + '\n[patch.crates-io]\n'))
    reject('changed_locked_dependency_record', lambda r: r['projects'][project].update(lock=r['projects'][project]['lock'] + '\n# replaced\n'))

    def metadata(r, edit):
        step = r['steps'][r['projects'][project]['metadata_step']]
        value = json.loads(step['stdout'])
        edit(value)
        step['stdout'] = json.dumps(value)

    def wrong_target(value):
        package = next(p for p in value['packages'] if p['manifest_path'] == str(ROOT / project))
        selected = next(c['target'] for c in registry['contracts'] if c['project'] == project)
        next(t for t in package['targets'] if t['name'] == selected)['src_path'] = '/tmp/not_the_declared_driver.rs'

    reject('different_compiled_driver', lambda r: metadata(r, wrong_target))

    def wrong_local_dependency(value):
        next(p for p in value['packages'] if p['name'] == 'teaql-core')['manifest_path'] = '/tmp/replaced_core/Cargo.toml'

    reject('unaccounted_local_dependency', lambda r: metadata(r, wrong_local_dependency))
    reject('missing_original_matrix', lambda r: r['contracts'].pop())
    result = execution['contracts'][0]
    reject('changed_original_input_payload', lambda r: r['steps'][result['original_step']].update(stdin_sha256='0' * 64))

    def output(r, side, edit, index=0):
        item = r['contracts'][index]
        step = r['steps'][item[side + '_step']]
        values = json_lines(step['stdout'])
        edit(values)
        step['stdout'] = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in values)

    reject('missing_original_record', lambda r: output(r, 'original', lambda v: v.pop()))
    reject('duplicate_original_record', lambda r: output(r, 'original', lambda v: v.append(v[0])))
    reject('missing_original_member', lambda r: output(r, 'original', lambda v: v[0].pop(next(iter(v[0])))))
    reject('missing_native_record', lambda r: output(r, 'native', lambda v: v.pop()))
    reject('duplicate_native_record', lambda r: output(r, 'native', lambda v: v.append(v[0])))

    def replace_bool(values):
        def visit(value):
            if isinstance(value, dict):
                for key, child in value.items():
                    if type(child) is bool:
                        value[key] = int(child)
                        return True
                    if visit(child):
                        return True
            if isinstance(value, list):
                for i, child in enumerate(value):
                    if type(child) is bool:
                        value[i] = int(child)
                        return True
                    if visit(child):
                        return True
            return False

        if not (visit(values)):
            raise AssertionError('Positive boolean field required')

    def has_bool(value):
        if type(value) is bool:
            return True
        if isinstance(value, dict):
            return any(has_bool(v) for v in value.values())
        if isinstance(value, list):
            return any(has_bool(v) for v in value)
        return False

    boolean_index = next(i for i, item in enumerate(execution['contracts'])
                         if has_bool(json_lines(execution['steps'][item['native_step']]['stdout'])))
    reject('native_boolean_is_not_integer', lambda r: output(r, 'native', replace_bool, boolean_index))
    for name in ('rust_field_layout', 'rust_sql_protocol_5_0_6'):
        lock = ROOT / 'tests/parity' / name / 'Cargo.lock'
        try:
            check_dependency_lock(lock, ROOT / '.reference/teaql-rs/Cargo.lock')
        except AssertionError:
            record['controls'].append(dict(name='actual_legacy_lock_' + name, rejected=True, lock_sha256=digest(lock)))
        else:
            raise AssertionError('Actual legacy lock unexpectedly qualifies: ' + name)
    if not (len(record['controls']) == 20):
        raise AssertionError()

    with tempfile.TemporaryDirectory(prefix='teaql_declared_driver_optimized_') as owned:
        directory = Path(owned)
        incomplete = copy.deepcopy(execution)
        incomplete['complete'] = False
        missing_output = copy.deepcopy(execution)
        first = missing_output['contracts'][0]
        missing_output['steps'][first['native_step']]['stdout'] = ''
        variants = {}
        for name, data in [('incomplete', incomplete), ('missing_native_output', missing_output)]:
            path = directory / (name + '.json.gz')
            path.write_bytes(gzip.compress((json.dumps(data) + '\n').encode(), mtime=0))
            variants[name] = path
        verifier = ROOT / 'scripts/verify_declared_driver_contracts_5_0_6.py'
        controls = [('valid', execution_path, False, True),
                    ('valid_optimized', execution_path, True, True),
                    ('incomplete', variants['incomplete'], False, False),
                    ('incomplete_optimized', variants['incomplete'], True, False),
                    ('missing_native_output_optimized', variants['missing_native_output'], True, False)]
        environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
        environment.pop('PYTHONOPTIMIZE', None)
        for name, path, optimized, should_pass in controls:
            command = [sys.executable] + (['-O'] if optimized else []) + [
                str(verifier), '--contracts', str(registry_path), '--execution', str(path)]
            result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                    capture_output=True, timeout=120)
            passed = ((result.returncode == 0) == should_pass
                      and ('complete source/corpus/native bindings verify' in result.stdout
                           if should_pass else 'AssertionError' in result.stderr))
            record['optimized_controls'].append(dict(name=name, optimized=optimized,
                                                      exit_code=result.returncode, passed=passed))
            if not passed:
                raise RuntimeError('Declared-driver optimized control failed: ' + name)
    record['temporary_directory_removed'] = not directory.exists()
    if not record['temporary_directory_removed'] or len(record['optimized_controls']) != 5:
        raise RuntimeError('Declared-driver optimized controls or cleanup incomplete')

    record['input_hashes_after'] = hashes()
    if not (record['input_hashes_before'] == record['input_hashes_after']):
        raise AssertionError()
    record['complete'] = True
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print(len(registry['contracts']), 'untouched bindings pass; 18 effective corruptions and '
          'two actual legacy locks reject; five normal/optimized CLI controls pass')


if __name__ == '__main__':
    main()
