#!/usr/bin/env python3
"""Check query-key width selection through the real two-stage Python CLI."""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

RUNNER = r'''
from pathlib import Path
import gzip,json,runpy,sys
root=Path(sys.argv[1]);output=Path(sys.argv[2]);argv=sys.argv[3:]
sys.path.insert(0,str(root/'tests/parity'))
import compare_values as common
original_main=common.main
trace={'runtime_execution':False,'argv':argv,'observer_calls':[]}
def selected(corpus_name,label):
    path=root/'tests/parity'/corpus_name
    corpus=json.loads(gzip.decompress(path.read_bytes()) if path.suffix=='.gz' else path.read_bytes())
    trace.update(corpus=corpus_name,label=label,forwarded_argv=sys.argv[1:],baseline=corpus['baseline'],count=len(corpus['cases']))
    def observer(executable,inputs,emulator=(),arguments=()):
        assert common.same(inputs,[r['input'] for r in corpus['cases']])
        trace['observer_calls'].append({'executable':executable,'inputs':len(inputs),'emulator':emulator,'arguments':arguments})
        return [r['expected'] for r in corpus['cases']]
    common.observe=observer
    return original_main(corpus_name,label)
common.main=selected
sys.argv=[str(root/'tests/parity/compare_query_keys.py'),*argv]
code=0
try:
    runpy.run_path(sys.argv[0],run_name='__main__')
except SystemExit as error:
    code=0 if error.code is None else error.code
    trace['exception']={'type':'SystemExit','message':str(error.code)}
except Exception as error:
    code=1
    trace['exception']={'type':type(error).__name__,'message':str(error)}
trace['exit_code']=code
output.write_text(json.dumps(trace))
raise SystemExit(code)
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    if args.record is not None:
        assert not args.record.exists()
    native = 'query_keys_5_0_6_cases.json.gz'
    target = 'query_keys_32_5_0_6_cases.json.gz'
    cpp = ['--cpp', 'controlled_cpp_observer']
    modes = [
        ('default_64', cpp, 0, native, [], cpp),
        ('explicit_64', cpp + ['--pointer-bytes', '8'], 0, native, [], cpp),
        ('explicit_32', cpp + ['--pointer-bytes', '4'], 0, target, [], cpp),
        ('equals_32', cpp + ['--pointer-bytes=4'], 0, target, [], cpp),
        ('emulator_64', ['--pointer-bytes=8'] + cpp + ['--cpp-emulator=qemu-test;-cpu;test_cpu'],
         0, native, ['qemu-test', '-cpu', 'test_cpu'], cpp + ['--cpp-emulator=qemu-test;-cpu;test_cpu']),
        ('emulator_32', ['--pointer-bytes=4'] + cpp + ['--cpp-emulator=qemu-test;-cpu;test_cpu'],
         0, target, ['qemu-test', '-cpu', 'test_cpu'], cpp + ['--cpp-emulator=qemu-test;-cpu;test_cpu']),
        ('invalid_width', cpp + ['--pointer-bytes=16'], 2, None, [], None),
        ('missing_width', cpp + ['--pointer-bytes'], 2, None, [], None),
        ('missing_cpp', ['--pointer-bytes=4'], 2, None, [], []),
        ('unknown_option', cpp + ['--unexpected-test-option'], 2, None, [], None),
    ]
    lock = json.loads((root / 'upstream.lock.json').read_text())
    record = dict(complete=False, passed=False, runtime_execution=False, whole_goal_complete=False,
                  scope='Controlled CLI selection/admission and emulator forwarding; no C++ or Rust execution.',
                  results=[])
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_query_key_width_cli_506_') as name:
            owned = Path(name)
            record['owned_directory'] = name
            runner = owned / 'runner.py'
            runner.write_text(RUNNER)
            environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
            for label, argv, wanted, corpus_name, emulator, forwarded in modes:
                trace_path = owned / (label + '.json')
                command = [sys.executable, str(runner), str(root), str(trace_path), *argv]
                result = subprocess.run(command, cwd=root, env=environment, text=True, capture_output=True)
                trace = json.loads(trace_path.read_text())
                errors = []
                if result.returncode != wanted or trace['exit_code'] != wanted:
                    errors.append('unexpected exit code')
                if wanted == 0:
                    if trace.get('corpus') != corpus_name or trace.get('baseline') != lock['commit']:
                        errors.append('wrong selected corpus or baseline')
                    expected_label = f"canonical and pagination query keys ({32 if corpus_name == target else 64}-bit usize)"
                    if trace.get('label') != expected_label or trace.get('count') != 451:
                        errors.append('wrong width label or cardinality')
                    calls = trace['observer_calls']
                    expected_call = dict(executable='controlled_cpp_observer', inputs=451,
                                         emulator=emulator, arguments=[])
                    if calls != [expected_call]:
                        errors.append('wrong observer command or cardinality')
                    if trace.get('forwarded_argv') != forwarded:
                        errors.append('width arguments leaked into the shared parser')
                elif trace['observer_calls']:
                    errors.append('invalid arguments reached the observer')
                record['results'].append(dict(name=label, command=command, exit_code=result.returncode,
                    expected_exit_code=wanted, passed=not errors, errors=errors,
                    stdout=result.stdout, stderr=result.stderr, trace=trace))
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['complete'] = True
        record['passed'] = all(result['passed'] for result in record['results'])
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    failed = [result['name'] for result in record['results'] if not result['passed']]
    print(f'Query-key width CLI: {len(modes) - len(failed)}/{len(modes)} controls pass')
    if failed:
        print('Unexpected outcomes:', ', '.join(failed))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
