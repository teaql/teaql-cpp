#!/usr/bin/env python3
"""Retain actual Rust borrow refusals for mutable Context reconfiguration."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', type=Path,
                        default=Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506'))
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    project = ROOT / 'tests/parity/rust_execution_shapes_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    wanted = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name not in {'teaql_execution_shapes_506', 'cpp-dynamic-json-core'}:
            assert resolved <= wanted.get(name, set()), name
    env = dict(os.environ, CARGO_TARGET_DIR=str(args.target),
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
               TEAQL_LOG_ENDPOINT='off')
    records = []
    command = ['cargo', 'build', '--offline', '--locked', '--manifest-path',
               str(project / 'Cargo.toml'), '--bin', 'context_resource_borrow']
    positive = subprocess.run(command, env=env, capture_output=True, text=True)
    records.append(dict(case='reconfigure_after_drop', command=command,
                        exit_code=positive.returncode, stdout=positive.stdout, stderr=positive.stderr))
    assert positive.returncode == 0, positive.stderr
    executable = args.target / 'debug/context_resource_borrow'
    executed = subprocess.run([str(executable)], env=env, capture_output=True, text=True)
    assert executed.returncode == 0, executed.stderr
    records.append(dict(case='run_after_successful_build', command=[str(executable)],
                        exit_code=executed.returncode, stdout=executed.stdout, stderr=executed.stderr))
    for kind in ['resource', 'registry', 'store']:
        command = ['cargo', 'rustc', '--offline', '--locked', '--message-format=json',
                   '--manifest-path', str(project / 'Cargo.toml'), '--bin',
                   'context_resource_borrow', '--', '--cfg', f'teaql_{kind}_negative']
        result = subprocess.run(command, env=env, capture_output=True, text=True)
        errors = []
        for line in result.stdout.split('\n'):
            if line.startswith('{'):
                entry = json.loads(line)
                if entry.get('reason') == 'compiler-message' and entry['message']['level'] == 'error':
                    errors.append(entry['message'])
        records.append(dict(case=kind, command=command, exit_code=result.returncode,
                            stdout=result.stdout, stderr=result.stderr, errors=errors))
        assert result.returncode != 0 and len(errors) == 1, (kind, result.stderr, errors)
        assert errors[0]['code']['code'] == 'E0502', (kind, errors)
        assert any(s['is_primary'] and s['file_name'].endswith('context_resource_borrow.rs')
                   for s in errors[0]['spans']), kind
    if args.record:
        assert not args.record.exists()
        source = project / 'src/bin/context_resource_borrow.rs'
        args.record.write_bytes(gzip.compress(json.dumps(dict(
            runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
            source=str(source.relative_to(ROOT)),
            source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
            source_text=source.read_text(), commands=records,
            boundary='Actual original generated future borrows UserContext. Resource/registry/store '
                     'setters require mutable access until that future is dropped. These compiler '
                     'refusals delimit C++ lifetime adaptations; they are not source scheduling results.'),
            ensure_ascii=False).encode(), mtime=0))
    print('Verified actual original Context borrowing: one compiled/executed positive, '
          'three independent E0502 mutable-reconfiguration refusals')


if __name__ == '__main__':
    main()
