#!/usr/bin/env python3
"""Replay complete policy source contracts without rewriting runtime algorithms."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


MATRICES = (
    ('policy_model_5_0_6_cases.json.gz', 'policy_model_source_506', 'policy_model_probe', 177),
    ('mutation_policy_5_0_6_cases.json.gz', 'policy_review_source_506', 'mutation_policy_probe', 276),
    ('policy_plan_5_0_6_cases.json.gz', None, 'policy_plan_probe', 2129),
    ('dynamic_governance_5_0_6_cases.json', 'teaql_dynamic_governance_reference_506',
     'dynamic_governance_probe', 24),
)


def read_corpus(path):
    return json.loads(gzip.decompress(path.read_bytes()) if path.suffix == '.gz'
                      else path.read_text())


def observer_fixture(root, inputs_path):
    driver = root / 'tests/parity/rust_private_save_5_0_6/src/bin/policy_plan_5_0_6.rs'
    original = driver.read_text()
    before = ('use teaql_runtime::{UserContext,EntityKey,EntityRuntimeState,GraphNode,'
              'GraphMutationPlan,GraphMutationKind,MutationPlan,__teaql_cpp_graph_policy_plan,'
              '__teaql_cpp_ledger_policy_plan};')
    after = ('use crate::{UserContext,EntityKey,EntityRuntimeState,GraphNode,GraphMutationPlan,'
             'GraphMutationKind,MutationPlan};\n'
             'use super::{graph_policy_plan as __teaql_cpp_graph_policy_plan, '
             'ledger_policy_plan as __teaql_cpp_ledger_policy_plan};')
    assert original.count(before) == 1
    adapted = original.replace(before, after).replace('//!', '//').replace(
        '"../../../rust_values/src/fixture_value.rs"',
        json.dumps(str(root / 'tests/parity/rust_values/src/fixture_value.rs')))
    return ('\n#[cfg(test)]\nmod cpp_policy_complete_source_observer {\n' + adapted +
            '\n#[test]\nfn replay_retained_plan_inputs() {\n'
            'let inputs:Vec<J>=serde_json::from_str(include_str!(' + json.dumps(str(inputs_path)) +
            ')).unwrap();\nprintln!();\nfor input in inputs {'
            'println!("TEAQL_POLICY_SOURCE_JSON {}",evaluate(input));}\n}\n}\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--project', type=Path)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native'),
                        default=['native', 'modules_native'])
    parser.add_argument('--staged-consumers-dir', type=Path,
                        help='Explicit provisional module programs; does not qualify registration')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh output path; preserve existing evidence')
    root = args.root.resolve()
    project = (args.project or root / 'tests/parity/rust_policy_source_5_0_6').resolve()
    sys.path.insert(0, str(root / 'scripts'))
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    from verify_constant_closure_5_0_6 import registry_identities
    record = dict(complete=False, source_promoted=False, steps=[], profiles={}, matrices=[],
                  staged=bool(args.staged_consumers_dir), inputs=capture_inputs(),
                  references=references(), controller_source=Path(__file__).read_text())
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, **kwargs):
        kwargs.setdefault('env', environment)
        result = subprocess.run([str(x) for x in command], cwd=root, text=True,
                                capture_output=True, **kwargs)
        record['steps'].append(dict(command=[str(x) for x in command],
                                    exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr))
        save()
        if result.returncode:
            raise RuntimeError(f'Command failed; output retained: {command[0]}')
        return result.stdout

    try:
        contract = json.loads((project / 'contract.json').read_text())
        assert contract['baseline'] == json.loads((root / 'upstream.lock.json').read_text())['commit']
        for name, expected in contract['guards'].items():
            assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
        record['observer_workspace'] = {
            path.name: dict(sha256=hashlib.sha256(path.read_bytes()).hexdigest(), text=path.read_text())
            for path in project.iterdir() if path.is_file()
        }
        reference_ids = registry_identities(root / '.reference/teaql-rs/Cargo.lock')
        assert registry_identities(project / 'Cargo.lock') <= reference_ids
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
             project / 'Cargo.toml', '--bins', '-j2'])
        dynamic = root / 'tests/parity/rust_dynamic_protocols_reference_5_0_6'
        assert registry_identities(dynamic / 'Cargo.lock') <= reference_ids
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
             dynamic / 'Cargo.toml', '--bin', 'teaql_dynamic_governance_reference_506', '-j2'])
        plan_corpus = read_corpus(root / 'tests/parity/policy_plan_5_0_6_cases.json.gz')
        with tempfile.TemporaryDirectory(prefix='teaql_policy_source_506_') as directory:
            work = Path(directory)
            inputs_path = work / 'plan_inputs.json'
            inputs = [case['input'] for case in plan_corpus['cases']]
            inputs_path.write_text(json.dumps(inputs) + '\n')
            observer = work / 'observer.rs'
            observer.write_text(observer_fixture(root, inputs_path))
            view = ReferenceRuntimeView(root, work / 'runtime',
                                        {'src/mutation_policy.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '--lib', 'mutation_policy::', '-j2',
                          '--', '--nocapture', '--test-threads=1'])
            assert '7 passed; 0 failed' in output
            plans = [json.loads(line.split('TEAQL_POLICY_SOURCE_JSON ', 1)[1])
                     for line in output.splitlines() if line.startswith('TEAQL_POLICY_SOURCE_JSON ')]
            assert len(plans) == 2129 and plans == [c['expected'] for c in plan_corpus['cases']]
            record.update(original_embedded_tests=6, new_private_caller_test=1,
                          private_original_plans=plans, private_view=view.provenance(),
                          private_fixture_inputs=inputs)
            save()
        for filename, oracle, probe, count in MATRICES:
            corpus = read_corpus(root / 'tests/parity' / filename)
            assert corpus['baseline'] == contract['baseline'] and len(corpus['cases']) == count
            payload = ''.join(json.dumps(c['input']) + '\n' for c in corpus['cases'])
            original = (plans if oracle is None else [json.loads(line) for line in run(
                [Path(environment['CARGO_TARGET_DIR']) / 'debug' / oracle], input=payload).splitlines()])
            assert original == [c['expected'] for c in corpus['cases']], filename
            record['matrices'].append(dict(name=filename, count=count, original=original, profiles={}))
        units = ['mutation_policy_context_contracts', 'mutation_policy_reentrant_close',
                 'context_event_contracts', 'dynamic_governance_rust_parity',
                 'policy_model_contracts', 'runtime_generic_completion_contracts',
                 'policy_model_rust_parity', 'mutation_policy_rust_parity', 'policy_plan_rust_parity']
        missing_stems = {'policy_model_probe', 'mutation_policy_probe', 'policy_plan_probe',
                         'policy_model_tests', 'runtime_completion_tests'}
        for profile in dict.fromkeys(args.profiles):
            native = profile == 'native'
            staged = bool(args.staged_consumers_dir) and not native
            prefix = 'teaql_' if native else 'teaql_module_'
            build = root / ('build_native_current' if native else 'build_modules_native/modules')
            stems = ['mutation_policy_tests', 'context_event_tests', 'policy_model_tests',
                     'runtime_completion_tests'] + [m[2] for m in MATRICES]
            targets = [prefix + stem for stem in stems if not (staged and stem in missing_stems)]
            run(['cmake', '--build', '--preset', profile, '--target', *targets])
            chosen = units[:4] if staged else units
            selected = [name if native else 'module_' + name for name in chosen]
            junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
            run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                 '--output-junit', junit, '-R', '^(' + '|'.join(selected) + ')$'])
            tree = ET.parse(junit).getroot()
            assert sorted(c.attrib['name'] for c in tree.findall('.//testcase')) == sorted(selected)
            assert not any(tree.findall('.//' + tag) for tag in ('failure', 'error', 'skipped'))
            record['profiles'][profile] = dict(checks=len(selected), junit=junit.read_text())
            if staged:
                for stem in ('policy_model_tests', 'runtime_completion_tests'):
                    run([args.staged_consumers_dir / stem])
            for entry, (filename, _, probe, _) in zip(record['matrices'], MATRICES):
                binary = (args.staged_consumers_dir / probe if staged and probe in missing_stems
                          else build / (prefix + probe))
                corpus = read_corpus(root / 'tests/parity' / filename)
                payload = ''.join(json.dumps(c['input']) + '\n' for c in corpus['cases'])
                actual = [json.loads(line) for line in run([binary], input=payload).splitlines()]
                assert actual == entry['original'], profile + '/' + filename
                entry['profiles'][profile] = actual
            assert 'no work to do' in run(['cmake', '--build', '--preset', profile, '--target', *targets])
        if 'modules_native' in args.profiles and not args.staged_consumers_dir:
            run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
            for stem in sorted(missing_stems):
                link = run([root / '.tools/toolchains/ninja-1.13.2/ninja', '-C',
                            root / 'build_modules_native', '-t', 'commands',
                            'teaql_module_' + stem]).splitlines()[-1]
                assert 'sqlite' not in link.lower() and 'provider' not in link.lower()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        for name, info in record['observer_workspace'].items():
            assert hashlib.sha256((project / name).read_bytes()).hexdigest() == info['sha256']
        record.update(complete=True, unchanged_inputs=True, observations_per_frontend=2606,
                      all_installed_frontends=(set(args.profiles) == {'native', 'modules_native'}
                                               and not args.staged_consumers_dir))
        save()
        print('Complete requested policy source replay: 2606 observations per selected frontend')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
