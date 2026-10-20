#!/usr/bin/env python3
"""Record isolated calls to the unchanged generated Rust runtime configuration.

These are source observations, not evidence of C++ parity. The subprocess owns
its fixture environment and temporary filesystem; no production configuration
or runtime algorithms are reimplemented here.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
ENV_NAME = b'CPP_DYNAMIC_JSON_CORE_DATABASE_URL'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--check', action='store_true',
                        help='Replay without changing the retained original observations')
    args = parser.parse_args()
    domain = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((domain / 'PROVENANCE.json').read_text())
    assert provenance['generator_commit'] == '45e1572c4956ca963410d8ea436ead3c8857087a'
    assert provenance['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert provenance['hand_edited'] is False
    for path, digest in provenance['files'].items():
        assert hashlib.sha256((domain / path).read_bytes()).hexdigest() == digest, path
    assert hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']
    project = ROOT / 'tests/parity/rust_execution_shapes_5_0_6'
    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
    original_versions = versions(reference)
    for name, resolved in versions(project / 'Cargo.lock').items():
        if name not in {'teaql_execution_shapes_506', 'cpp-dynamic-json-core'}:
            assert resolved <= original_versions.get(name, set()), name
    cases = [
        ('config_missing', 'config', None),
        ('config_empty', 'config', b''),
        ('config_blank', 'config', b'   '),
        ('config_memory', 'config', b':memory:'),
        ('config_sqlite_url', 'config', b'sqlite://folder/data.db?mode=ro'),
        ('config_unicode', 'config', 'folder/\u6570\u636e.db'.encode()),
        ('config_invalid_unicode', 'config', b'\xff'),
        ('connect_missing', 'connect', None),
        ('connect_invalid_unicode', 'connect', b'\xff'),
        ('connect_memory', 'connect', b':memory:'),
        ('connect_sqlite_memory', 'connect', b'sqlite::memory:'),
        ('connect_two_slash_memory', 'connect', b'sqlite://:memory:'),
        ('connect_three_slash_memory', 'connect', b'sqlite:///:memory:'),
        ('connect_empty', 'connect', b''),
        ('connect_query_only', 'connect', b'?mode=memory'),
        ('connect_plain_file', 'connect', b'folder/data.db'),
        ('connect_url_query_ignored', 'connect', b'sqlite://folder/data.db?mode=ro'),
        ('connect_literal_percent', 'connect', b'sqlite://db%20name.db?mode=ro'),
        ('connect_file_uri', 'connect', b'file:memorydb?mode=memory&cache=shared'),
        ('connect_parent_failure', 'connect', b'blocked/data.db'),
    ]
    observations = []
    for name, mode, value in cases:
        with tempfile.TemporaryDirectory(prefix='teaql_configuration_original_') as directory:
            work = Path(directory)
            (work / 'blocked').write_text('A regular file, not a directory.\n')
            environment = dict(os.environb)
            environment.pop(ENV_NAME, None)
            environment[b'TEAQL_LOG_ENDPOINT'] = b'off'
            if value is not None:
                environment[ENV_NAME] = value
            run = subprocess.run([str(args.rust.resolve()), mode], cwd=work,
                                 env=environment, capture_output=True, check=True)
            result = json.loads(run.stdout)
            files = sorted(str(path.relative_to(work)) for path in work.rglob('*')
                           if path.is_file() and path.name != 'blocked')
            observations.append({'name': name, 'mode': mode,
                                 'environment_hex': value.hex() if value is not None else None,
                                 'source_expected': result, 'created_files': files})
    paths = [
        'tests/generator/parity/dynamic_json_rust/lib/src/runtime.rs',
        'tests/generator/parity/dynamic_json_rust/PROVENANCE.json',
        'tests/parity/rust_execution_shapes_5_0_6/Cargo.lock',
        'tests/parity/rust_execution_shapes_5_0_6/src/bin/runtime_configuration_original_5_0_6.rs',
    ]
    hashes = {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
              for path in paths}
    record = {'schema': 1, 'status': 'original_observations_cpp_comparison_required',
              'runtime_commit': json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
              'generator_commit': '45e1572c4956ca963410d8ea436ead3c8857087a',
              'source_inputs': hashes, 'cases': observations,
              'notice': 'Calls unchanged original public functions. No C++ outcome or full G12 acceptance is claimed.'}
    if args.check:
        assert json.loads(args.output.read_text()) == record, 'Original configuration observations changed'
        print(f'Verified {len(observations)} retained original runtime configuration observations')
    else:
        assert not args.output.exists(), 'Preserve existing original observations'
        args.output.write_text(json.dumps(record, indent=2, ensure_ascii=True) + '\n')
        print(f'Recorded {len(observations)} isolated original runtime configuration observations')


if __name__ == '__main__':
    main()
