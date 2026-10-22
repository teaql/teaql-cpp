#!/usr/bin/env python3
"""Reproduce active SDKs, allowing only the explicit flat-graph capability."""
import argparse
import gzip
import json
import re
import subprocess
import tempfile
from pathlib import Path
from regenerate_embedded_graph_adapters_5_0_6 import ROOT, PRODUCER, FAMILIES, digest, file_map


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt; do not overwrite historical evidence')
    record = dict(complete=False, apply=args.apply, steps=[], sdks=[])

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))

    def run(command, cwd=ROOT):
        result = subprocess.run(list(map(str, command)), cwd=cwd, text=True, capture_output=True)
        record['steps'].append(dict(command=list(map(str, command)), cwd=str(cwd),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    run(['mvn', '-q', '-pl', 'generator', '-am', '-DskipTests', 'package'], PRODUCER)
    entries = [(ROOT/'examples/generated', 'manifest.json', None)]
    entries += [(p.parent, 'manifest.json', None) for p in sorted((ROOT/'tests').glob('generated_*/manifest.json'))]
    for family, probe in FAMILIES.items():
        entries += [(p.parent, 'PROVENANCE.json', probe) for p in
                    sorted((ROOT/'tests/generator/parity'/family).glob('*/cpp/PROVENANCE.json'))]
    entries += [(ROOT/'tests/generator/parity'/family/'cpp', 'PROVENANCE.json', 'TypeConfigurationProbe')
                for family in ['query_alias_contracts', 'relation_identity_contracts',
                               'expression_root_contracts', 'graph_serialization_contracts',
                               'hydration_allocation_contracts']]
    active = {directory for directory, _, _ in entries}
    protected = {}
    for manifest in (ROOT/'tests/generator/parity').rglob('PROVENANCE.json'):
        if manifest.parent in active:
            continue
        for name, expected in json.loads(manifest.read_text())['files'].items():
            path = manifest.parent/name
            if digest(path) != expected:
                raise RuntimeError(f'Changed historical artifact: {path}')
            protected[str(path.relative_to(ROOT))] = expected
        protected[str(manifest.relative_to(ROOT))] = digest(manifest)
    record['protected_artifacts'] = protected
    staged = []
    with tempfile.TemporaryDirectory(prefix='teaql_flat_graph_sdk_') as temporary:
        work = Path(temporary)
        cp = work/'classpath.txt'
        run(['mvn', '-q', '-pl', 'generator', 'dependency:build-classpath', f'-Dmdep.outputFile={cp}'], PRODUCER)
        classpath = ':'.join(str(PRODUCER/p) for p in ['generator/target/classes', 'model/target/classes'])
        classpath += ':'+cp.read_text().strip()
        run(['javac', '-cp', classpath, '-d', work,
             *(ROOT/'tests/generator'/(probe+'.java') for probe in sorted(set(FAMILIES.values())))])
        classpath = str(work)+':'+classpath
        formatter = ROOT/'.tools/clang-format/bin/clang-format'
        for index, (destination, manifest_name, probe) in enumerate(entries):
            previous = json.loads((destination/manifest_name).read_text())
            before = file_map(destination, manifest_name)
            if before != previous['files']:
                raise RuntimeError(f'Artifact/provenance mismatch: {destination}')
            model = ROOT/previous['model']
            if digest(model) != previous['model_sha256']:
                raise RuntimeError(f'Model changed: {model}')
            output = work/str(index)
            run(['java', '-cp', classpath, probe or 'com.skynet.codegenerator.cpp.CppCoreGenerator',
                 *(['cpp'] if probe else []), model, output], PRODUCER)
            headers = sorted(output.rglob('*.hpp'))
            if headers:
                run([formatter, f'--style=file:{ROOT/".clang-format"}', '--Werror', '-i', *headers])
            after = file_map(output, manifest_name)
            if before.keys() != after.keys():
                raise RuntimeError(f'Changed file set: {destination}')
            changes = []
            for name in before:
                if before[name] == after[name]:
                    continue
                if not args.apply or not name.endswith('/support.hpp'):
                    raise RuntimeError(f'Unexpected generated change: {destination}/{name}')
                text = (output/name).read_text()
                restored, count = re.subn(r'    static constexpr bool requires_flat_graph_hydration\(\) \{\n        return true;\n    \}\n\n', '', text)
                if count != 1 or restored != (destination/name).read_text():
                    raise RuntimeError(f'Capability addition changes other code: {destination}/{name}')
                changes.append(name)
                staged.append((destination/name, (output/name).read_bytes()))
            current = dict(previous, files=after)
            current['generator_implementation_sha256'] = {
                path: digest(PRODUCER/path) for path in previous['generator_implementation_sha256']}
            manifest = (json.dumps(current, indent=2)+'\n').encode()
            if not args.apply and manifest != (destination/manifest_name).read_bytes():
                raise RuntimeError(f'Stale producer provenance: {destination}')
            staged.append((destination/manifest_name, manifest))
            record['sdks'].append(dict(path=str(destination.relative_to(ROOT)), changes=changes,
                                       before=before, after=after))
            save()
            print('Reproduced', destination.relative_to(ROOT), flush=True)
    if args.apply:
        for path, data in staged:
            if path.read_bytes() != data:
                path.write_bytes(data)
    for path, expected in protected.items():
        if digest(ROOT/path) != expected:
            raise RuntimeError(f'Protected artifact changed: {path}')
    record['complete'] = True
    record['temporary_directory_removed'] = not work.exists()
    save()
    print('Complete SDK reproduction:', len(entries), 'SDKs', flush=True)


if __name__ == '__main__':
    main()
