#!/usr/bin/env python3
"""Compare shared key-environment error payloads; this is not factory acceptance."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
from verify_business_runtime_configuration_5_0_6 import source_inputs

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Preserve existing evidence; choose a new record path')
    original = json.loads((ROOT / 'docs/parity/business_runtime_configuration_original_5_0_6.json').read_text())
    assert original['source_inputs'] == source_inputs()
    assert original['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    report = dict(scope='Shared key reader only: exact error payloads from actual original factory failures; '
                  'no claim of source-shaped factory setup/order/cleanup acceptance',
                  errors=[], versions=[], fresh_boundaries=[])

    def observe(encoded, version):
        environment = {os.fsencode(name): os.fsencode(value) for name, value in os.environ.items()}
        environment[b'TEAQL_LOG_ENDPOINT'] = b'off'
        for name, value in [('TEAQL_BUSINESS_ID_KEY_V1_HEX', encoded),
                            ('TEAQL_BUSINESS_ID_KEY_VERSION', version)]:
            name = os.fsencode(name)
            environment.pop(name, None)
            if value is not None:
                environment[name] = os.fsencode(value)
        result = subprocess.run([str(args.cpp.resolve()), '--observe'], env=environment,
                                text=True, capture_output=True)
        return result

    def parsed(result):
        assert result.returncode == 0, result.stderr
        return json.loads(result.stdout)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(report).encode(), mtime=0))

    for case in original['source_environment_errors']:
        source = case['source_expected']['rounds'][0]
        assert source['assembled'] is False
        actual = parsed(observe(case['key_hex'], case['key_version']))
        payload = {name: source[name] for name in ['display', 'debug', 'source']}
        matching = actual['key_loaded'] is False and actual['error'] == payload
        report['errors'].append(dict(name=case['name'], entry=case['input']['entry'],
                                     original=case, actual=actual, matching=matching))
        save()
        assert matching, (case['name'], actual, source)
    for version in sorted({case['input']['key_version'] for case in original['cases']}):
        actual = parsed(observe(bytes(range(32)).hex(), str(version)))
        report['versions'].append(dict(source_version=version, actual=actual))
        save()
        assert actual == dict(key_loaded=True, version=version)
    witnesses = json.loads(gzip.decompress(
        (ROOT / 'docs/parity/business_key_environment_source_witnesses_5_0_6.json.gz').read_bytes()))
    assert witnesses['complete'] and witnesses['source_inputs'] == source_inputs()
    for case in witnesses['fresh']:
        encoded = bytes.fromhex(case['key_hex_bytes']) if case['key_hex_bytes'] is not None else None
        version = bytes.fromhex(case['version_bytes']) if case['version_bytes'] is not None else None
        result = observe(encoded, version)
        row = dict(original=case, cpp_exit_code=result.returncode,
                   cpp_stdout=result.stdout, cpp_stderr=result.stderr)
        report['fresh_boundaries'].append(row)
        save()
        if case['exit_code']:
            assert case['exit_code'] == 101 and result.returncode == 1
            assert 'not a char boundary' in case['stderr'] and 'UTF-8 character boundary' in result.stderr
            row['disposition'] = 'Original Rust panic maps to a C++ out_of_range exception; '
            row['disposition'] += 'both interrupt instead of returning a typed configuration result. '
            row['disposition'] += 'Process exit codes and diagnostics remain distinct and unnormalized.'
        else:
            actual = parsed(result)
            expected = case['actual']['rounds'][0]
            assert actual['key_loaded'] is expected['assembled']
            if expected['assembled']:
                assert actual['version'] == 1
            else:
                assert actual['error'] == {name: expected[name] for name in ['display', 'debug', 'source']}
            row['disposition'] = 'Original source boundary matches the shared key-read responsibility.'
        save()
    report.update(complete=True, error_payloads=len(report['errors']),
                  executable_sha256=hashlib.sha256(args.cpp.read_bytes()).hexdigest(),
                  source_inputs=original['source_inputs'])
    save()
    print('18/18 original error payloads, three source key versions and seven original boundary '
          'dispositions verified; source-shaped factory integration remains unqualified.')


if __name__ == '__main__':
    main()
