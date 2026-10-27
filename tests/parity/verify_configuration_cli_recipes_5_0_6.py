#!/usr/bin/env python3
"""Check configuration input recipes against unchanged collectors and corpora.

Only the pure original input-producing AST is evaluated. This verifies complete
input selection; it does not execute or qualify the original runtime observers.
"""

import argparse
import ast
import gzip
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
import sys

from compare_values import same
from configuration_cli_cases_5_0_6 import bootstrap_inputs, scalar_candidate_inputs
from expression_cli_cases import require_cases
from verify_configuration_cli_admission_5_0_6 import FAMILIES

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def collector_inputs(path):
    tree = ast.parse(path.read_text())
    main = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'main')
    node = next(n for n in main.body if isinstance(n, ast.Assign)
                and len(n.targets) == 1 and isinstance(n.targets[0], ast.Name)
                and n.targets[0].id == 'inputs')
    if not isinstance(node.value, ast.ListComp):
        raise RuntimeError('Original bootstrap collector recipe is no longer a list comprehension')
    return eval(compile(ast.Expression(node.value), str(path), 'eval'), {'itertools': itertools})


def pure_function(path, name):
    tree = ast.parse(path.read_text())
    function = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == name)
    module = ast.Module(body=[function], type_ignores=[])
    namespace = {'itertools': itertools}
    exec(compile(module, str(path), 'exec'), namespace)
    return namespace[name]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--ctest-build-dir', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical evidence')
    sources = {}
    expected_bootstrap = bootstrap_inputs()
    if len(expected_bootstrap) != 96:
        raise RuntimeError('Bootstrap recipe cardinality changed')
    for name in ('constant_metadata', 'constant_topology', 'type_configuration'):
        path = ROOT / f'scripts/verify_{name}_execution.py'
        if not same(expected_bootstrap, collector_inputs(path)):
            raise RuntimeError('Bootstrap recipe differs from original collector: ' + name)
        sources[path.relative_to(ROOT).as_posix()] = digest(path)
    scalar_path = ROOT / 'scripts/verify_scalar_candidate_execution.py'
    original_scalar = pure_function(scalar_path, 'inputs')
    sources[scalar_path.relative_to(ROOT).as_posix()] = digest(scalar_path)
    # Both authentic model cardinalities plus a wider pure-function probe.
    for count in range(9):
        if not same(scalar_candidate_inputs(count), original_scalar(count)):
            raise RuntimeError('Scalar recipe differs from original collector')
    extension_path = ROOT / 'tests/parity/compare_constant_topology_extensions_5_0_6.py'
    extension_inputs = list(pure_function(extension_path, 'cases')())
    sources[extension_path.relative_to(ROOT).as_posix()] = digest(extension_path)
    records = []
    for prefix, corpus_prefix, fixture, _, cases in FAMILIES:
        for case in cases:
            path = ROOT / f'tests/parity/{corpus_prefix}_{case}_5_0_6_cases.json.gz'
            corpus = json.loads(gzip.decompress(path.read_bytes()))
            if prefix == 'scalar_candidate':
                metadata = ROOT / f'tests/generator/parity/{fixture}/{case}/rust/OBSERVATION.json'
                field = next(f for f in json.loads(metadata.read_text())['fields'] if f['field'] == 'choice')
                count = len(field['candidates'])
                if count not in (2, 3):
                    raise RuntimeError('Authentic candidate cardinality changed')
                inputs = scalar_candidate_inputs(count)
                sources[metadata.relative_to(ROOT).as_posix()] = digest(metadata)
            elif prefix == 'constant_topology_extensions':
                inputs = extension_inputs
            else:
                inputs = expected_bootstrap
            require_cases(corpus, len(inputs), inputs)
            if corpus['case'] != case:
                raise RuntimeError('Retained corpus selects another model variant')
            records.append(dict(name=prefix + '_' + case, count=len(inputs),
                                corpus=path.relative_to(ROOT).as_posix(), corpus_sha256=digest(path)))
    if len(records) != 30 or sum(r['count'] for r in records) != 5080:
        raise RuntimeError('Complete configuration recipe inventory changed')
    invocations = []
    if args.ctest_build_dir is not None:
        build = args.ctest_build_dir.resolve()
        tests = json.loads(subprocess.check_output(
            ['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))['tests']
        scripts = {str(ROOT / 'tests/parity' / f[3]) for f in FAMILIES}
        selected = {t['name']: t for t in tests
                    if len(t.get('command', [])) > 1 and t['command'][1] in scripts}
        expected_names = {f'{prefix}_{case}_rust_parity'
                          for prefix, _, _, _, cases in FAMILIES for case in cases}
        if set(selected) != expected_names:
            raise RuntimeError('Actual CTest configuration selection differs from complete recipe inventory')
        for prefix, _, _, script, cases in FAMILIES:
            for case in cases:
                test = selected[f'{prefix}_{case}_rust_parity']
                command = test['command']
                expected = [str(ROOT / 'tests/parity' / script), '--case', case, '--cpp',
                            str(build / f'teaql_{prefix}_{case}_probe')]
                if prefix == 'scalar_candidate':
                    expected.append('--cpp-emulator=')
                if (Path(command[0]).resolve() != Path(sys.executable).resolve()
                        or command[1:] != expected):
                    raise RuntimeError('Actual CTest configuration arguments changed: ' + test['name'])
                properties = {p['name']: p['value'] for p in test.get('properties', [])}
                if 'configuration_cli_recipes' not in properties.get('FIXTURES_REQUIRED', []):
                    raise RuntimeError('Configuration comparison is missing its recipe prerequisite')
                invocations.append(test)
    if args.record is not None:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, original_runtime_executed=False,
            scope='Pure collector recipes and complete retained input matrices only.',
            source_files=sources, datasets=records, ctest_invocations=invocations,
            support_files={str(p.relative_to(ROOT)): digest(p) for p in (
                Path(__file__).resolve(), ROOT / 'tests/parity/configuration_cli_cases_5_0_6.py',
                ROOT / 'tests/parity/expression_cli_cases.py',
                ROOT / 'tests/parity/verify_configuration_cli_admission_5_0_6.py')}),
            indent=2) + '\n').encode(), mtime=0))
    print('Verified 30 configuration recipes / 5080 retained inputs against original collector recipes')


if __name__ == '__main__':
    main()
