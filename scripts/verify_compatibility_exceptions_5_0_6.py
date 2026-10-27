#!/usr/bin/env python3
"""Check approved exception bounds and reject missing or changed relationship results."""
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

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from compatibility_exceptions_5_0_6 import MANIFEST, approved_exception
from compare_generated_relation_analysis import cpp_observation


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Preserve previous evidence; choose a fresh record')
    contract = json.loads(MANIFEST.read_text())
    approved = approved_exception('quoted_fk')
    approved_exception('invalid_wire_naming')
    corpus = json.loads(gzip.decompress((ROOT / approved['relationship_corpus']).read_bytes()))
    inputs = [case['input'] for case in corpus['cases']]
    payload = ''.join(json.dumps(value, ensure_ascii=True) + '\n' for value in inputs)
    record = dict(complete=False, scope=__doc__, authorization_controls=[], result_controls=[],
                  exception_manifest_sha256=hashlib.sha256(MANIFEST.read_bytes()).hexdigest(),
                  native=str(args.cpp.resolve()),
                  native_sha256=hashlib.sha256(args.cpp.read_bytes()).hexdigest())
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_compatibility_contracts_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            variants = []
            for field in ['runtime_commit', 'generator_commit']:
                invalid = copy.deepcopy(contract)
                invalid[field] = 'invalid'
                variants.append((field, invalid))
            invalid = copy.deepcopy(contract)
            invalid['quoted_fk']['status'] = 'pending'
            variants.append(('unapproved', invalid))
            invalid = copy.deepcopy(contract)
            invalid['evidence_sha256'] = {}
            variants.append(('missing_evidence', invalid))
            invalid = copy.deepcopy(contract)
            invalid['evidence_sha256'][next(iter(invalid['evidence_sha256']))] = '0' * 64
            variants.append(('changed_evidence', invalid))
            invalid = copy.deepcopy(contract)
            invalid['quoted_fk']['exception_case_indices'].pop()
            variants.append(('incomplete_indices', invalid))
            invalid = copy.deepcopy(contract)
            invalid['quoted_fk']['exception_case_indices'][0] = True
            variants.append(('boolean_index', invalid))
            invalid = copy.deepcopy(contract)
            invalid['quoted_fk']['documentation'] = 'docs/nonexistent_exception.md'
            variants.append(('missing_documentation', invalid))
            for name, invalid in variants:
                candidate = owned / (name + '.json')
                candidate.write_text(json.dumps(invalid))
                try:
                    approved_exception('quoted_fk', candidate)
                except (RuntimeError, KeyError, ValueError) as error:
                    record['authorization_controls'].append(dict(name=name, rejected=True,
                                                                   error=str(error)))
                else:
                    raise RuntimeError(f'Invalid exception contract accepted: {name}')
            environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off')
            native = subprocess.run([str(args.cpp.resolve())], input=payload, text=True,
                                    capture_output=True, env=environment, check=True)
            base = json_lines(native.stdout)
            assert len(base) == len(inputs) == 7122
            record['native_stderr'] = native.stderr
            record['native_stdout_sha256'] = hashlib.sha256(native.stdout.encode()).hexdigest()
            for mode in ['valid', 'missing_result', 'source_zero', 'extra_field']:
                outputs = copy.deepcopy(base)
                if mode == 'missing_result':
                    outputs.pop()
                elif mode == 'source_zero':
                    index = approved['exception_case_indices'][0]
                    outputs[index] = cpp_observation(corpus['cases'][index]['expected'])
                    assert not same(outputs[index], base[index])
                elif mode == 'extra_field':
                    outputs[0]['counterfactual_extra'] = True
                data = owned / (mode + '.jsonl')
                data.write_text(''.join(json.dumps(value, ensure_ascii=True) + '\n'
                                        for value in outputs))
                wrapper = owned / (mode + '_probe')
                source = ('#!' + sys.executable + '\nimport hashlib,pathlib,sys\n'
                          + 'assert hashlib.sha256(sys.stdin.buffer.read()).hexdigest() == '
                          + repr(hashlib.sha256(payload.encode()).hexdigest()) + '\n'
                          + 'sys.stdout.buffer.write(pathlib.Path(' + repr(str(data))
                          + ').read_bytes())\n')
                wrapper.write_text(source)
                wrapper.chmod(0o700)
                command = [sys.executable, str(ROOT / 'tests/parity/compare_generated_relation_analysis.py'),
                           '--cpp', str(wrapper), '--require-complete']
                result = subprocess.run(command, cwd=ROOT, env=environment,
                                        text=True, capture_output=True)
                record['result_controls'].append(dict(mode=mode, command=command,
                    wrapper_source=source, output_sha256=hashlib.sha256(data.read_bytes()).hexdigest(),
                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
                assert (result.returncode == 0) == (mode == 'valid'), mode
                print(mode + ': expected result verified', flush=True)
            record['owned_bytes_removed'] = sum(path.stat().st_size for path in owned.rglob('*')
                                               if path.is_file())
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        assert hashlib.sha256(MANIFEST.read_bytes()).hexdigest() == record['exception_manifest_sha256']
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Approved compatibility contracts: 8 invalid manifests reject; '
          '6576 exact + 546 approved relationship results checked with 3 rejection controls')


if __name__ == '__main__':
    main()
