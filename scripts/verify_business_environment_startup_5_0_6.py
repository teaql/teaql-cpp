#!/usr/bin/env python3
"""Compare complete generated startup/save/readback, including environment failures."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sqlite3
import tempfile
from contextlib import closing
from verify_business_runtime_configuration_5_0_6 import source_inputs
from verify_legacy_observation_identity_5_0_6 import verify_record

ROOT = Path(__file__).resolve().parents[1]
BOUNDARIES = ROOT / 'docs/parity/business_environment_source_boundaries_5_0_6.json.gz'
FILESYSTEM = ROOT / 'docs/parity/business_environment_source_filesystem_5_0_6.json.gz'


def run(executable, value, key, version, database_url=':memory:', cwd=None):
    environment = {os.fsencode(k): os.fsencode(v) for k, v in os.environ.items()}
    environment[b'TEAQL_LOG_ENDPOINT'] = b'off'
    environment[b'CPP_BUSINESS_IDS_CORE_DATABASE_URL'] = os.fsencode(database_url)
    for name, content in [(b'TEAQL_BUSINESS_ID_KEY_V1_HEX', key),
                          (b'TEAQL_BUSINESS_ID_KEY_VERSION', version)]:
        environment.pop(name, None)
        if content is not None:
            environment[name] = os.fsencode(content)
    result = subprocess.run([str(executable.resolve())], input=json.dumps(value) + '\n',
                            env=environment, cwd=cwd, text=True, capture_output=True)
    assert 'ERROR: AddressSanitizer' not in result.stderr, result.stderr
    assert 'UndefinedBehaviorSanitizer:' not in result.stderr, result.stderr
    return dict(exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr,
                actual=json.loads(result.stdout) if result.returncode == 0 else None)


def filesystem_run(executable, key, version):
    with tempfile.TemporaryDirectory(prefix='teaql_key_startup_') as directory:
        work = Path(directory)
        value = dict(entry='env', rounds=1, key_version=1)
        result = run(executable, value, key, version,
                     'sqlite://./nested/startup.sqlite?mode=rwc', work)
        result['filesystem'] = {str(p.relative_to(work)): p.stat().st_size if p.is_file() else 'directory'
                                for p in sorted(work.rglob('*'))}
        database = work / 'nested/startup.sqlite'
        assert database.is_file(), result
        with closing(sqlite3.connect(f'file:{database}?mode=ro', uri=True)) as connection:
            result['tables'] = connection.execute('SELECT name FROM sqlite_master ORDER BY name').fetchall()
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path)
    parser.add_argument('--observe-original', type=Path)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--filesystem-only', action='store_true',
                        help='Record original file creation before key startup failure')
    args = parser.parse_args()
    assert bool(args.cpp) != bool(args.observe_original)
    if args.record and args.record.exists():
        parser.error('Preserve prior evidence; select a new record path')
    original_path = ROOT / 'docs/parity/business_runtime_configuration_original_5_0_6.json'
    verify_record(original_path)
    original = json.loads(original_path.read_text())
    inputs = source_inputs()
    assert original['source_inputs'] == inputs
    assert original['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    executable = args.cpp or args.observe_original
    report = dict(scope='Actual unchanged original factories' if args.observe_original else
                  'Actual generated C++ factories, without manual key injection; full startup/save/readback comparison',
                  source_inputs=inputs, executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                  cases=[], complete=False)
    report['runtime_commit'] = original['runtime_commit']
    report['generator_commit'] = original['generator_commit']

    def save():
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(report).encode(), mtime=0))

    if args.observe_original:
        witnesses = json.loads(gzip.decompress(
            (ROOT / 'docs/parity/business_key_environment_source_witnesses_5_0_6.json.gz').read_bytes()))
        assert witnesses['complete'] and witnesses['source_inputs'] == inputs
        assert report['executable_sha256'] == witnesses['binary_sha256']
        if args.filesystem_only:
            assert args.record == FILESYSTEM
            for case in original['source_environment_errors']:
                if case['input']['entry'] != 'env' or case['name'] not in ('missing_key', 'invalid_hex'):
                    continue
                result = filesystem_run(executable, case['key_hex'], case['key_version'])
                report['cases'].append(dict(name=case['name'], key_hex=case['key_hex'],
                                             key_version=case['key_version'], **result))
                save()
                assert result['exit_code'] == 0 and result['actual'] == case['source_expected']
                assert result['tables'] == []
            assert len(report['cases']) == 2
            report['complete'] = True
            save()
            print('Recorded two actual original file-creation/key-failure outcomes without schema.')
            return
        assert args.record == BOUNDARIES
        for case in witnesses['fresh']:
            key = bytes.fromhex(case['key_hex_bytes']) if case['key_hex_bytes'] is not None else None
            version = bytes.fromhex(case['version_bytes']) if case['version_bytes'] is not None else None
            for entry in ('pool', 'config', 'env'):
                value = dict(entry=entry, rounds=1, key_version=1)
                result = run(executable, value, key, version)
                row = dict(name=case['name'], input=value, key_hex_bytes=case['key_hex_bytes'],
                           version_bytes=case['version_bytes'], **result)
                report['cases'].append(row)
                save()
                assert result['exit_code'] == case['exit_code']
                assert result['actual'] == case['actual']
                if result['exit_code']:
                    assert result['exit_code'] == 101 and 'not a char boundary' in result['stderr']
    else:
        verify_record(BOUNDARIES)
        boundaries = json.loads(gzip.decompress(BOUNDARIES.read_bytes()))
        assert boundaries['complete'] and boundaries['source_inputs'] == inputs
        assert len(original['cases']) == 27 and len(original['source_environment_errors']) == 18
        assert len(boundaries['cases']) == 21
        for category, cases in [('successful_runtime', original['cases']),
                                ('startup_errors', original['source_environment_errors']),
                                ('additional_boundaries', boundaries['cases'])]:
            for case in cases:
                if category == 'additional_boundaries':
                    key = bytes.fromhex(case['key_hex_bytes']) if case['key_hex_bytes'] is not None else None
                    version = bytes.fromhex(case['version_bytes']) if case['version_bytes'] is not None else None
                    expected = case['actual']
                else:
                    key = case.get('key_hex', bytes(range(32)).hex())
                    version = case.get('key_version', str(case['input']['key_version']))
                    expected = case['source_expected']
                result = run(executable, case['input'], key, version)
                row = dict(category=category, source=case, cpp=result)
                report['cases'].append(row)
                save()
                if category == 'additional_boundaries' and case['exit_code']:
                    assert result['exit_code'] == 1 and 'UTF-8 character boundary' in result['stderr'], row
                    row['disposition'] = 'Rust panic/C++ out_of_range interrupt startup; distinct exits and diagnostics retained without normalization.'
                else:
                    assert result['exit_code'] == 0 and result['actual'] == expected, row
                    row['disposition'] = 'Complete original JSON outcome matches.'
                save()
        verify_record(FILESYSTEM)
        filesystem = json.loads(gzip.decompress(FILESYSTEM.read_bytes()))
        assert filesystem['complete'] and filesystem['source_inputs'] == inputs
        assert len(filesystem['cases']) == 2
        for case in filesystem['cases']:
            result = filesystem_run(executable, case['key_hex'], case['key_version'])
            row = dict(category='filesystem_before_key_failure', source=case, cpp=result)
            report['cases'].append(row)
            save()
            assert result['exit_code'] == 0 and result['actual'] == case['actual'], row
            assert result['filesystem'] == case['filesystem'] and result['tables'] == case['tables'], row
            row['disposition'] = 'Full startup error, created relative paths/file sizes and absent schema match.'
            save()
    report['complete'] = True
    save()
    print(f'Verified {len(report["cases"])} complete startup scenarios; '
          + ('original observations recorded.' if args.observe_original else
             '27 success/18 failure/21 boundary/two filesystem scenarios, including three explicit panic/exception dispositions.'))


if __name__ == '__main__':
    main()
