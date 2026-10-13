#!/usr/bin/env python3
"""Compare original fixed-layout parent-input validation with the C++ adapter."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT/'tests/parity/inheritance_metadata_5_0_6_cases.json'


def field(name, kind='String', **kwargs):
    return dict(name=name, type=kind, **kwargs)


def entity(name, fields, parent=None):
    value = dict(name=name, fields=fields)
    if parent is not None:
        value['parent'] = parent
    return value


def cases():
    base = entity('base', [field('id', 'Long'), field('displayName'), field('active', 'Boolean')])
    for parent in ['missing', 'base', '', '   ']:
        yield dict(entities=[base, entity('child', [field('amount', 'Decimal')], parent)], target='child')
    for name in ['displayName', 'display_name', 'active', 'id']:
        for kind in ['String', 'Boolean', 'Long', 'Decimal']:
            yield dict(entities=[base, entity('child', [field(name, kind)], 'base')], target='child')
    for names in [['child'], ['base', 'child'], ['base', 'middle', 'child']]:
        definitions = [entity(name, [field('value')], names[(index+1) % len(names)]) for index, name in enumerate(names)]
        yield dict(entities=definitions, target=names[-1])
    for name in [None, '', '   ', '_computed', '#dynamic', 'camelCase', 'camel_case']:
        yield dict(entities=[base, entity('child', [field(name), field(name)], 'base')], target='child')
    for parent_type, child_type in [('Tenant', 'Tenant'), ('Tenant', 'Other'), ('', 'Tenant')]:
        yield dict(entities=[entity('base', [field('owner', 'Long', entity_type=parent_type)]),
                            entity('child', [field('owner', 'Long', entity_type=child_type)], 'base')], target='child')
    for depth in [2, 8, 32]:
        definitions = [entity('level_0', [field('id', 'Long'), field('name')])]
        for index in range(1, depth):
            definitions.append(entity(f'level_{index}', [] if index % 2 else [field(f'value_{index}', 'Long')], f'level_{index-1}'))
        yield dict(entities=definitions, target=f'level_{depth-1}')


def run(workspace, inputs, original, temporary):
    subprocess.run(['mvn', '-q', '-pl', 'generator', '-am', '-DskipTests', 'package'], cwd=workspace, check=True, stdout=subprocess.DEVNULL)
    directory = temporary/('original' if original else 'cpp')
    directory.mkdir()
    classpath = directory/'classpath.txt'
    subprocess.run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath', f'-Dmdep.outputFile={classpath}'], cwd=workspace, check=True, stdout=subprocess.DEVNULL)
    cp = ':'.join(str(workspace/path) for path in ['generator/target/classes', 'model/target/classes']) + ':' + classpath.read_text().strip()
    source = (ROOT/'tests/generator/inheritance_metadata_probe.java.template').read_text()
    source = source.replace('FIELD_EXPRESSION', 'entity.getPredefinedFields()' if original else 'CppFieldLayoutMetadata.fields(entity)')
    source = source.replace('REVISION_EXPRESSION', 'entity.getPredefinedFieldLayoutRevision()' if original else 'CppFieldLayoutMetadata.revision(entity)')
    path = directory/'InheritanceMetadataProbe.java'
    path.write_text(source)
    subprocess.run(['javac', '-cp', cp, '-d', str(directory), str(path)], check=True)
    result = subprocess.run(['java', '-cp', str(directory)+':'+cp, 'com.skynet.codegenerator.cpp.InheritanceMetadataProbe'],
                            input=''.join(json.dumps(v)+'\n' for v in inputs), text=True, capture_output=True, check=True)
    outputs = [json.loads(line[len('TEAQL_CASE:'):]) for line in result.stdout.splitlines() if line.startswith('TEAQL_CASE:')]
    if len(outputs) != len(inputs):
        raise SystemExit('Incomplete metadata probe output')
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_generator_parity.py')], check=True)
    inputs = list(cases())
    with tempfile.TemporaryDirectory(prefix='teaql_parent_metadata_') as temporary:
        original = run(ROOT/'.reference/teaql_code_gen', inputs, True, Path(temporary))
        cpp = run(ROOT.parent/'teaql_code_gen_cpp', inputs, False, Path(temporary))
    for case, source, target in zip(inputs, original, cpp):
        if source != target:
            raise SystemExit(f'Parent-input mismatch: {case}\nOriginal: {source}\nC++: {target}')
    ledger = json.loads((ROOT/'docs/cpp_generator_parity_ledger.json').read_text())
    value = dict(baseline=ledger['runtime_commit'], generator=ledger['generator_commit'],
                 note='Direct ObjectDescriptor fixed-layout API versus CppFieldLayoutMetadata on constructed metadata. Exact ordered fields, owners, revision and exception type/message. This does not claim malformed KSML frontend or full generator pipeline parity.',
                 cases=[dict(input=case, expected=result) for case, result in zip(inputs, original)])
    data = json.dumps(value, indent=2)+'\n'
    if args.check:
        # Java Map.of does not promise object-key iteration order. Compare JSON
        # values while preserving field-array order and every diagnostic value.
        if not OUTPUT.is_file() or json.loads(OUTPUT.read_text()) != value:
            raise SystemExit('Retained parent metadata observations differ')
    else:
        OUTPUT.write_text(data)
    print(f'Compared {len(inputs)} exact original/C++ parent metadata observations')


if __name__ == '__main__':
    main()
