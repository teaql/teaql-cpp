#!/usr/bin/env python3
"""Exercise the real shared comparator with valid and malformed corpus copies."""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--allow-differences', action='store_true')
    args = parser.parse_args()
    assert args.cpp.is_file()
    assert args.record is None or not args.record.exists()
    comparator = ROOT / 'tests/parity/compare_values.py'
    corpus_path = ROOT / 'tests/parity/value_5_0_6_cases.json.gz'
    corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
    assert len(corpus['cases']) == 722
    variants = [('complete_source', corpus, True)]
    one = copy.deepcopy(corpus)
    one['cases'] = one['cases'][:1]
    variants.append(('one_valid_observation', one, True))
    for name, value in [('empty_list', []), ('empty_object', {}), ('empty_string', ''),
                        ('null_cases', None), ('false_cases', False), ('numeric_cases', 0),
                        ('null_record', [None]), ('missing_input', [{'expected': None}]),
                        ('missing_expected', [{'input': None}])]:
        changed = copy.deepcopy(corpus)
        changed['cases'] = value
        variants.append((name, changed, False))
    record = dict(complete=False, scope=__doc__, cases=[], comparator_source=comparator.read_text(),
                  input_hashes_before={str(p): digest(p) for p in [comparator, corpus_path, args.cpp.resolve()]})
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_corpus_admission_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            directory = owned / 'tests/parity'
            directory.mkdir(parents=True)
            (owned / 'upstream.lock.json').write_bytes((ROOT / 'upstream.lock.json').read_bytes())
            script = directory / 'compare_values.py'
            script.write_bytes(comparator.read_bytes())
            environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', TEAQL_LOG_ENDPOINT='off')
            for name, data, valid in variants:
                (directory / corpus_path.name).write_bytes(gzip.compress(json.dumps(data).encode(), mtime=0))
                command = [sys.executable, str(script), '--cpp', str(args.cpp.resolve())]
                result = subprocess.run(command, cwd=owned, env=environment, text=True, capture_output=True)
                accepted = result.returncode == 0
                record['cases'].append(dict(name=name, valid=valid, accepted=accepted,
                                            matches=accepted is valid, command=command,
                                            exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['owned_directory_removed'] = not owned.exists()
        record['input_hashes_after'] = {str(p): digest(p) for p in [comparator, corpus_path, args.cpp.resolve()]}
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = all(c['matches'] for c in record['cases'])
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Corpus admission:', sum(c['matches'] for c in record['cases']), '/', len(record['cases']),
          '; false passes:', [c['name'] for c in record['cases'] if c['accepted'] and not c['valid']])
    if not args.allow_differences:
        assert record['complete'], 'Malformed corpus or valid source admission differs'


if __name__ == '__main__':
    main()
