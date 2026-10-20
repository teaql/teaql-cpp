#!/usr/bin/env python3
"""Qualify regenerated graph adapters through native and imported generated callers."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from run_native_integration_epoch_5_0_6 import capture_inputs, references

ROOT = Path(__file__).resolve().parents[1]
NAMES = [
    'generated_graph_serialization_contracts',
    'generated_json_graph_views_rust_parity',
    'generated_json_hydration_rust_parity',
    'generated_inherited_json_rust_parity',
    'generated_relation_contracts',
    'generated_relations_rust_parity',
    'generated_inheritance_graph_contracts',
    'generated_inheritance_graph_rust_parity',
    'generated_load_state_bulk_contracts',
    'generated_ledger_composition_contracts',
    'generated_dynamic_field_contracts',
    'generated_readonly_property_contracts',
    'generated_api_contracts',
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt')
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    update = ROOT / 'docs/parity/generated_embedded_graph_sdks_update_5_0_6.json.gz'
    reproduced = ROOT / 'docs/parity/generated_embedded_graph_sdks_reproduced_5_0_6.json.gz'
    for path in [update, reproduced]:
        assert json.loads(gzip.decompress(path.read_bytes()))['complete'], path
    record = dict(complete=False, scope='Generated reverse-relation adapter integration; not full parity',
                  inputs=capture_inputs(), references=references(), steps=[], profiles={},
                  regeneration={str(p.relative_to(ROOT)): digest(p) for p in [update, reproduced]})
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        print('exit', result.returncode, ' '.join(str(x) for x in command[:5]), flush=True)
        result.check_returncode()
        return result.stdout

    for profile, imported in [('native', False), ('modules_native', True)]:
        build = ROOT / ('build_modules_native' if imported else 'build_native_current')
        run(['cmake', '--preset', profile])
        registry = json.loads(run(['ctest', '--preset', profile, '--show-only=json-v1']))
        module_absent = {'generated_inheritance_graph_rust_parity', 'generated_relation_contracts',
                         'generated_relations_rust_parity', 'generated_api_contracts'}
        names = sorted(('module_' if imported else '') + name for name in NAMES
                       if not imported or name not in module_absent)
        if imported:
            names += ['module_dynamic_relation_contracts', 'module_relation_identity_generated_contracts']
            names.sort()
        if not imported:
            names.append('generated_api_type_errors')
            names.sort()
        registered = {t['name']: t for t in registry['tests']}
        assert set(names) <= registered.keys(), set(names) - registered.keys()
        binaries = set()
        for name in names:
            for argument in registered[name]['command']:
                path = Path(argument)
                if path.parent in [build, build / 'modules'] and path.name.startswith('teaql_'):
                    binaries.add(path)
        targets = sorted(p.name for p in binaries)
        run(['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets])
        hashes = {str(p): digest(p) for p in binaries}
        with tempfile.TemporaryDirectory(prefix='teaql_embedded_graph_integration_') as directory:
            selection = Path(directory) / 'selection.txt'
            selection.write_text('\n'.join(names) + '\n')
            junit = Path(directory) / 'results.xml'
            run(['ctest', '--preset', profile, '--output-on-failure', '--parallel', '2',
                 '--tests-from-file', selection, '--output-junit', junit])
            cases = ET.parse(junit).getroot().findall('testcase')
            assert sorted(c.attrib['name'] for c in cases) == names
            assert all(c.find('failure') is None and c.find('skipped') is None and c.find('error') is None for c in cases)
            result = dict(tests=names, junit=junit.read_text(), executables=hashes)
        if imported:
            run(['python3', 'scripts/verify_module_consumers.py', '--build-dir', build])
        else:
            log = (build / 'Testing/Temporary/LastTest.log').read_text()
            counts = re.findall(r'Verified (\d+) positive and (\d+) negative generated API compiler contracts', log)
            assert len(counts) == 1, 'Missing complete generated compiler gate'
            result['compiler'] = dict(positive=int(counts[0][0]), negative=int(counts[0][1]))
            result['terminal_log'] = log
        assert 'no work to do' in run(['cmake', '--build', '--preset', profile, '--parallel', '2', '--target', *targets])
        assert all(digest(Path(p)) == value for p, value in hashes.items())
        record['profiles'][profile] = result
        save()
    assert capture_inputs() == record['inputs'], 'Inputs changed during verification'
    assert references() == record['references'], 'Reference changed during verification'
    record['complete'] = True
    save()
    print('Generated graph SDK integration: native14/module11, compiler and unchanged-input gates passed')


if __name__ == '__main__':
    main()
