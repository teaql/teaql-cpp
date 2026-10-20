#!/usr/bin/env python3
"""Compare actual locked Java field-layout metadata APIs with the C++ producer."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'
REFERENCE = ROOT / '.reference/teaql_code_gen'


def field(name, kind='integer', **attributes):
    return dict(name=name, type=kind, **attributes)


def entity(name='Item', fields=None, **attributes):
    return dict(name=name, fields=copy.deepcopy(fields if fields is not None else
                [field('id', 'id'), field('version', 'version'), field('name', 'string')]),
                **attributes)


def cases():
    output = []

    def add(label, entities=None, target='Item', domain='geometry'):
        output.append(dict(label=label, domain=domain, target=target,
                           entities=copy.deepcopy(entities or [entity()])))

    for count in [0, 1, 2, 31, 32, 63, 64, 65, 127, 128, 129, 140, 512]:
        add(f'width_{count}', [entity(fields=[field('id', 'id'), field('version', 'version')]
                                    + [field(f'field_{i}') for i in range(count)])])
    for domain in ['', None, 'layout_domain', 'LayoutDomain', 'Geometry_\u4e16\u754c', 'd' * 2048]:
        add('domain_' + repr(domain), domain=domain)
    add('no_domain_owner', [entity(null_domain=True)])
    for name in ['ownerName', 'owner_name', 'HTTPServer', 'type', 'match', 'order',
                 '\u0394Name', 'field\x00name', 'field_' + 'x' * 2048, '#extension', '_derived']:
        add('field_' + repr(name), [entity(fields=[field('id', 'id'),
                                                field('version', 'version'), field(name)])])
    for kind in [None, '', 'integer', 'string', 'dateTime', 'decimal(18,7)']:
        add('type_' + repr(kind), [entity(fields=[field('name', kind)])])
    for column in ['', 'physical_name', '"order"', 'line\ncolumn', 'nul\x00column', '\u6570\u636e']:
        add('column_' + repr(column), [entity(fields=[field('ownerName', sql_column=column)])])
    for table in [None, '', 'physical_table', 'quoted"table', '\u8868']:
        add('table_' + repr(table), [entity(fields=[field('name', sql_table=table)])])
    for target_type in [None, '', 'Target', 'Other']:
        add('reference_' + repr(target_type), [entity(fields=[field('owner', 'object',
                                                                 entity_type=target_type,
                                                                 object=True)])])
    for depth in [1, 2, 3, 5]:
        entities = [entity('Base')]
        for i in range(depth):
            entities.append(entity('Child' + str(i), fields=[field('own_' + str(i))],
                                   parent='Base' if i == 0 else 'Child' + str(i - 1)))
        add(f'inherited_depth_{depth}', entities, entities[-1]['name'])
    for name in ['id', 'version', 'name']:
        kind = dict(id='id', version='version', name='string')[name]
        add('compatible_redeclaration_' + name,
            [entity('Base'), entity(fields=[field(name, kind, sql_column='ignored_child')], parent='Base')])
    add('reserved_duplicate_exclusion', [entity(fields=[field('#x'), field('#x'),
                                                       field('_x'), field('_x')])])
    add('ordered_fields', [entity(fields=[field('id'), field('a'), field('b')])])
    add('reordered_fields', [entity(fields=[field('id'), field('b'), field('a')])])
    for parent in ['', ' ', '\u2003', 'Missing']:
        add('parent_' + repr(parent), [entity(parent=parent)])
    for names in [('Item',), ('Item', 'Other'), ('Item', 'Other', 'Third')]:
        add('cycle_' + str(len(names)),
            [entity(name, parent=names[(i + 1) % len(names)]) for i, name in enumerate(names)])
    for name in [None, '', ' ', '\u2003']:
        add('blank_field_' + repr(name), [entity(fields=[field(name)])])
    for names in [('name', 'name'), ('ownerName', 'owner_name')]:
        add('duplicate_' + repr(names), [entity(fields=[field(name) for name in names])])
    add('incompatible_type', [entity('Base'), entity(fields=[field('name', 'integer')], parent='Base')])
    add('incompatible_reference', [entity('Base', fields=[field('owner', 'object', entity_type='A')]),
                                  entity(fields=[field('owner', 'object', entity_type='B')], parent='Base')])
    add('duplicate_child_override', [entity('Base'),
                                    entity(fields=[field('name', 'string'), field('name', 'string')],
                                           parent='Base')])
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True,
                        help='Fresh audit path; previous observations are never overwritten')
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve prior evidence; choose a fresh path'
    record = dict(scope='Direct descriptor API numbering, mapping, revision and failure behavior; '
                        'not malformed KSML parsing, full generation or runtime acceptance',
                  runtime_commit=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                  generator_commit=json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit'],
                  inputs=cases(), steps=[], hashes={}, complete=False)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record, ensure_ascii=False).encode(), mtime=0))

    def run(name, command, cwd=ROOT):
        result = subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        record['steps'].append(dict(name=name, command=list(map(str, command)),
                                    exit_code=result.returncode, output=result.stdout))
        save()
        result.check_returncode()

    run('runtime_lock', ['python3', 'scripts/verify_upstream.py'])
    run('generator_lock', ['python3', 'scripts/verify_generator_parity.py'])
    observer = ROOT / 'tests/generator/SharedLoadStateMetadataProbe.java'
    with tempfile.TemporaryDirectory(prefix='teaql_shared_layout_metadata_') as directory:
        work = Path(directory)
        classpaths = {}
        for name, workspace, module in [('reference', REFERENCE, 'model'),
                                        ('cpp', PRODUCER, 'generator')]:
            run(name + '_compile', ['mvn', '-q', '-o', '-pl', module, '-am', 'compile',
                                    '-DskipTests'], workspace)
            path = work / (name + '_classpath.txt')
            run(name + '_dependencies', ['mvn', '-q', '-o', '-pl', module,
                                         'dependency:build-classpath', f'-Dmdep.outputFile={path}'], workspace)
            classes = [workspace / 'model/target/classes']
            if name == 'cpp':
                classes.insert(0, workspace / 'generator/target/classes')
            classpaths[name] = ':'.join(map(str, classes)) + ':' + path.read_text().strip()
            for base in classes:
                relative = ('com/skynet/codegenerator/cpp/CppFieldLayoutMetadata.class'
                            if 'generator/target/classes' in str(base) else
                            'com/skynet/codegenerator/meta/ObjectDescriptor.class')
                binary = base / relative
                record['hashes'][str(binary)] = hashlib.sha256(binary.read_bytes()).hexdigest()
        run('observer_compile', ['javac', '-cp', classpaths['cpp'], '-d', str(work), str(observer)])
        inputs = work / 'inputs.json'
        inputs.write_text(json.dumps(record['inputs'], ensure_ascii=False))
        for name in ['reference', 'cpp']:
            output = work / (name + '_outputs.json')
            run(name + '_observe', ['java', '-cp', str(work) + ':' + classpaths[name],
                                    'SharedLoadStateMetadataProbe', name, str(inputs), str(output)])
            record[name] = json.loads(output.read_text())
    assert len(record['reference']) == len(record['cpp']) == len(record['inputs'])
    record['differences'] = [i for i, (a, b) in enumerate(zip(record['reference'], record['cpp'])) if a != b]
    expected_failures = {i for i, row in enumerate(record['inputs'])
                         if row['label'].startswith(('cycle_', 'blank_field_', 'duplicate_', 'incompatible_'))
                         or row['label'] == "parent_'Missing'"}
    actual_failures = {i for i, row in enumerate(record['reference']) if not row['ok']}
    record['unexpected_failures'] = sorted(actual_failures - expected_failures)
    record['unexpected_successes'] = sorted(expected_failures - actual_failures)
    record['successes'] = sum(row['ok'] for row in record['reference'])
    record['source_failures'] = len(record['reference']) - record['successes']
    for p in [Path(__file__), observer,
              REFERENCE / 'model/src/main/java/com/skynet/codegenerator/meta/ObjectDescriptor.java',
              REFERENCE / 'model/src/main/java/com/skynet/codegenerator/meta/PredefinedFieldIndex.java',
              PRODUCER / 'generator/src/main/java/com/skynet/codegenerator/cpp/CppFieldLayoutMetadata.java']:
        record['hashes'][str(p)] = hashlib.sha256(p.read_bytes()).hexdigest()
    record['complete'] = not (record['differences'] or record['unexpected_failures'] or
                              record['unexpected_successes'])
    save()
    print(f"{len(record['inputs'])} actual metadata comparisons; "
          f"{record['successes']} successes, {record['source_failures']} source failures, "
          f"{len(record['differences'])} differences")
    if not record['complete']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
