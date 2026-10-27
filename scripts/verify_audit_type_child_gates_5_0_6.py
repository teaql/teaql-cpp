#!/usr/bin/env python3
"""Run scoped current privacy consumers and final guards after type-parser installation."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sizes():
    paths = [ROOT / 'build_native_current', ROOT / 'build_modules_native',
             ROOT.parent / 'teaql_code_gen_cpp/generator/target']
    result = {}
    for path in paths:
        result[str(path)] = int(subprocess.check_output(['du', '-sb', str(path)], text=True).split()[0])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    source_roots = [*ROOT.glob('teaql_*'), ROOT / 'examples', ROOT / 'tests']
    suffixes = {'.cpp', '.hpp', '.h', '.cppm', '.ixx', '.cc', '.cxx'}
    inputs = sorted({p for directory in source_roots if directory.is_dir()
                     for p in directory.rglob('*') if p.is_file() and p.suffix in suffixes})
    record = dict(complete=False, scope=__doc__, steps=[], profiles=[],
                  input_hashes_before={str(p): digest(p) for p in inputs},
                  free_bytes_before=shutil.disk_usage(ROOT).free, stable_output_bytes_before=sizes())

    def run(command):
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        print(' '.join(command), 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        for preset, prefix, target in [
            ('native', '', 'teaql_generated_dynamic_json_privacy_probe'),
            ('modules_native', 'module_', 'teaql_module_generated_dynamic_json_privacy_probe'),
        ]:
            run(['cmake', '--build', '--preset', preset, '--target', target, '-j', '4'])
            output = run(['ctest', '--preset', preset, '--no-tests=error', '-R',
                 '^(' + prefix + 'generated_dynamic_json_privacy_rust_parity|'
                 + prefix + 'generated_dynamic_json_privacy_sink_contracts)$'])
            assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 2\b', output)
            expected = {prefix + 'generated_dynamic_json_privacy_rust_parity',
                        prefix + 'generated_dynamic_json_privacy_sink_contracts'}
            passed = re.findall(r'Test #\d+: (\S+)\s+\.+\s+Passed\b', output)
            assert len(passed) == 2 and set(passed) == expected
            record['profiles'].append(dict(preset=preset, test_count=2))
        for command in [['python3', 'scripts/format_cpp.py'],
                        ['python3', 'scripts/format_cpp.py', '--check'],
                        ['python3', 'scripts/verify_upstream.py'],
                        ['python3', 'scripts/verify_generator_parity.py'],
                        ['python3', 'scripts/verify_module_paths.py']]:
            run(command)
        record['input_hashes_after'] = {str(p): digest(p) for p in inputs}
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['stable_output_bytes_after'] = sizes()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Scoped native2/imported2 and pinned guards pass; C++ inputs unchanged')


if __name__ == '__main__':
    main()
