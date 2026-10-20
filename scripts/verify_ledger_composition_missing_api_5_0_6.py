#!/usr/bin/env python3
"""Compile the preserved complete pre-fix SDK and prove the canonical API was absent."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    archive = ROOT / 'docs/parity/ledger_composition_sdks_before_fix_5_0_6.json.gz'
    snapshot = json.loads(gzip.decompress(archive.read_bytes()))
    sdk = snapshot['tests/generator/parity/relation_identity_contracts/cpp']
    support = next(text for name, text in sdk.items() if name.endswith('/support.hpp'))
    assert 'includePendingMutationsFrom' not in support
    commands = json.loads((args.build_dir / 'compile_commands.json').read_text())
    row = next(row for row in commands if row['file'].endswith('/tests/generator/generated_api_tests.cpp'))
    original = row.get('arguments') or shlex.split(row['command'])
    command, skip = [], False
    for part in original:
        if skip:
            skip = False
            continue
        if part == '-o':
            skip = True
        elif part != '-c' and part != row['file']:
            command.append(part)
    with tempfile.TemporaryDirectory(prefix='teaql_missing_composition_') as temporary:
        work = Path(temporary)
        for name, text in sdk.items():
            path = work / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
        source = work / 'missing.cpp'
        source.write_text('#include "include/cpp_relation_identities.hpp"\n'
                          'void exercise(teaql::UserContext& user_context) {\n'
                          'auto& parent = user_context.make<cpp_relation_identities::Owner>();\n'
                          'auto& child = user_context.make<cpp_relation_identities::Entry>();\n'
                          'parent.includePendingMutationsFrom(child);\n}\n')
        command += ['-fsyntax-only', '-ferror-limit=2', str(source)]
        result = subprocess.run(command, text=True, capture_output=True)
        assert result.returncode != 0 and "no member named 'includePendingMutationsFrom'" in result.stderr, result.stderr
        if args.report:
            assert not args.report.exists()
            args.report.write_bytes(gzip.compress(json.dumps(dict(
                snapshot_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),
                support_sha256=hashlib.sha256(support.encode()).hexdigest(),
                source=source.read_text(), command=command, exit_code=result.returncode,
                stdout=result.stdout, stderr=result.stderr)).encode(), mtime=0))
    print('Verified real compiler rejection of the canonical API in the complete original C++ SDK')


if __name__ == '__main__':
    main()
