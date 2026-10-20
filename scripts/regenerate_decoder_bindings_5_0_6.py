#!/usr/bin/env python3
"""Reproduce all active SDKs; updates permit only checked decoder bindings."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
from reproduce_cpp_contract_fixtures import FAMILIES

ROOT = Path(__file__).resolve().parents[1]
PRODUCER = ROOT.parent / 'teaql_code_gen_cpp'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def file_map(directory, manifest):
    return {str(p.relative_to(directory)): digest(p) for p in sorted(directory.rglob('*'))
            if p.is_file() and p.name != manifest}


def restore_models(text):
    binding = (r'ArenaBorrow<FieldLayout>\s+field_layout_binding\s*\(\s*'
               r'std::allocator_arg,\s*Allocator\(r\),\s*field_layout\(r\)\);\n?')
    restored, bindings = re.subn(binding, '', text)
    restored, lookups = re.subn(r',\s*field_layout_binding\s*\)', ')', restored)
    if not bindings or lookups != text.count('GeneratedRelationAccess::lookupField('):
        raise RuntimeError('Incomplete binding/lookup delegate set')
    decoders = len(re.findall(r'::from_compact_row\(\s*Memory r,\s*CompactRow row\)', text))
    if decoders != bindings:
        raise RuntimeError('Require exactly one binding per generated decoder')
    return restored, dict(decoders=bindings, lookups=lookups)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true', help='Permit only the reviewed decoder binding delta')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh report; preserve historical evidence')
    record = dict(scope='All active SDK reproduction; narrow decoder-only change proof',
                  complete=False, apply=args.apply, steps=[], sdks=[])

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, cwd=ROOT):
        result = subprocess.run([str(x) for x in command], cwd=cwd, text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], cwd=str(cwd),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    run(['mvn', '-q', '-pl', 'generator', '-am', '-DskipTests', 'package'], PRODUCER)
    entries = [(ROOT / 'examples/generated', 'manifest.json', None)]
    entries += [(p.parent, 'manifest.json', None) for p in sorted((ROOT / 'tests').glob('generated_*/manifest.json'))]
    for family, probe in FAMILIES.items():
        entries += [(p.parent, 'PROVENANCE.json', probe)
                    for p in sorted((ROOT / 'tests/generator/parity' / family).glob('*/cpp/PROVENANCE.json'))]
    entries += [(ROOT / 'tests/generator/parity' / family / 'cpp', 'PROVENANCE.json', 'TypeConfigurationProbe')
                for family in ['query_alias_contracts', 'relation_identity_contracts', 'expression_root_contracts']]
    active = {directory for directory, _, _ in entries}
    protected = {}
    for provenance in (ROOT / 'tests/generator/parity').rglob('PROVENANCE.json'):
        if provenance.parent in active:
            continue
        source = json.loads(provenance.read_text())
        for name, expected in source['files'].items():
            path = provenance.parent / name
            if digest(path) != expected:
                raise RuntimeError(f'Changed protected artifact: {path}')
            protected[str(path.relative_to(ROOT))] = expected
        protected[str(provenance.relative_to(ROOT))] = digest(provenance)
    record['protected_artifacts'] = protected
    staged = []
    with tempfile.TemporaryDirectory(prefix='teaql_decoder_bindings_') as temporary:
        work = Path(temporary)
        cp_file = work / 'classpath.txt'
        run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath', f'-Dmdep.outputFile={cp_file}'], PRODUCER)
        classpath = ':'.join(str(PRODUCER / p) for p in ['generator/target/classes', 'model/target/classes'])
        classpath += ':' + cp_file.read_text().strip()
        run(['javac', '-cp', classpath, '-d', work,
             *(ROOT / 'tests/generator' / (probe + '.java') for probe in sorted(set(FAMILIES.values())))])
        classpath = str(work) + ':' + classpath
        formatter = ROOT / '.tools/clang-format/bin/clang-format'
        for index, (destination, manifest_name, probe) in enumerate(entries):
            previous = json.loads((destination / manifest_name).read_text())
            before = file_map(destination, manifest_name)
            if before != previous['files']:
                raise RuntimeError(f'Artifact/provenance mismatch: {destination}')
            output = work / 'outputs' / str(index)
            model = ROOT / previous['model']
            if digest(model) != previous['model_sha256']:
                raise RuntimeError(f'Model changed: {model}')
            command = ['java', '-cp', classpath, probe or 'com.skynet.codegenerator.cpp.CppCoreGenerator']
            run([*command, *(['cpp'] if probe else []), model, output], PRODUCER)
            headers = sorted(output.rglob('*.hpp'))
            if headers:
                run([formatter, f'--style=file:{ROOT / ".clang-format"}', '--Werror', '-i', *headers])
            after = file_map(output, manifest_name)
            if before.keys() != after.keys():
                raise RuntimeError(f'Changed SDK file set: {destination}')
            changes = []
            for name in before:
                if before[name] == after[name]:
                    continue
                if not args.apply or not name.endswith('/models.hpp'):
                    raise RuntimeError(f'Unexpected generated change: {destination}/{name}')
                restored, counts = restore_models((output / name).read_text())
                result = subprocess.run([str(formatter), f'--style=file:{ROOT / ".clang-format"}',
                                         '--Werror', '--assume-filename=' + name],
                                        input=restored, text=True, capture_output=True, check=True)
                if result.stdout != (destination / name).read_text():
                    raise RuntimeError(f'Decoder binding changed another body: {destination}/{name}')
                changes.append(dict(file=name, **counts))
                staged.append((destination / name, (output / name).read_bytes()))
            current = dict(previous, files=after)
            current['generator_implementation_sha256'] = {
                path: digest(PRODUCER / path) for path in previous['generator_implementation_sha256']}
            manifest = (json.dumps(current, indent=2) + '\n').encode()
            if not args.apply and manifest != (destination / manifest_name).read_bytes():
                raise RuntimeError(f'Stale generator provenance: {destination}')
            staged.append((destination / manifest_name, manifest))
            record['sdks'].append(dict(path=str(destination.relative_to(ROOT)), changes=changes,
                                       before=before, after=after))
            save()
            print('Reproduced', destination.relative_to(ROOT), flush=True)
    if args.apply:
        for path, data in staged:
            if path.read_bytes() != data:
                path.write_bytes(data)
    for relative, expected in protected.items():
        if digest(ROOT / relative) != expected:
            raise RuntimeError(f'Protected source/historical output changed: {relative}')
    record['complete'] = True
    save()
    print('Qualified complete SDK reproduction:', len(entries), 'SDKs;',
          'only decoder bindings permitted' if args.apply else 'every byte identical', flush=True)


if __name__ == '__main__':
    main()
