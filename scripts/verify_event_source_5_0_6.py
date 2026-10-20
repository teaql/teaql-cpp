#!/usr/bin/env python3
"""Replay complete audit-event source contracts with exact reference dependencies."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET


MATRICES = (
    ('event_model_5_0_6_cases', 'event_model_source_506', 'event_model_probe', 112),
    ('audit_event_projection_5_0_6_cases', 'audit_projection_source_506',
     'audit_event_projection_probe', 104),
    ('dynamic_audit_projection_5_0_6_cases', 'dynamic_audit_source_506',
     'dynamic_audit_projection_probe', 3968),
    ('audit_event_baseline_5_0_6_cases', 'audit_event_source_506', 'audit_event_probe', 1427),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--project', type=Path)
    parser.add_argument('--profiles', nargs='+', choices=('native', 'modules_native'),
                        default=['native', 'modules_native'])
    parser.add_argument('--staged-module-model-probe', type=Path,
                        help='Explicit provisional consumer; never qualifies permanent registration')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use fresh evidence paths; preserve previous receipts')
    root = args.root.resolve()
    project = (args.project or root / 'tests/parity/rust_event_source_5_0_6').resolve()
    sys.path.insert(0, str(root / 'scripts'))
    from run_native_integration_epoch_5_0_6 import capture_inputs, references
    from verify_constant_closure_5_0_6 import registry_identities
    record = dict(complete=False, source_promoted=False,
                  staged=bool(args.staged_module_model_probe), steps=[], profiles={}, matrices=[],
                  inputs=capture_inputs(), references=references(),
                  controller_source=Path(__file__).read_text())

    def save():
        args.record.write_bytes(gzip.compress(
            (json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, **kwargs):
        result = subprocess.run([str(x) for x in command], cwd=root, text=True,
                                capture_output=True, **kwargs)
        record['steps'].append(dict(command=[str(x) for x in command],
                                    exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr))
        save()
        if result.returncode:
            raise RuntimeError(f'Command failed; complete output retained: {command[0]}')
        return result.stdout

    try:
        contract = json.loads((project / 'contract.json').read_text())
        assert contract['baseline'] == json.loads((root / 'upstream.lock.json').read_text())['commit']
        for name, expected in contract['guards'].items():
            assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
        identities = registry_identities(project / 'Cargo.lock')
        assert identities <= registry_identities(root / '.reference/teaql-rs/Cargo.lock')
        record['registry_identities'] = sorted(identities)
        record['observer_workspace'] = {
            path.name: dict(sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                            text=path.read_text())
            for path in project.iterdir() if path.is_file()
        }
        environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                           CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                           TEAQL_LOG_ENDPOINT='off')
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
             project / 'Cargo.toml', '--bins', '-j2'], env=environment)
        tests = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                     project / 'Cargo.toml', '--bin', 'event_sink_source_506', '-j2',
                     '--', '--nocapture'], env=environment)
        assert '5 passed; 0 failed' in tests
        record['original_public_sink_caller_tests'] = 5
        for name, oracle, probe, count in MATRICES:
            data = json.loads(gzip.decompress((root / 'tests/parity' / (name + '.json.gz')).read_bytes()))
            assert data['baseline'] == contract['baseline'] and len(data['cases']) == count
            payload = ''.join(json.dumps(case['input']) + '\n' for case in data['cases'])
            values = [json.loads(line) for line in run(
                [Path(environment['CARGO_TARGET_DIR']) / 'debug' / oracle],
                input=payload, env=environment).splitlines()]
            assert len(values) == count and values == [c['expected'] for c in data['cases']], name
            record['matrices'].append(dict(name=name, count=count, original=values, profiles={}))
        units = ['event_model_contracts', 'audit_event_contracts',
                 'audit_event_context_reentrant_close', 'audit_event_owner_reentrant_close',
                 'audit_event_payload_reentrant_close', 'audit_event_result_reentrant_close',
                 'event_model_rust_parity', 'audit_event_rust_parity',
                 'audit_event_projection_5_0_6_rust_parity', 'dynamic_audit_projection_rust_parity']
        for profile in dict.fromkeys(args.profiles):
            native = profile == 'native'
            prefix = 'teaql_' if native else 'teaql_module_'
            build = root / ('build_native_current' if native else 'build_modules_native/modules')
            staged = bool(args.staged_module_model_probe) and not native
            stems = ['event_model_tests', 'audit_event_tests'] + [m[2] for m in MATRICES]
            if staged:
                stems.remove('event_model_probe')
            targets = [prefix + stem for stem in stems]
            run(['cmake', '--build', '--preset', profile, '--target', *targets])
            selected = [name if native else 'module_' + name for name in units
                        if not (staged and name == 'event_model_rust_parity')]
            junit = args.record.with_name(args.record.name + '.' + profile + '.xml')
            run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                 '--output-junit', junit, '-R', '^(' + '|'.join(selected) + ')$'])
            tree = ET.parse(junit).getroot()
            cases = tree.findall('.//testcase')
            assert sorted(c.attrib['name'] for c in cases) == sorted(selected)
            assert not any(tree.findall('.//' + tag) for tag in ('failure', 'error', 'skipped'))
            record['profiles'][profile] = dict(checks=len(selected), junit=junit.read_text())
            for entry, (name, _, probe, _) in zip(record['matrices'], MATRICES):
                binary = (args.staged_module_model_probe if staged and probe == 'event_model_probe'
                          else build / (prefix + probe))
                data = json.loads(gzip.decompress((root / 'tests/parity' / (name + '.json.gz')).read_bytes()))
                payload = ''.join(json.dumps(c['input']) + '\n' for c in data['cases'])
                values = [json.loads(line) for line in run(
                    [binary], input=payload, env=environment).splitlines()]
                assert values == entry['original'], profile + '/' + name
                entry['profiles'][profile] = values
            assert 'no work to do' in run(
                ['cmake', '--build', '--preset', profile, '--target', *targets])
        if not args.staged_module_model_probe and 'modules_native' in args.profiles:
            run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', 'build_modules_native'])
            link = run([root / '.tools/toolchains/ninja-1.13.2/ninja', '-C',
                        root / 'build_modules_native', '-t', 'commands',
                        'teaql_module_event_model_probe']).splitlines()[-1]
            assert 'sqlite' not in link.lower() and 'provider' not in link.lower()
            record['runtime_only_model_link'] = link
        assert capture_inputs() == record['inputs'] and references() == record['references']
        for name, info in record['observer_workspace'].items():
            assert hashlib.sha256((project / name).read_bytes()).hexdigest() == info['sha256']
        record.update(complete=True, unchanged_inputs=True, observations_per_frontend=5611,
                      all_installed_frontends=(set(args.profiles) == {'native', 'modules_native'}
                                               and not args.staged_module_model_probe))
        save()
        print('Complete requested event source replay: 5611 observations per selected frontend')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
