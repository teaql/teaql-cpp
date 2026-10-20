#!/usr/bin/env python3
"""Verify/regenerate C++ generator XID tables using the locked Rust dependency.

Rust is source-oracle tooling only. Neither Java generation nor the C++ runtime
loads a Rust helper. Existing Java code surrounding the table arrays is retained.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
RUST = r'''use serde_json::json;
fn ranges(predicate: fn(char) -> bool) -> Vec<(u32, u32)> {
    let mut result = Vec::new();
    let mut first = None;
    for value in 0..=0x110000 {
        let yes = char::from_u32(value).is_some_and(predicate);
        if yes && first.is_none() { first = Some(value); }
        if !yes {
            if let Some(start) = first.take() { result.push((start, value - 1)); }
        }
    }
    result
}
fn main() {
    println!("{}", json!({
        "start": ranges(unicode_ident::is_xid_start),
        "continue": ranges(unicode_ident::is_xid_continue),
        "whitespace": ranges(|c| c.is_whitespace() || c == '\u{200e}' || c == '\u{200f}')
    }));
}
'''
MANIFEST = '''[package]
name="teaql_audit_xid_reference"
version="0.0.0"
edition="2024"
[workspace]
[dependencies]
unicode-ident="=1.0.24"
serde_json="=1.0.150"
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generator-dir', type=Path,
                        default=ROOT.parent / 'teaql_code_gen_cpp')
    parser.add_argument('--write', action='store_true', help='Regenerate only the table arrays')
    parser.add_argument('--output', type=Path, help='New immutable compressed evidence receipt')
    args = parser.parse_args()
    if args.output and args.output.exists():
        parser.error('Evidence output already exists')
    reference = ROOT / '.reference/teaql-rs'
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    head = subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'],
                                   text=True).strip()
    if head != lock['commit'] or lock['version'] != '5.0.6':
        raise RuntimeError('Expected the exact active Rust 5.0.6 reference')
    target = args.generator_dir / ('generator/src/main/java/com/skynet/codegenerator/cpp/'
                                   'CppRustXidMetadata.java')
    before = digest(target)
    record = dict(scope='All Unicode scalar values in locked Rust XID/whitespace tables; '
                        'not complete attribute-expression or generated API qualification',
                  reference=head, target=str(target), before_sha256=before,
                  rust_source=RUST, manifest=MANIFEST)
    with tempfile.TemporaryDirectory(prefix='teaql_audit_xid_') as temporary:
        work = Path(temporary)
        (work / 'src').mkdir()
        (work / 'src/main.rs').write_text(RUST)
        (work / 'Cargo.toml').write_text(MANIFEST)
        (work / 'Cargo.lock').write_bytes((reference / 'Cargo.lock').read_bytes())
        command = ['cargo', 'run', '--offline', '--quiet', '--manifest-path',
                   str(work / 'Cargo.toml')]
        run = subprocess.run(command, text=True, capture_output=True)
        record.update(command=command, exit_code=run.returncode, stderr=run.stderr,
                      resolved_lock=(work / 'Cargo.lock').read_text())
        run.check_returncode()
        if not registry_identities(work / 'Cargo.lock') <= registry_identities(reference / 'Cargo.lock'):
            raise RuntimeError('Source-oracle registry identities differ from the reference')
        ranges = json.loads(run.stdout)
    source = target.read_text()
    for key, pairs in ranges.items():
        name = key.upper()
        pattern = rf'  private static final int\[\] {name} = \{{\n.*?\n  \}};'
        old = re.search(pattern, source, re.S)
        if not old:
            raise RuntimeError(f'Missing table: {name}')
        values = [value for pair in pairs for value in pair]
        if args.write:
            lines = [f'  private static final int[] {name} = {{']
            for offset in range(0, len(values), 10):
                line = '    ' + ', '.join(f'0x{value:X}' for value in values[offset:offset + 10])
                lines.append(line + (',' if offset + 10 < len(values) else ''))
            lines.append('  };')
            source = source[:old.start()] + '\n'.join(lines) + source[old.end():]
        elif [int(value, 16) for value in re.findall(r'0x[0-9A-F]+', old.group())] != values:
            raise RuntimeError(f'Source table differs: {name}')
    if args.write:
        target.write_text(source)
    record.update(complete=True, after_sha256=digest(target), tables=ranges,
                  code_points_per_property=0x110000, source=source)
    if args.output:
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))
    print('Verified 1,114,112 code points for each of XID_Start, XID_Continue and whitespace')


if __name__ == '__main__':
    main()
