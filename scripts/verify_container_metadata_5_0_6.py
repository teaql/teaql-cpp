#!/usr/bin/env python3
"""Observe container metadata through unchanged Rust source and the C++ producer."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from verify_audit_metadata_sequences_5_0_6 import stimuli
from verify_constant_closure_5_0_6 import registry_identities
from verify_generated_privacy_metadata import classpath, CPP

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'tests/parity/rust_container_metadata_reference_5_0_6'


def controls():
    result = []
    for key in ('entity', 'table', 'data_service'):
        for index, value in enumerate(('"Other"', 'r#"Other"#', '""', '"雪"',
                                       'false', '19', '("Other")', 'ignored!()',
                                       '"Other" +', '"Other" trailing')):
            result.append((f'{key}_{index}', f'17, {key}={value}, audit_value_max_len=21'))
        result.append((f'{key}_twice', f'17, {key}="first", {key}="last", audit_value_max_len=21'))
    for index, settings in enumerate((
        '', 'name="others"', 'target="Child"',
        'name="others", target="Child"',
        'name="others", target="Child", many',
        'name="others", target="Child", local_key="version", foreign_key="item", many,',
        'name="others", name="last", target="Child", target="Item", many, many',
        'name="others", unknown, target="Child", many',
        'name="others", unknown=0, target="Child"',
        'name="others", unknown(), target="Child"',
        'name="others", many=false, target="Child"',
        'name=false', 'target=19', 'local_key=("id")', 'foreign_key=ignored!()',
        'name="others", json_type="Child"',
        'name="others", json_type="crate::Child"',
        'name="others", json_type="&\'a Child"',
        'name="others", json_type="Vec<Child>"',
        'name="others", json_type="(Child,)"',
        'name="others", json_type="[Child; 2]"',
        'name="others", json_type="fn(Child) -> Child"',
        'name="others", json_type="dyn Trait + Send"',
        'name="others", json_type=""',
        'name="others", json_type="Child +"',
        'name="others", json_type="Vec<,>"',
        'name="others", json_type=false',
        'name="others", json_type="Child", json_type="bad!"',
    )):
        result.append((f'reverse_{index}', f'17, reverse_relation({settings}), audit_value_max_len=21'))
    result += [
        ('indexed', '17, indexed_layout, audit_value_max_len=21'),
        ('indexed_bad_value', '17, indexed_layout=false, audit_value_max_len=21'),
        ('unknown_then_entity', '17, unknown, entity="Other", audit_value_max_len=21'),
        ('unknown_assignment_then_entity', '17, unknown=0, entity="Other", audit_value_max_len=21'),
        ('multiple_reverse', '17, reverse_relation(name="a", target="Child"), '
         'reverse_relation(name="b", target="Item", many), audit_value_max_len=21'),
        ('failed_second_reverse', '17, reverse_relation(name="a", target="Child"), '
         'reverse_relation(name="b", unknown=0), audit_value_max_len=21'),
    ]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--require-match', action='store_true')
    parser.add_argument('--extra-cases', type=Path, action='append', default=[])
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a fresh output path; existing evidence is immutable')
    record = dict(scope='All parsed descriptor effects, indexed flag and typed-relation presence; '
                  'actual generated JSON execution is a separate gate.',
                  execution_complete=False, complete=False, steps=[], cases=[])

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, **kwargs):
        command = [str(value) for value in command]
        result = subprocess.run(command, text=True, capture_output=True, **kwargs)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT / 'Cargo.toml'], env=environment)
    assert registry_identities(PROJECT / 'Cargo.lock') <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    values = stimuli() + controls()
    for path in args.extra_cases:
        additional = json.loads(path.read_text())
        assert additional['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        values += additional['cases']
    payload = ''.join(json.dumps(value) + '\n' for _, value in values)
    original = [json.loads(line) for line in run(
        [Path(environment['CARGO_TARGET_DIR']) / 'debug/teaql_container_metadata_reference_506'],
        input=payload, env=environment).splitlines()]
    with tempfile.TemporaryDirectory(prefix='teaql_container_metadata_') as temporary:
        work = Path(temporary)
        cp = classpath(CPP, work / 'classpath.txt')
        run(['javac', '-cp', cp, '-d', work, ROOT / 'tests/generator/ContainerMetadataProbe.java'])
        installed = [json.loads(line) for line in run(
            ['java', '-cp', str(work) + ':' + cp, 'com.skynet.codegenerator.cpp.ContainerMetadataProbe'],
            input=payload).splitlines()]
    assert len(original) == len(installed) == len(values)
    for (name, value), source, actual in zip(values, original, installed):
        equal = source == actual if source['parsed'] else not actual['parsed']
        record['cases'].append(dict(name=name, value=value, original=source, installed=actual, match=equal))
    record['differences'] = [case for case in record['cases'] if not case['match']]
    record['source_rejections'] = sum(not value['parsed'] for value in original)
    sources = [Path(__file__), PROJECT / 'main.rs', PROJECT / 'Cargo.toml', PROJECT / 'Cargo.lock',
               ROOT / 'tests/generator/ContainerMetadataProbe.java',
               ROOT / '.reference/teaql-rs/teaql-macros/src/attr.rs', *args.extra_cases]
    sources += [CPP / 'generator/src/main/java/com/skynet/codegenerator/cpp' / (name + '.java')
                for name in ('CppRustLiteralMetadata', 'CppRustXidMetadata', 'CppJsonInputMetadata',
                             'CppFieldLayoutMetadata', 'CppCoreGenerator')]
    record['sources'] = {str(path.resolve()): dict(sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                                 text=path.read_text()) for path in sources}
    record['execution_complete'] = True
    record['complete'] = not record['differences']
    save()
    print(f"Container metadata: {len(values)-len(record['differences'])}/{len(values)} agree; "
          f"{record['source_rejections']} source rejections")
    for case in record['differences']:
        print(json.dumps(case, ensure_ascii=True))
    if args.require_match and record['differences']:
        raise SystemExit('Container metadata differences remain')


if __name__ == '__main__':
    main()
