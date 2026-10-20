#!/usr/bin/env python3
"""Check real generated C++ headers and quoted Rust usize constants on MIPS32.

Native descriptor execution comes from the supplied complete generated-pair
receipt. Rust's no_core frontend fixture only checks the actual quoted integer's
target width; it does not supply a Rust MIPS standard library or runtime test.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import xml.etree.ElementTree as ET

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'tests/parity/rust_audit_width_reference_5_0_6'
CASES = ('zero', 'max_u32', 'above_u32', 'max_u64', 'overflow')
PRELUDE = '''#![feature(no_core, lang_items)]
#![no_core]
#[lang="pointee_sized"] pub trait PointeeSized {}
#[lang="meta_sized"] pub trait MetaSized: PointeeSized {}
#[lang="sized"] pub trait Sized: MetaSized {}
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiled-pairs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a new evidence path')
    data = {'complete': False, 'steps': [], 'cases': [],
            'scope': 'Native64/MIPS32 compiler width contracts, actual complete C++ SDK headers, '
                     'actual original host parsing/quote and no_core Rust target constants. '
                     'Not Rust MIPS std/runtime, linking, QEMU or device execution.',
            'full_AP02_qualified': False, 'full_goal_accepted': False}

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(data, indent=2) + '\n').encode(), mtime=0))

    def run(command, env=None, input_text=None, required=True):
        command = [str(item) for item in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=input_text,
                                text=True, capture_output=True)
        data['steps'].append({'command': command, 'exit_code': result.returncode,
                              'stdout': result.stdout, 'stderr': result.stderr})
        save()
        if required:
            result.check_returncode()
        return result

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    pairs = json.loads(gzip.decompress(args.compiled_pairs.read_bytes()))
    if pairs['runtime_commit'] != baseline:
        raise RuntimeError('Generated pair receipt baseline differs')
    selected = {case['case']: case for case in pairs['cases'] if case['case'] in CASES}
    if set(selected) != set(CASES) or not all(row['matching'] for row in selected.values()):
        raise RuntimeError('Complete original/native generated cases must match first')
    if not registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock'):
        raise RuntimeError('Original width observer dependency identity differs')
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml'], env=environment)
    inputs = {name: ET.fromstring(selected[name]['model']).find('item').get('_audit_value_max_len')
              for name in CASES}
    result = run([Path(environment['CARGO_TARGET_DIR']) / 'debug/teaql_audit_width_reference_506'],
                 input_text=''.join(json.dumps(inputs[name]) + '\n' for name in CASES))
    values = [json.loads(line) for line in result.stdout.split('\n') if line]
    if len(values) != len(CASES):
        raise RuntimeError('Incomplete host parser/quoting output')
    compilers = {}
    for target, directory in [('native64', 'build_native_current'), ('mips32', 'build_mips_current')]:
        commands = json.loads((ROOT / directory / 'compile_commands.json').read_text())
        entry = next(row for row in commands if row['file'].endswith('/teaql_provider_sqlite/src/provider.cpp'))
        command = shlex.split(entry['command'])
        command.remove('-c')
        command.remove(entry['file'])
        for flag in ('-o', '-MT', '-MF', '-MD'):
            while flag in command:
                index = command.index(flag)
                del command[index:index + (1 if flag == '-MD' else 2)]
        compilers[target] = command
    mips_environment = dict(os.environ, STAGING_DIR=str(Path(compilers['mips32'][0]).parent.parent))
    data.update(baseline=baseline, generated_pairs_sha256=digest(args.compiled_pairs),
                source_quote_observer={name: digest(PROJECT / name) for name in ('Cargo.toml', 'Cargo.lock', 'main.rs')},
                source_attr_sha256=digest(ROOT / '.reference/teaql-rs/teaql-macros/src/attr.rs'),
                source_derive_sha256=digest(ROOT / '.reference/teaql-rs/teaql-macros/src/derive_impl.rs'),
                verifier_sha256=digest(Path(__file__)), rust_compiler=run(['rustc', '--version']).stdout,
                mips_compiler=run([compilers['mips32'][0], '--version'], env=mips_environment).stdout,
                rust_mips_cfg=run(['rustc', '--print', 'cfg', '--target', 'mips-unknown-linux-musl']).stdout)
    if 'target_pointer_width="32"' not in data['rust_mips_cfg']:
        raise RuntimeError('Rust source target is not 32-bit MIPS')
    with tempfile.TemporaryDirectory(prefix='teaql_audit_width_506_') as temporary:
        work = Path(temporary)
        for name, value in zip(CASES, values):
            row = selected[name]
            original_item = next(item for item in row['source'] if item['entity'] == 'Item')
            if value['limit'] != original_item['limit']:
                raise RuntimeError(f'Host parser and actual generated derive differ: {name}')
            case = work / name
            case.mkdir()
            for relative, source in row['files'].items():
                if not relative.startswith('cpp/'):
                    continue
                path = case / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(source['text'])
                if digest(path) != source['sha256']:
                    raise RuntimeError(f'Complete generated header differs: {relative}')
            cpp = case / 'width.cpp'
            cpp.write_text('#include "cpp_privacy_contracts.hpp"\n')
            rust = case / 'width.rs'
            rust.write_text(PRELUDE + ('pub const LIMIT: usize = ' + value['literal'] + ';\n'
                                      if value['literal'] else 'pub const DECLARED: bool = false;\n'))
            observations = {}
            for target, rust_target in [('native64', 'x86_64-unknown-linux-gnu'),
                                        ('mips32', 'mips-unknown-linux-musl')]:
                expected = value['limit'] is None or int(value['limit']) <= (2 ** (32 if target == 'mips32' else 64) - 1)
                cpp_result = run([*compilers[target], '-fsyntax-only', '-I' + str(case / 'cpp/include'), cpp],
                                 env=mips_environment if target == 'mips32' else None, required=False)
                rust_result = run(['rustc', '--crate-type', 'lib', '--target', rust_target,
                                   '--emit=metadata', '-o', case / (target + '.rmeta'), rust],
                                  env=dict(os.environ, RUSTC_BOOTSTRAP='1'), required=False)
                if (cpp_result.returncode == 0) != expected or (rust_result.returncode == 0) != expected:
                    raise RuntimeError(f'Target-width acceptance differs: {name}/{target}')
                if not expected:
                    if 'audit_value_max_len exceeds target usize' not in cpp_result.stderr:
                        raise RuntimeError('C++ failed for an unrelated reason')
                    if 'literal out of range for `usize`' not in rust_result.stderr:
                        raise RuntimeError('Rust failed for an unrelated reason')
                observations[target] = {'accepted': expected,
                                        'cpp_exit_code': cpp_result.returncode,
                                        'rust_exit_code': rust_result.returncode}
            data['cases'].append({'name': name, 'input': inputs[name], 'source_quote': value,
                                   'rust_frontend_source': rust.read_text(), 'targets': observations})
            save()
    data.update(complete=True, target_case_pairs=10, expected_mips_rejections=2)
    save()
    print('Width contracts pass: five generated SDKs, native64/MIPS32, two intentional MIPS overflow refusals.')


if __name__ == '__main__':
    main()
