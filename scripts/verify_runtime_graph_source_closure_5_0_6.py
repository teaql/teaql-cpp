#!/usr/bin/env python3
"""Replay complete locked graph models and their current canonical consumers."""
import argparse
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
from verify_entity_save_source_closure_5_0_6 import digest

ROOT = Path(__file__).resolve().parents[1]
CORPORA = (
    ('runtime_graph', 'runtime_graph_5_0_6_cases.json.gz'),
    ('runtime_graph_model', 'runtime_graph_model_5_0_6_cases.json.gz'),
    ('dynamic_graph_debug', 'dynamic_graph_debug_5_0_6_cases.json'),
)
TESTS = [
    'runtime_graph_rust_parity', 'runtime_graph_model_rust_parity',
    'dynamic_graph_debug_rust_parity', 'original_runtime_policy_graph_contracts',
    'runtime_graph_contracts', 'runtime_graph_model_contracts',
    'graph_plan_scopes_rust_parity', 'graph_planner_scopes_rust_parity',
    'graph_planner_upsert_scopes_rust_parity', 'graph_planner_complete_rows_rust_parity',
    'legacy_graph_intent_rust_parity', 'legacy_graph_intent_extended_rust_parity',
    'recursive_graph_write_rust_parity', 'graph_node_command_rust_parity',
]


def original(work, run, record):
    parity = ROOT / 'tests/parity'
    view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/graph.rs':
        parity / 'runtime_graph_components_source_5_0_6.rs'}, run)
    output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
        work / 'runtime/Cargo.toml', '-j', '2', '--lib', 'graph::', '--', '--test-threads=1'])
    passed = re.findall(r'^test (\S+) \.\.\. ok$', output, re.M)
    expected = [
        'graph::tests::test_hierarchical_trace_chain_recovery',
        'graph::tests::test_graph_mutation_plan_batching_keys_and_counts',
        'graph::cpp_dynamic_graph_components::recursive_dynamic_components',
        'data_service::graph::ledger_trace_resolution_tests::'
        'complete_ledger_chain_replaces_fallback_and_empty_chain_inherits_it',
    ]
    assert sorted(passed) == sorted(expected)
    record.update(original_model_tests=2, supplemental_component_tests=1,
                  additional_service_test=1, passed_tests=passed, corpora={})
    drivers = work / 'runtime/src/bin'
    drivers.mkdir(exist_ok=True)
    payloads = {}
    for family, name in CORPORA:
        corpus = parity / name
        data = json.loads(gzip.decompress(corpus.read_bytes()) if corpus.suffix == '.gz'
                          else corpus.read_bytes())
        assert data['baseline'] == references()['teaql-rs']
        source = ROOT / data.get('driver',
            'tests/parity/rust_field_layout/src/bin/dynamic_graph_debug_5_0_6.rs')
        code = source.read_text()
        if 'driver_sha256' in data:
            assert digest(source) == data['driver_sha256']
        code, count = re.subn(r'#\[path\s*=\s*"[^"]*fixture_value.rs"\]',
            '#[path = ' + json.dumps(str(parity / 'rust_values/src/fixture_value.rs')) + ']', code)
        assert count == (1 if family == 'runtime_graph_model' else 0)
        destination = drivers / (family + '.rs')
        destination.write_text(code)
        payloads[family] = data['cases']
        record['corpora'][family] = dict(path=str(corpus.relative_to(ROOT)),
            sha256=digest(corpus), cases=len(data['cases']),
            driver=str(source.relative_to(ROOT)), driver_sha256=digest(source),
            prepared_driver=code, prepared_driver_sha256=digest(destination))
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
         work / 'runtime/Cargo.toml', '-j', '2', '--bins'])
    for family, cases in payloads.items():
        output = run([work / 'target/debug' / family],
                     input_text=''.join(json.dumps(case['input']) + '\n' for case in cases))
        actual = [json.loads(line) for line in output.splitlines()]
        assert len(actual) == len(cases)
        assert actual == [case['expected'] for case in cases], family
        record['corpora'][family]['matched'] = len(actual)
    record.update(original_observations=sum(len(cases) for cases in payloads.values()),
                  provenance=view.provenance())


def candidate(profile, work, run, record):
    module = profile == 'modules_native'
    directory = 'build_modules_native' if module else 'build_native_current'
    tests = ['module_' + name if module else name for name in TESTS]
    pattern = '^(' + '|'.join(map(re.escape, tests)) + ')$'
    run(['cmake', '--preset', profile])
    registry = json.loads(run(['ctest', '--test-dir', directory,
                              '--show-only=json-v1', '-R', pattern]))
    assert sorted(row['name'] for row in registry['tests']) == sorted(tests)
    binaries = []
    for row in registry['tests']:
        paths = [arg for arg in row['command'] if arg.startswith(str(ROOT / directory) + '/')
                 and Path(arg).name.startswith('teaql_')]
        assert len(paths) == 1, row
        binaries.extend(paths)
    targets = sorted({Path(path).name for path in binaries})
    record.update(tests=tests, targets=targets, registry=registry)
    command = ['cmake', '--build', '--preset', profile, '--target', *targets, '-j', '2']
    run(command)
    assert 'ninja: no work to do.' in run(command)
    junit = work / 'selected.xml'
    run(['ctest', '--test-dir', directory, '--output-on-failure', '-j', '2',
         '--output-junit', junit, '-R', pattern])
    text = junit.read_text()
    xml = ET.fromstring(text)
    assert sorted(row.attrib['name'] for row in xml.findall('.//testcase')) == sorted(tests)
    assert not any(xml.findall('.//' + tag) for tag in ('failure', 'error', 'skipped'))
    record.update(junit=text, test_cases=len(tests))
    assert 'ninja: no work to do.' in run(command)
    if module:
        run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', directory])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=('original', 'native', 'modules_native'), required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and args.record.suffix == '.gz', 'Use a fresh receipt'
    before, ref = capture_inputs(), references()
    record = dict(complete=False, source_promoted=False, profile=args.profile, steps=[],
                  inputs=before, references=ref, controller=Path(__file__).read_text(),
                  free_bytes=os.statvfs(ROOT).f_bavail * os.statvfs(ROOT).f_frsize)
    env = dict(os.environ, TEAQL_LOG_ENDPOINT='off', CARGO_INCREMENTAL='0',
        CARGO_PROFILE_DEV_DEBUG='0', CARGO_PROFILE_TEST_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, input_text=None):
        command = list(map(str, command))
        print('Running:', ' '.join(command), flush=True)
        result = subprocess.run(command, cwd=ROOT, env=env, input=input_text,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                   stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    work = None
    try:
        run(['python3', 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_runtime_graph_closure_506_') as temporary:
            work = Path(temporary)
            if args.profile == 'original':
                env['CARGO_TARGET_DIR'] = str(work / 'target')
                original(work, run, record)
            else:
                candidate(args.profile, work, run, record)
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in work.rglob('*')
                                                if p.is_file())
        assert before == capture_inputs() and ref == references(), 'Execution inputs changed'
        record['complete'] = True
    except BaseException as error:
        record['error'] = repr(error)
        raise
    finally:
        record['owned_directory_removed'] = work is not None and not work.exists()
        record['owned_directory'] = str(work)
        save()
    print(args.profile, 'graph-model source closure passed', flush=True)


if __name__ == '__main__':
    raise SystemExit(main())
