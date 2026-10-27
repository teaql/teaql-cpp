#!/usr/bin/env python3
"""Require complete comparator assertion rewrites and optimized provenance rejection."""

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
PARITY = ROOT / 'tests/parity'
MANIFEST = ROOT / 'docs/parity/remaining_comparator_assert_rewrite_installed_5_0_6.json.gz'
PREEDIT = ROOT / 'docs/parity/dynamic_json_policy_baseline_optimized_preedit_5_0_6.json.gz'
sys.path.insert(0, str(PARITY))
sys.path.insert(0, str(ROOT / 'scripts'))
from generate_dynamic_json_policy_5_0_6 import CORPUS, adapted
from optional_source_guard_transition_5_0_6 import reconcile_assert_rewrite_source_files


def read(path):
    with gzip.open(path, 'rt') as source:
        return json.load(source)


def digest(data):
    return hashlib.sha256(data).hexdigest()


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        failure = ast.Raise(
            exc=ast.Call(func=ast.Name(id='AssertionError', ctx=ast.Load()),
                         args=[node.msg] if node.msg else [], keywords=[]), cause=None)
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[failure], orelse=[])


def verify_source(manifest):
    if not (manifest['complete'] is True and manifest['installed'] is True
            and manifest['source_hashes_verified'] is True
            and manifest['ast_structural_equivalence'] is True
            and manifest['scripts'] == 51 and manifest['assertions'] == 425):
        raise RuntimeError('Installed comparator rewrite manifest is incomplete')
    rows = manifest['rows']
    if len(rows) != 51 or len({row['path'] for row in rows}) != 51:
        raise RuntimeError('Comparator rewrite paths are incomplete')
    count = 0
    for row in rows:
        path = row['path']
        original = subprocess.check_output(
            ['git', 'show', manifest['source_commit'] + ':' + path], cwd=ROOT)
        current = (ROOT / path).read_bytes()
        if digest(original) != row['before_sha256'] or digest(current) != row['after_sha256']:
            raise RuntimeError('Comparator source identity changed: ' + path)
        old_tree = ast.parse(original.decode())
        new_tree = ast.parse(current.decode())
        nodes = [node for node in ast.walk(old_tree) if isinstance(node, ast.Assert)]
        if len(nodes) != row['assertions'] or any(
                isinstance(node, ast.Assert) for node in ast.walk(new_tree)):
            raise RuntimeError('Comparator assertion count changed: ' + path)
        expected = ReplaceAssert().visit(old_tree)
        if ast.dump(expected, include_attributes=False) != ast.dump(
                new_tree, include_attributes=False):
            raise RuntimeError('Comparator AST differs beyond assertion rewrite: ' + path)
        count += len(nodes)
    if count != 425:
        raise RuntimeError('Incomplete comparator assertion coverage')
    return count


def provenance_controls():
    prior = read(PREEDIT)
    if not (prior['complete'] is True and prior['cases'] == 3072
            and prior['normal']['exit_code'] != 0
            and prior['optimized']['exit_code'] == 0
            and prior['temporary_directory_removed'] is True):
        raise RuntimeError('Pre-edit provenance failure witness changed')
    source = json.loads(gzip.decompress(CORPUS.read_bytes()))
    if len(source['cases']) != 3072:
        raise RuntimeError('Dynamic JSON policy corpus changed size')
    changed = dict(source)
    changed['baseline'] = 'deliberately-wrong-locked-baseline'
    results = []
    with tempfile.TemporaryDirectory(prefix='teaql_comparator_assert_controls_') as owned:
        directory = Path(owned)
        altered = directory / 'altered.json.gz'
        altered.write_bytes(gzip.compress(json.dumps(changed, ensure_ascii=False).encode(), mtime=0))
        outputs = directory / 'outputs.jsonl'
        outputs.write_text(''.join(json.dumps(adapted(row['expected'])) + '\n'
                                   for row in source['cases']))
        fake = directory / 'saved_result_probe.py'
        fake.write_text('#!/usr/bin/env python3\nimport sys\nfrom pathlib import Path\n'
                        'requests = sys.stdin.read().splitlines()\n'
                        'if len(requests) != 3072:\n'
                        '    raise SystemExit("wrong request count")\n'
                        'sys.stdout.write(Path(' + repr(str(outputs)) + ').read_text())\n')
        fake.chmod(0o700)
        wrapper = directory / 'run_comparator.py'
        wrapper.write_text('import sys\nfrom pathlib import Path\n'
                           'sys.path.insert(0, ' + repr(str(PARITY)) + ')\n'
                           'import compare_dynamic_json_policy_5_0_6 as comparator\n'
                           'comparator.CORPUS = Path(sys.argv[1])\n'
                           'sys.argv = ["comparator", "--cpp", sys.argv[2]]\n'
                           'comparator.main()\n')
        for label, path in [('valid', CORPUS), ('altered', altered)]:
            for optimized in (False, True):
                command = [sys.executable]
                if optimized:
                    command.append('-O')
                command += [str(wrapper), str(path), str(fake)]
                environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
                environment.pop('PYTHONOPTIMIZE', None)
                result = subprocess.run(command, cwd=ROOT, env=environment,
                                        text=True, capture_output=True, timeout=120)
                if label == 'valid':
                    if result.returncode != 0 or '3072/3072' not in result.stdout:
                        raise RuntimeError('Valid saved-result comparator rejected')
                elif result.returncode == 0 or 'Changed provenance: baseline' not in result.stderr:
                    raise RuntimeError('Altered baseline accepted under optimized Python')
                results.append(dict(label=label, optimized=optimized,
                                    exit_code=result.returncode))
    if directory.exists() or len(results) != 4:
        raise RuntimeError('Incomplete comparator provenance controls or cleanup')
    return results


def clock_transition_controls():
    contract = json.loads((PARITY / 'clock_cli_source_contracts_5_0_6.json').read_text())
    historical = read(ROOT / contract['source_execution']['path'])
    recorded = historical['source_files']
    current = contract['source_files']
    transition = contract['assert_rewrite_transition']
    if reconcile_assert_rewrite_source_files(ROOT, recorded, current, transition) != current:
        raise RuntimeError('Valid Clock source transition rejected')
    changed = transition['paths'][0]
    unchanged = next(name for name in current if name not in transition['paths'])
    candidates = []
    altered = dict(current)
    altered[changed] = '0' * 64
    candidates.append(('changed_current_hash', recorded, altered, transition))
    altered = dict(recorded)
    altered[changed] = '0' * 64
    candidates.append(('changed_original_hash', altered, current, transition))
    altered = dict(current)
    altered[unchanged] = '0' * 64
    candidates.append(('unlisted_source_change', recorded, altered, transition))
    altered = copy.deepcopy(transition)
    altered['paths'] = []
    candidates.append(('missing_transition_path', recorded, current, altered))
    altered = copy.deepcopy(transition)
    altered['rewrite']['sha256'] = '0' * 64
    candidates.append(('changed_rewrite_receipt', recorded, current, altered))
    altered = copy.deepcopy(transition)
    altered['helper_sha256'] = '0' * 64
    candidates.append(('changed_helper_identity', recorded, current, altered))
    results = []
    for label, old, new, scope in candidates:
        try:
            reconcile_assert_rewrite_source_files(ROOT, old, new, scope)
        except RuntimeError:
            results.append(dict(mutation=label, rejected=True))
        else:
            raise RuntimeError('Accepted corrupted Clock source transition: ' + label)
    if len(results) != 6:
        raise RuntimeError('Incomplete Clock source transition controls')
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve existing evidence; use a fresh record path')
    manifest = read(MANIFEST)
    count = verify_source(manifest)
    results = provenance_controls()
    clock_controls = clock_transition_controls()
    if args.record is not None:
        receipt = dict(scope=__doc__, complete=True, whole_goal_complete=False,
                       manifest_sha256=digest(MANIFEST.read_bytes()),
                       scripts=51, assertions=count, provenance_controls=results,
                       clock_transition_controls=clock_controls,
                       temporary_directory_removed=True)
        args.record.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(),
                                              mtime=0))
    print('Comparator assertion rewrites: 51 scripts / 425 checks; '
          'valid and altered provenance controls pass under normal and optimized Python; '
          'six Clock transition corruptions rejected')


if __name__ == '__main__':
    main()
