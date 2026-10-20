#!/usr/bin/env python3
"""Compare effective audit metadata with the complete unchanged locked Rust parser."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from verify_constant_closure_5_0_6 import registry_identities
from verify_generated_privacy_metadata import classpath, CPP

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'tests/parity/rust_audit_metadata_reference_5_0_6'
TARGET = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stimuli():
    previous = ROOT / 'docs/parity/generated_audit_literal_parser_repaired_5_0_6.json.gz'
    result = json.loads(gzip.decompress(previous.read_bytes()))['values']
    remaining = ROOT / 'docs/parity/generated_audit_literal_remaining_5_0_6.json.gz'
    result += [[f'retained_remaining_{index}', value] for index, value in enumerate(
        json.loads(gzip.decompress(remaining.read_bytes()))['values'])]
    for key in ['audit_value_max_len', 'audit_value_max_len_int']:
        for index, value in enumerate(['19', '18446744073709551616', '"19"', '"bad"',
                                       'false', '1e2', '(19)', '-19', '+19', 'false trailing']):
            prefix = '17, ' + key + '=' + value
            result += [[f'{key}_{index}', prefix],
                       [f'{key}_{index}_then_assignment', prefix + ', audit_value_max_len=21']]
    for index, value in enumerate([
        '17, other=0, audit_value_max_len=19',
        '17, other, another, audit_value_max_len=19',
        '17, other(), audit_value_max_len=19',
        '17, audit_mask_fields=""',
        '17, audit_mask_fields="name,NAME,name"',
        '17, audit_mask_fields="\u0085name\u00a0, \u200ename\u200f "',
        '17, audit_mask_fields="name", audit_mask_fields="secret"',
        '17, audit_mask_fields="name", other=0, audit_mask_fields="secret"',
        '17, audit_mask_fields=false',
        '17, audit_mask_fields="name", audit_value_max_len=19',
        '0x', '0xg', '0b2', '0o9', '17\u0301', '0x11a\u0301',
        '17 // trailing', '17 /* unfinished',
    ]):
        result.append([f'sequence_boundary_{index}', value])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--require-match', action='store_true')
    parser.add_argument('--extra-cases', type=Path, action='append', default=[],
                        help='Append named source-derived controls; repeat for independent matrices')
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a fresh evidence path')
    record = dict(scope='Effective audit limit/masks; complete source parser, installed generator',
                  execution_complete=False, complete=False, steps=[], cases=[])

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, **kwargs):
        result = subprocess.run([str(item) for item in command], text=True, capture_output=True, **kwargs)
        record['steps'].append(dict(command=[str(item) for item in command],
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(
        ROOT / '.reference/teaql-rs/Cargo.lock')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(TARGET),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml'],
        env=environment)
    values = stimuli()
    for extra_cases in args.extra_cases:
        additional = json.loads(extra_cases.read_text())
        assert additional['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        values += additional['cases']
    payload = ''.join(json.dumps(value) + '\n' for _, value in values)
    original = [json.loads(line) for line in run(
        [TARGET / 'debug/teaql_audit_metadata_reference_506'], input=payload).splitlines()]
    with tempfile.TemporaryDirectory(prefix='teaql_audit_metadata_') as temporary:
        work = Path(temporary)
        cp = classpath(CPP, work / 'classpath.txt')
        run(['javac', '-cp', cp, '-d', work, ROOT / 'tests/generator/AuditMetadataProbe.java'])
        installed = [json.loads(line) for line in run(
            ['java', '-cp', str(work) + ':' + cp, 'com.skynet.codegenerator.cpp.AuditMetadataProbe'],
            input=payload).splitlines()]
    assert len(original) == len(installed) == len(values)
    for (name, value), source, cpp in zip(values, original, installed):
        # Error messages/stages belong to different language boundaries. Retain
        # both in full; rejection agreement is distinct from exact valid outputs.
        equal = source == cpp if source['parsed'] else not cpp['parsed']
        record['cases'].append(dict(name=name, value=value, original=source,
                                     installed=cpp, match=equal))
    record['differences'] = [case for case in record['cases'] if not case['match']]
    record['source_rejections'] = sum(not value['parsed'] for value in original)
    record['valid_exact_matches'] = sum(case['match'] and case['original']['parsed']
                                        for case in record['cases'])
    paths = [PROJECT / 'main.rs', PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
             ROOT / '.reference/teaql-rs/teaql-macros/src/attr.rs', Path(__file__).resolve(),
             ROOT / 'tests/generator/AuditMetadataProbe.java']
    paths.extend(path.resolve() for path in args.extra_cases)
    paths += [CPP / 'generator/src/main/java/com/skynet/codegenerator/cpp' / (name + '.java')
              for name in ['CppRustLiteralMetadata', 'CppRustXidMetadata', 'CppJsonInputMetadata',
                           'CppFieldLayoutMetadata', 'CppCoreGenerator']]
    record['sources'] = {str(path): dict(sha256=digest(path), text=path.read_text()) for path in paths}
    record['execution_complete'] = True
    record['complete'] = not record['differences']
    save()
    print(f"Audit metadata: {len(values)-len(record['differences'])}/{len(values)} agree; "
          f"{record['valid_exact_matches']} exact valid outputs; "
          f"{record['source_rejections']} source rejections")
    for case in record['differences']:
        print(json.dumps(case, ensure_ascii=True))
    if args.require_match and record['differences']:
        raise SystemExit('Audit metadata differences remain')


if __name__ == '__main__':
    main()
