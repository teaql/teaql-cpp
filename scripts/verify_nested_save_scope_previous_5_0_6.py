#!/usr/bin/env python3
"""Compile the retained previous scope against the current fixture under sanitizers."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--comparison-record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and not args.comparison_record.exists()
    retained = ROOT / 'docs/parity/nested_public_save_scope_negative_compiler_5_0_6.json.gz'
    initial = ROOT / 'docs/parity/nested_public_save_scope_before_5_0_6.json.gz'
    old = json.loads(gzip.decompress(retained.read_bytes()))
    before = json.loads(gzip.decompress(initial.read_bytes()))
    relative = 'teaql_runtime/include/teaql/runtime/detail/graph_fix_scope.hpp'
    header = old['old_header'].encode()
    assert hashlib.sha256(header).hexdigest() == old['old_header_sha256'] == before['candidate_sources'][relative]
    fixture = ROOT / 'tests/support/nested_save_scope_fixture.hpp'
    record = dict(complete=False, steps=[], controller=Path(__file__).read_text(),
                  retained_header_receipt_sha256=digest(retained), fixture_sha256=digest(fixture),
                  original_negative_sha256=digest(initial), old_header_sha256=old['old_header_sha256'])
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')

    def run(command):
        command = list(map(str, command))
        print('Running:', command[0], flush=True)
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        return result.stdout

    try:
        with tempfile.TemporaryDirectory(prefix='teaql_nested_scope_previous_') as temporary:
            work = Path(temporary)
            overlay = work / 'include/teaql/runtime/detail/graph_fix_scope.hpp'
            overlay.parent.mkdir(parents=True)
            overlay.write_bytes(header)
            source = work / 'probe.cpp'
            source.write_text('#include <teaql/runtime/detail/graph_fix_scope.hpp>\n#include "' +
                str(fixture) + '"\nint main() {\n'
                'std::pmr::set_default_resource(std::pmr::null_memory_resource());\n'
                'return nested_save_scope_fixture::verify() ? 0 : 1;\n}\n')
            run([ROOT / '.tools/clang-format/bin/clang-format', '--style=file:' +
                 str(ROOT / '.clang-format'), '-i', source])
            record['probe_source'] = source.read_text()
            entry = next(row for row in json.loads((ROOT / 'build_sanitize_current/compile_commands.json').read_text())
                         if row['file'].endswith('/teaql_runtime/tests/graph_fix_tests.cpp'))
            command = list(entry.get('arguments') or shlex.split(entry['command']))
            for flag in ('-MD', '-MT', '-MF', '-o'):
                while flag in command:
                    position = command.index(flag)
                    del command[position:position + (1 if flag == '-MD' else 2)]
            command.remove('-c')
            command.remove(entry['file'])
            command.insert(1, '-I' + str(work / 'include'))
            libraries = [ROOT / 'build_sanitize_current' / package / ('lib' + package + '.a')
                         for package in ('teaql_runtime', 'teaql_sql', 'teaql_data_service',
                                         'teaql_core', 'teaql_memory')]
            record['libraries'] = {str(p.relative_to(ROOT)): digest(p) for p in libraries}
            binary = work / 'probe'
            run([*command, source, '-Wl,--start-group', *libraries, '-Wl,--end-group',
                 '-fsanitize=address,undefined', '-Wl,-rpath,' +
                 str(ROOT / '.tools/toolchains/gcc-16.2.0/lib'), '-pthread', '-ldl', '-lm', '-o', binary])
            record['binary_sha256'] = digest(binary)
            run(['python3', 'scripts/verify_nested_save_scope_5_0_6.py', '--cpp', binary,
                 '--scope-header', overlay, '--record', args.comparison_record,
                 '--expect-differences', '3'])
            negative = json.loads(gzip.decompress(args.comparison_record.read_bytes()))
            invocation = next(step for step in negative['steps'] if step['command'] == [str(binary)])
            assert invocation['exit_code'] == 1 and not invocation['stderr'], invocation
            assert negative['candidate_sha256'] == record['binary_sha256']
            assert negative['counterfactual_scope_header']['sha256'] == old['old_header_sha256']
            assert digest(fixture) == record['fixture_sha256']
            assert all(digest(ROOT / p) == expected for p, expected in record['libraries'].items())
            record['comparison_sha256'] = digest(args.comparison_record)
            record['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        record.update(complete=True, disposable_outputs_removed=True)
    finally:
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))
    print('Previous scope: three differences with the current fixture; no sanitizer diagnostic', flush=True)


if __name__ == '__main__':
    main()
