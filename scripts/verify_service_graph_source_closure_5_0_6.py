#!/usr/bin/env python3
"""Replay the locked complete graph-service source and current consuming contracts."""
import argparse
import ast
import gzip
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from run_native_integration_epoch_5_0_6 import capture_inputs, references
from verify_entity_save_source_closure_5_0_6 import digest, literal

ROOT = Path(__file__).resolve().parents[1]
FAMILIES = ('ledger_plan', 'ledger_execution', 'graph_execution', 'graph_save_adapter',
            'graph_node_command', 'recursive_graph_write', 'entity_graph_plan', 'graph_plan_scopes')
TESTS = [
    'graph_outcome_models_rust_parity',
    'ledger_execution_context_contracts', 'ledger_execution_rust_parity',
    'ledger_plan_context_contracts', 'ledger_plan_rust_parity',
    'entity_graph_execution_context_contracts', 'graph_execution_rust_parity',
    'graph_save_adapter_context_contracts', 'graph_save_adapter_rust_parity',
    'recursive_graph_write_context_contracts', 'recursive_graph_write_rust_parity',
    'graph_node_command_context_contracts', 'graph_node_command_rust_parity',
    'entity_graph_plan_context_contracts', 'entity_graph_plan_rust_parity',
    'graph_plan_scopes_rust_parity', 'graph_planner_scopes_rust_parity',
    'graph_planner_upsert_scopes_rust_parity', 'graph_planner_complete_rows_rust_parity',
    'legacy_graph_intent_rust_parity', 'legacy_graph_intent_extended_rust_parity',
    'graph_readback_source_contracts', 'dynamic_graph_save_contracts',
    'sqlite_ledger_allocated_trace_contracts', 'original_runtime_entity_save_contracts',
    'generated_inheritance_graph_contracts', 'generated_inheritance_graph_rust_parity',
    'generated_runtime_state_bridge_context_contracts', 'commit_audit_transaction_contracts',
    'runtime_generic_completion_contracts',
]


def load(path):
    return json.loads(gzip.decompress(path.read_bytes()))


def driver(path):
    code = path.read_text()
    assert code.count('fn main(') == 1
    code = code[:code.index('fn main(')].replace('teaql_runtime::', 'crate::')
    code, count = re.subn(r'#\[path\s*=\s*"[^"]*fixture_value.rs"\]',
        '#[path = ' + json.dumps(str(ROOT / 'tests/parity/rust_values/src/fixture_value.rs')) + ']', code)
    assert count == 1
    return code


def corpus_test(work, name, cases):
    path = work / (name + '.json')
    path.write_text(json.dumps(cases))
    return '''
#[test] fn ''' + name + '''() {
    let cases: serde_json::Value = serde_json::from_str(
        &std::fs::read_to_string(''' + json.dumps(str(path)) + ''').unwrap()).unwrap();
    for (index, case) in cases.as_array().unwrap().iter().enumerate() {
        assert_eq!(evaluate(case["input"].clone()),case["expected"],"retained case {index}");
    }
}
'''


def original(work, run, record):
    parity = ROOT / 'tests/parity'
    graph_bridge = literal(parity / 'build_graph_execution_oracle.py', 'BRIDGE')
    node_bridge = literal(parity / 'build_graph_execution_oracle.py', 'NODE_BRIDGE')
    node_exports = literal(parity / 'build_graph_execution_oracle.py', 'NODE_EXPORTS')
    ledger_bridge = literal(parity / 'build_ledger_execution_oracle.py', 'BRIDGE')
    before = '.execute_ledger_plan_internal(root,locations).await'
    assert ledger_bridge.count(before) == 1
    ledger_bridge = ledger_bridge.replace(before, before + '.map(|outcome| {\n'
        'assert!(outcome.dynamic_readbacks.is_none(), "historical observer cannot omit dynamic readbacks");\n'
        'outcome.generated_ids\n})')
    service_bridge = graph_bridge + literal(parity / 'build_ledger_plan_oracle.py', 'BRIDGE') + ledger_bridge
    service_bridge += '\npub use graph::{' + node_exports + '};\n'
    exports = ('\npub use data_service::{__cpp_execute_graph,__cpp_save_graph,__cpp_current_row,'
        '__cpp_save_entity_adapter,__cpp_save_core_graph,__cpp_order_ledger,__cpp_execute_ledger,' +
        node_exports + '};\n')
    graph_observers = node_bridge
    groups = {}
    record['corpora'] = {}
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    for family in FAMILIES:
        path = parity / (family + '_5_0_6_cases.json.gz')
        data = load(path)
        assert data['baseline'] == baseline
        source = ROOT / data.get('driver', 'tests/parity/rust_field_layout/src/bin/entity_graph_plan_5_0_6.rs')
        groups.setdefault(source, []).append((family, data['cases']))
        record['corpora'][family] = dict(path=str(path.relative_to(ROOT)), sha256=digest(path),
            cases=len(data['cases']), driver=str(source.relative_to(ROOT)), driver_sha256=digest(source))
    for index, (source, members) in enumerate(groups.items()):
        graph_observers += '\n#[cfg(test)] mod cpp_sg_driver_' + str(index) + ' {\n' + driver(source)
        for family, cases in members:
            graph_observers += corpus_test(work, family, cases)
        graph_observers += '\n}\n'

    # These observers deliberately add request-owned intent to the historical
    # private legacy entries. Keep their independently retained complete outputs.
    legacy_source = parity / 'rust_private_save_5_0_6/src/bin/graph_execution_5_0_6.rs'
    legacy = driver(legacy_source)
    names = node_exports.split(',')
    def remove_node_imports(match):
        return re.sub(r'\b(' + '|'.join(names) + r')\s*,?', '', match.group())
    legacy = re.sub(r'use crate::\{[^}]+\};', remove_node_imports, legacy, flags=re.S)
    old = '"entity":n.entity_type,"comment":n.comment'
    assert legacy.count(old) == 1
    legacy = legacy.replace(old, '"entity":n.entity_type,"entity_id":n.entity_id,"comment":n.comment')
    factory = 'EntityDataService::for_executor(context,service,executor)'
    bound = factory + '.with_mutation_intent(teaql_core::MutationIntent::new("verify legacy graph trace").unwrap())'
    graph_observers += '\n#[cfg(test)] mod cpp_sg_legacy {\n' + legacy
    graph_observers += '\nuse super::EntityDataService;\n' + node_bridge.replace(factory, bound)
    record['additional_observations'] = {}
    for name in ('legacy_graph_intent', 'legacy_graph_intent_extended'):
        path = ROOT / 'docs/parity' / (name + '_original_5_0_6.json.gz')
        data = load(path)
        assert data['complete'] and len(data['inputs']) == len(data['observations'])
        graph_observers += corpus_test(work, name,
            [dict(input=i, expected=o) for i, o in zip(data['inputs'], data['observations'])])
        record['additional_observations'][name] = dict(sha256=digest(path), cases=len(data['inputs']))
    graph_observers += '\n}\n'

    planner = driver(parity / 'rust_field_layout/src/bin/entity_graph_plan_5_0_6.rs')
    substitutions = {
        'exists:bool,failure:String}': 'exists:bool,failure:String,rows:Option<Vec<CompactRow>>}',
        'vec![CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(999))]))]':
            'self.rows.clone().unwrap_or_else(|| vec![CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(999))]))])',
        'failure:j["fail_entity"].as_str().unwrap().into()}':
            'failure:j["fail_entity"].as_str().unwrap().into(),rows:j["rows"].as_array().map(|rows|rows.iter().map(|row|CompactRow::from_map(record(row))).collect())}',
    }
    for old, new in substitutions.items():
        assert planner.count(old) == 1
        planner = planner.replace(old, new)
    graph_observers += '\n#[cfg(test)] mod cpp_sg_planner {\n' + planner
    for name in ('graph_planner_scopes', 'graph_planner_upsert_scopes', 'graph_planner_complete_rows'):
        path = ROOT / 'docs/parity' / (name + '_original_5_0_6.json.gz')
        data = load(path)
        assert data['complete'] and len(data['inputs']) == len(data['observations'])
        graph_observers += corpus_test(work, name,
            [dict(input=i, expected=o) for i, o in zip(data['inputs'], data['observations'])])
        record['additional_observations'][name] = dict(sha256=digest(path), cases=len(data['inputs']))
    graph_observers += '\n}\n'
    graph_observers += literal(ROOT / 'scripts/observe_graph_outcome_models_5_0_6.py', 'OBSERVER')
    observers = {}
    for name, code in [('src/data_service/graph.rs', graph_observers),
                       ('src/data_service/mod.rs', service_bridge), ('src/lib.rs', exports)]:
        path = work / (name.replace('/', '_') + '.observer')
        path.write_text(code)
        observers[name] = path
    view = ReferenceRuntimeView(ROOT, work / 'runtime', observers, run)
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
        work / 'runtime/Cargo.toml', '-j', '2', '--lib', 'data_service::graph::',
        '--', '--nocapture', '--test-threads=1'])
    passed = re.findall(r'^test (data_service::graph::\S+) \.\.\. ok$', output, re.M)
    assert ('data_service::graph::ledger_trace_resolution_tests::'
            'complete_ledger_chain_replaces_fallback_and_empty_chain_inherits_it') in passed
    # The outcome observer prints inside its running test line under nocapture.
    assert 'test result: ok. 15 passed; 0 failed;' in output
    for family in FAMILIES:
        assert re.search(r'::' + family + r' \.\.\. ok', output), family
    marker = 'GRAPH_OUTCOME_MODEL:'
    models = [json.loads(line.split(marker, 1)[1]) for line in output.splitlines() if marker in line]
    expected_models = load(ROOT / 'docs/parity/graph_outcome_models_original_5_0_6.json.gz')['observations']
    assert models == expected_models and len(models) == 15
    record.update(original_tests=1, original_unit_name='complete_ledger_chain_replaces_fallback_and_empty_chain_inherits_it',
        observer_tests=14, passed_test_lines=passed, outcome_models=models,
        original_observations=sum(row['cases'] for row in record['corpora'].values()) +
            sum(row['cases'] for row in record['additional_observations'].values()) + len(models),
        provenance=view.provenance())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=('original', 'native', 'modules_native'), required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve previous evidence'
    inputs = capture_inputs()
    ref = references()
    record = dict(complete=False, profile=args.profile, source_promoted=False, steps=[], inputs=inputs,
                  references=ref, controller=Path(__file__).read_text(),
                  free_bytes=os.statvfs(ROOT).f_bavail * os.statvfs(ROOT).f_frsize)
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off', CARGO_INCREMENTAL='0',
        CARGO_PROFILE_DEV_DEBUG='0', CARGO_PROFILE_TEST_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        command = list(map(str, command))
        print('Running:', ' '.join(command), flush=True)
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
            stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='teaql_service_graph_closure_') as temporary:
            work = Path(temporary)
            run(['python3', 'scripts/verify_upstream.py'])
            if args.profile == 'original':
                env['CARGO_TARGET_DIR'] = str(work / 'target')
                original(work, run, record)
            else:
                module = args.profile == 'modules_native'
                directory = 'build_modules_native' if module else 'build_native_current'
                tests = ['module_' + name if module else name for name in TESTS]
                native_pattern = '^(' + '|'.join(map(re.escape, TESTS)) + ')$'
                run(['cmake', '--preset', args.profile])
                native = json.loads(run(['ctest', '--test-dir', 'build_native_current', '--show-only=json-v1', '-R', native_pattern]))
                assert sorted(t['name'] for t in native['tests']) == sorted(TESTS)
                binaries = {Path(arg).name for test in native['tests'] for arg in test['command']
                    if arg.startswith(str(ROOT / 'build_native_current') + '/') and Path(arg).name.startswith('teaql_')}
                targets = sorted(('teaql_module_original_runtime_entity_save' if name == 'teaql_original_entity_save_tests'
                    else name.replace('teaql_', 'teaql_module_', 1)) if module else name for name in binaries)
                record.update(tests=tests, targets=targets)
                run(['cmake', '--build', '--preset', args.profile, '--target', *targets, '-j', '2'])
                pattern = '^(' + '|'.join(map(re.escape, tests)) + ')$'
                selection = json.loads(run(['ctest', '--test-dir', directory, '--show-only=json-v1', '-R', pattern]))
                assert sorted(t['name'] for t in selection['tests']) == sorted(tests)
                paths = {str(ROOT / directory / ('modules' if module else '') / name) for name in targets}
                assert all(sum(arg in paths for arg in test.get('command', [])) == 1 for test in selection['tests'])
                junit = work / 'tests.xml'
                run(['ctest', '--test-dir', directory, '-R', pattern, '--output-on-failure', '--output-junit', junit])
                record['junit'] = junit.read_text()
                cases = list(ET.fromstring(record['junit']).iter('testcase'))
                assert sorted(c.attrib['name'] for c in cases) == sorted(tests)
                assert all(c.find('failure') is None and c.find('error') is None and c.find('skipped') is None for c in cases)
                record['tests_passed'] = len(cases)
                assert 'ninja: no work to do.' in run(['cmake', '--build', '--preset', args.profile, '--target', *targets, '-j', '2'])
                if module:
                    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', directory])
            run(['python3', 'scripts/verify_generator_parity.py'])
            run(['python3', 'scripts/verify_module_paths.py'])
            assert capture_inputs() == inputs and references() == ref
            record['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        record.update(complete=True, disposable_outputs_removed=True)
    except BaseException as error:
        record['error'] = repr(error)
        raise
    finally:
        save()
    print(args.profile, 'complete service-graph closure passed', flush=True)


if __name__ == '__main__':
    main()
