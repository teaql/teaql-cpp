#!/usr/bin/env python3
"""Check actual default-corpus bindings and inventory admission using isolated copies."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from inventory_parity_baselines import direct_corpus


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    assert args.record is None or not args.record.exists()
    comparator = ROOT / 'tests/parity/compare_values.py'
    source = ROOT / 'scripts/inventory_parity_baselines.py'
    archived = ROOT / 'docs/parity/corpus_inventory_before_5_0_6.json.gz'
    before = json.loads(gzip.decompress(archived.read_bytes()))
    assert hashlib.sha256(before['source'].encode()).hexdigest() == before['sha256']
    namespace = {'__file__': str(source), '__name__': 'preceding_inventory'}
    exec(compile(before['source'], '<preceding inventory>', 'exec'), namespace)
    old = namespace['direct_corpus']
    expected = 'value_5_0_6_cases.json.gz'
    fixtures = [('default', 'from compare_values import main\nmain()\n', expected),
                ('alias', 'from compare_values import main as check\ncheck()\n', expected),
                ('explicit', f'from compare_values import main\nmain({expected!r})\n', expected),
                ('historical', 'from compare_values import main\nmain("value_cases.json")\n', 'value_cases.json'),
                ('shadow', 'from compare_values import main\ndef main(): pass\nmain()\n', None),
                ('dynamic', 'from compare_values import main\nmain(corpus_name)\n', None),
                ('ambiguous', 'from compare_values import main\nmain("a.json")\nmain("b.json")\n', None)]
    record = dict(complete=False, scope=__doc__, binding_controls=[], shape_controls=[],
                  input_hashes_before={str(p): digest(p) for p in [Path(__file__), source, comparator, archived]})
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_corpus_inventory_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            actual = direct_corpus(comparator)
            assert actual == expected
            record['binding_controls'].append(dict(name='actual_shared_entry', previous=old(comparator), actual=actual, expected=expected))
            for name, text, wanted in fixtures:
                wrapper = owned / (name + '.py')
                wrapper.write_text(text)
                actual = direct_corpus(wrapper)
                assert actual == wanted, name
                record['binding_controls'].append(dict(name=name, previous=old(wrapper), actual=actual, expected=wanted))
            scripts = owned / 'scripts'
            parity = owned / 'tests/parity'
            scripts.mkdir()
            parity.mkdir(parents=True)
            (parity / 'compare_values.py').write_bytes(comparator.read_bytes())
            wrapper = parity / 'compare_fixture.py'
            wrapper.write_text('from compare_values import main\nmain("cases.json")\n')
            (owned / 'upstream.lock.json').write_bytes((ROOT / 'upstream.lock.json').read_bytes())
            registry = owned / 'registry.json'
            registry.write_text(json.dumps(dict(tests=[dict(name='controlled_binding', command=[sys.executable, str(wrapper)])])))
            environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
            for name, cases, wanted in [('valid', [{'input': None, 'expected': None}], 'active_baseline'),
                                        ('empty_list', [], 'invalid_corpus_shape'),
                                        ('empty_object', {}, 'invalid_corpus_shape'),
                                        ('empty_string', '', 'invalid_corpus_shape')]:
                lock = json.loads((ROOT / 'upstream.lock.json').read_text())
                (parity / 'cases.json').write_text(json.dumps(dict(baseline=lock['commit'], cases=cases)))
                observations = {}
                for version, code in [('previous', before['source']), ('current', source.read_text())]:
                    executable = scripts / 'inventory_parity_baselines.py'
                    executable.write_text(code)
                    output = owned / 'report.json'
                    command = [sys.executable, str(executable), '--registry', str(registry), '--report', str(output), '--check']
                    result = subprocess.run(command, cwd=owned, env=environment, text=True, capture_output=True)
                    report = json.loads(output.read_text())
                    observations[version] = dict(status=report['tests'][0]['status'], exit_code=result.returncode,
                                                 command=command, stdout=result.stdout, stderr=result.stderr)
                assert observations['current']['status'] == wanted
                assert observations['current']['exit_code'] == (0 if name == 'valid' else 1)
                record['shape_controls'].append(dict(name=name, **observations))
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        assert sum(c['actual'] != c['previous'] for c in record['binding_controls']) == 3
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        record['input_hashes_after'] = {str(p): digest(p) for p in [Path(__file__), source, comparator, archived]}
        assert record['input_hashes_before'] == record['input_hashes_after']
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Eight binding controls pass; three default bindings corrected; three previously accepted malformed inventories now reject')


if __name__ == '__main__':
    main()
