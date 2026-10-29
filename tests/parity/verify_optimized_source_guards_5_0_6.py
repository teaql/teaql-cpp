#!/usr/bin/env python3
"""Keep provenance and diagnostic rejections active under optimized Python."""

import ast
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import textwrap


ROOT = Path(__file__).resolve().parents[2]
PARITY = ROOT / 'docs/parity'
MANIFESTS = (
    ('entity_runtime_assert_rewrite_candidate_5_0_6.json.gz', 2, 75),
    ('corpus_source_assert_rewrite_candidate_5_0_6.json.gz', 2, 48),
    ('expression_framing_assert_rewrite_candidate_5_0_6.json.gz', 1, 9),
    ('native_epoch_controller_assert_rewrite_candidate_5_0_6.json.gz', 1, 10),
)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(gzip.decompress(path.read_bytes()) if path.suffix == '.gz'
                      else path.read_bytes())


class ReplaceAssert(ast.NodeTransformer):
    def visit_Assert(self, node):
        self.generic_visit(node)
        failure = ast.Raise(
            exc=ast.Call(func=ast.Name(id='AssertionError', ctx=ast.Load()),
                         args=[node.msg] if node.msg else [], keywords=[]), cause=None)
        return ast.If(test=ast.UnaryOp(op=ast.Not(), operand=node.test),
                      body=[failure], orelse=[])


def verify_rewrites():
    total = 0
    paths = set()
    for name, expected_scripts, expected_assertions in MANIFESTS:
        manifest = read(PARITY / name)
        candidate_only = name.startswith('native_epoch_controller_')
        require(manifest['candidate_complete'] is True
                and manifest['installed'] is False
                and manifest['whole_goal_complete'] is False
                and manifest['ast_equivalent'] is True,
                'Candidate state changed: ' + name)
        rows = manifest.get('rows', [manifest])
        require(len(rows) == expected_scripts
                and sum(row['assertions'] for row in rows) == expected_assertions,
                'Candidate scope changed: ' + name)
        witness = name.replace('assert_rewrite_candidate', 'optimized_preedit_witness')
        require(digest(PARITY / witness) == manifest['witness_sha256'],
                'Pre-edit witness changed: ' + name)
        for row in rows:
            path = ROOT / row['path']
            require(row['path'] not in paths, 'Duplicate candidate source: ' + row['path'])
            paths.add(row['path'])
            before = row['before_source']
            after = row['after_source']
            require(hashlib.sha256(before.encode()).hexdigest() == row['before_sha256']
                    and hashlib.sha256(after.encode()).hexdigest() == row['after_sha256']
                    and digest(path) == row['before_sha256' if candidate_only else 'after_sha256'],
                    'Source differs from bounded rewrite: ' + row['path'])
            original = ast.parse(before)
            installed = ast.parse(after)
            original_asserts = sum(isinstance(node, ast.Assert)
                                   for node in ast.walk(original))
            require(original_asserts == row['assertions']
                    and not any(isinstance(node, ast.Assert)
                                for node in ast.walk(installed))
                    and ast.dump(ReplaceAssert().visit(original), include_attributes=False)
                    == ast.dump(installed, include_attributes=False),
                    'Unconditional-check rewrite changed behavior: ' + row['path'])
            total += original_asserts
    require(len(paths) == 6 and total == 142, 'Incomplete installed rewrite scope')


def run_case(name, code, argument, *, optimized, should_pass, input_text=None,
             expected_output='ACCEPTED\n'):
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    environment.pop('PYTHONOPTIMIZE', None)
    command = [sys.executable] + (['-O'] if optimized else []) + ['-c', code, argument]
    result = subprocess.run(command, cwd=ROOT, env=environment, input=input_text,
                            text=True, capture_output=True, timeout=120)
    require((result.returncode == 0) == should_pass,
            name + ': unexpected subprocess exit ' + str(result.returncode))
    if should_pass:
        require(result.stdout == expected_output and not result.stderr,
                name + ': successful output differs')
    else:
        require('AssertionError' in result.stderr,
                name + ': wrong rejection reason')


def four_modes(label, code, valid_argument, corrupt_argument, *, input_text=None,
               expected_output='ACCEPTED\n'):
    for argument, valid in ((valid_argument, True), (corrupt_argument, False)):
        for optimized in (False, True):
            run_case(label + ':' + argument + (':optimized' if optimized else ':ordinary'),
                     code, argument, optimized=optimized, should_pass=valid,
                     input_text=input_text, expected_output=expected_output)


def check_entity_runtime():
    code = textwrap.dedent('''\
        import sys
        sys.path.insert(0, 'scripts')
        import verify_entity_runtime_driver_contracts_5_0_6 as verifier
        registry = verifier.read(verifier.ROOT / 'tests/parity/entity_runtime_driver_contracts_5_0_6.json')
        execution = verifier.read(verifier.ROOT / 'docs/parity/entity_runtime_driver_execution_5_0_6.json.gz')
        if sys.argv[1] == 'incomplete':
            execution['complete'] = False
        verifier.validate(execution, registry)
        print('ACCEPTED')
    ''')
    four_modes('entity-runtime', code, 'valid', 'incomplete')


def check_corpus_source():
    code = textwrap.dedent('''\
        import json
        from pathlib import Path
        import sys
        import tempfile
        sys.path.insert(0, 'scripts')
        import verify_corpus_source_contracts_5_0_6 as verifier
        registry = json.loads((verifier.ROOT / 'tests/parity/corpus_source_contracts_5_0_6.json').read_text())
        contract = registry['contracts'][0]
        receipt = verifier.read(verifier.ROOT / contract['receipt'])
        corpus = verifier.read(verifier.ROOT / contract['corpus'])
        if sys.argv[1] == 'incomplete':
            receipt['complete'] = False
        with tempfile.TemporaryDirectory(prefix='teaql_corpus_optimized_control_') as owned:
            verifier.validate_source(contract, receipt, corpus, Path(owned))
        print('ACCEPTED')
    ''')
    four_modes('corpus-source', code, 'valid', 'incomplete')


def check_expression_framing():
    witness = read(PARITY / 'expression_framing_optimized_preedit_witness_5_0_6.json.gz')
    require(digest(ROOT / 'tests/parity/expression_diagnostic_framing_5_0_6_cases.json.gz')
            == witness['corpus_sha256'], 'Expression corpus changed')
    corpus = read(ROOT / 'tests/parity/expression_diagnostic_framing_5_0_6_cases.json.gz')
    case = next(row for row in corpus['cases'] if row['input'] == witness['case_input'])
    expected = case['expected']['raw']
    require(hashlib.sha256(expected.encode()).hexdigest() == witness['original_text_sha256'],
            'Original expression text changed')
    code = textwrap.dedent('''\
        import json
        import sys
        sys.path.insert(0, 'tests/parity')
        from expression_diagnostic_framing import to_source
        value = json.load(sys.stdin)
        if sys.argv[1] == 'extra':
            value['invented_field'] = 'fake'
        sys.stdout.write(to_source(value))
    ''')
    four_modes('expression-framing', code, 'valid', 'extra',
               input_text=json.dumps(witness['native_output']), expected_output=expected)


def check_native_epoch_controller():
    manifest = read(PARITY / 'native_epoch_controller_assert_rewrite_candidate_5_0_6.json.gz')
    source = manifest.get('rows', [manifest])[0]['after_source']
    node = next(node for node in ast.walk(ast.parse(source))
                if isinstance(node, ast.FunctionDef) and node.name == 'check_inputs')
    function = textwrap.dedent(ast.get_source_segment(source, node))
    code = textwrap.dedent('''\
        import sys
        original = {'runtime:input': 'original'}
        current = {'runtime:input': 'changed'} if sys.argv[1] == 'changed' else original
        scope = {'capture_inputs': lambda: current, 'before': original,
                 'record': {}, 'save': lambda: None,
                 'references': lambda: {'teaql-rs': 'locked'},
                 'ref_before': {'teaql-rs': 'locked'}}
        exec(compile(sys.stdin.read(), 'frozen_check_inputs', 'exec'), scope)
        scope['check_inputs']()
        print('ACCEPTED')
    ''')
    four_modes('native-epoch-candidate', code, 'unchanged', 'changed', input_text=function)
    live_source = (ROOT / 'scripts/run_native_integration_epoch_5_0_6.py').read_text()
    live_node = next(node for node in ast.walk(ast.parse(live_source))
                     if isinstance(node, ast.FunctionDef) and node.name == 'check_inputs')
    live_function = textwrap.dedent(ast.get_source_segment(live_source, live_node))
    for argument, valid in (('unchanged', True), ('changed', False)):
        run_case('native-epoch-live:' + argument, code, argument, optimized=False,
                 should_pass=valid, input_text=live_function)


def main():
    verify_rewrites()
    check_entity_runtime()
    check_corpus_source()
    check_expression_framing()
    check_native_epoch_controller()
    print('Three installed repairs, one locked controller candidate, and eighteen controls pass')


if __name__ == '__main__':
    main()
