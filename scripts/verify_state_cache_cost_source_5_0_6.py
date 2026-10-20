#!/usr/bin/env python3
"""Execute the unchanged Rust cache-cost test and both C++ cache-hit profiles."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from run_native_integration_epoch_5_0_6 import capture_inputs, references

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'teaql-runtime/tests/state_cache_cost.rs'
GRID = {(hints, 10000, sample) for hints in (1, 64, 512) for sample in range(7)}


def samples(output, columns):
    result = {}
    for line in output.splitlines():
        if not line.startswith('STATE_CACHE_HIT,'):
            continue
        values = [int(value) for value in line.split(',')[1:]]
        assert len(values) == columns
        key = tuple(values[:3])
        assert key not in result
        assert all(value == 0 for value in values[3:-1]), line
        assert values[-1] >= 0
        result[key] = values[3:]
    assert set(result) == GRID
    return [dict(hints=k[0], reads=k[1], sample=k[2], measured=result[k]) for k in sorted(result)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists() or args.record.suffix != '.gz':
        parser.error('Use a fresh .gz receipt')
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    record = dict(complete=False, source_promoted=False, scope='Complete cache-cost test; allocation outcomes only, no latency equivalence',
                  inputs=capture_inputs(), references=references(), steps=[], profiles={},
                  source_sha256={SOURCE: digest(ROOT / '.reference/teaql-rs' / SOURCE)},
                  controller_source=Path(__file__).read_text(),
                  fixture=(ROOT / 'teaql_runtime/tests/state_cache_cost_tests.cpp').read_text())
    environment = dict(os.environ, CARGO_INCREMENTAL='0', TEAQL_LOG_ENDPOINT='off',
                       CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(command[:5]), flush=True)
        result.check_returncode()
        return result.stdout

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_state_cache_cost_source_506_') as temporary:
            work = Path(temporary)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          view.directory / 'Cargo.toml', '-j2', '--test', 'state_cache_cost',
                          '--', '--show-output', '--test-threads=1'])
            assert '1 passed; 0 failed' in output
            assert 'test shared_state_cache_hit_cost_with_retained_shapes ... ok' in output
            record['original'] = dict(test='shared_state_cache_hit_cost_with_retained_shapes',
                                      samples=samples(output, 6), provenance=view.provenance())
            for profile, imported in [('native', False), ('modules_native', True)]:
                build = ROOT / ('build_modules_native' if imported else 'build_native_current')
                prefix = 'teaql_module_' if imported else 'teaql_'
                targets = [prefix + name + '_tests' for name in ['state_cache_cost', 'loaded_snapshot']]
                names = [('module_' if imported else '') + name + '_contracts'
                         for name in ['state_cache_cost', 'loaded_snapshot']]
                run(['cmake', '--preset', profile])
                command = ['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets]
                run(command)
                if imported:
                    run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', build])
                junit = work / (profile + '.xml')
                run(['ctest', '--preset', profile, '--output-on-failure', '--no-tests=error',
                     '--output-junit', junit, '-R', '^(' + '|'.join(names) + ')$'])
                tree = ET.parse(junit).getroot()
                assert sorted(x.attrib['name'] for x in tree.findall('.//testcase')) == sorted(names)
                assert not any(tree.findall('.//' + tag) for tag in ['failure', 'error', 'skipped'])
                directory = build / 'modules' if imported else build
                binaries = {name: digest(directory / name) for name in targets}
                output = run([directory / targets[0]])
                assert 'STATE_CACHE_HIT_SAMPLES=21;READS_PER_SAMPLE=10000;POSITIVE_CONTROLS=passed' in output
                actual = samples(output, 8)
                assert 'no work to do' in run(command)
                assert all(digest(directory / name) == value for name, value in binaries.items())
                record['profiles'][profile] = dict(samples=actual, binaries=binaries, tests=names,
                                                   junit=junit.read_text(), no_work=True)
                save()
            view.verify()
        assert capture_inputs() == record['inputs'] and references() == record['references']
        record.update(complete=True, unchanged_inputs=True)
        save()
        print('Original1/native2/module2; exact 21-sample allocation grids pass; timing not compared')
    except BaseException as error:
        record['controller_error'] = dict(type=type(error).__name__, message=str(error))
        save()
        raise


if __name__ == '__main__':
    main()
