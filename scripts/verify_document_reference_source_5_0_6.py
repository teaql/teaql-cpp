#!/usr/bin/env python3
"""Replay unchanged reference corpora with exact frozen dependency identities."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--matrix', action='append', choices=['reference_model', 'round_trip_reference', 'entity_reference'])
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve prior evidence'
    record = dict(scope='Selected whole unchanged 229-model, 933-reference and 548-codec corpora against actual '
                        'frozen source, exact dependencies and current C++; no normalization. '
                        'Historical corpora and producers are not rewritten.',
                  selected_matrices=args.matrix, steps=[], matrices=[], complete=False)

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    reference = ROOT / '.reference/teaql-rs/Cargo.lock'
    command = ['python3', 'scripts/verify_upstream.py']
    result = subprocess.run(command, cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    record['steps'].append(dict(command=command, exit_code=result.returncode, output=result.stdout))
    save()
    result.check_returncode()
    expected_versions = versions(reference)
    matrices = [
        ('reference_model', 'reference_model_5_0_6_cases.json.gz', 229,
         'rust_reference_models_reference_5_0_6', 'teaql_reference_models_reference_506',
         '/tmp/teaql_cpp_inherited_business_binding_bridge_506', 'reference_models_reference_506',
         'rust_field_layout/src/bin/reference_model_5_0_6.rs'),
        ('round_trip_reference', 'round_trip_reference_baseline_5_0_6_cases.json.gz', 933,
         'rust_final_baselines_5_0_6', 'teaql_final_baselines_506',
         '/tmp/teaql_cpp_final_baselines_506', 'round_trip_reference_oracle',
         'rust_values/src/bin/round_trip_reference_oracle.rs'),
        ('entity_reference', 'entity_reference_baseline_5_0_6_cases.json.gz', 548,
         'rust_final_baselines_5_0_6', 'teaql_final_baselines_506',
         '/tmp/teaql_cpp_final_baselines_506', 'entity_reference_oracle',
         'rust_values/src/bin/entity_reference_oracle.rs')]
    paths = [Path(__file__), reference]
    for stem, corpus_name, count, directory, package, target, binary, driver in matrices:
        if args.matrix and stem not in args.matrix:
            continue
        project = PARITY / directory
        for name, actual in versions(project / 'Cargo.lock').items():
            if name != package:
                assert actual <= expected_versions.get(name, set()), (name, actual)
        assert registry_identities(project / 'Cargo.lock') <= registry_identities(reference)
        environment = dict(os.environ, CARGO_TARGET_DIR=target,
                           CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                           TEAQL_LOG_ENDPOINT='off')
        command = ['cargo', 'build', '--jobs', '2', '--offline', '--locked', '--manifest-path',
                   str(project / 'Cargo.toml'), '--bin', binary]
        result = subprocess.run(command, cwd=ROOT, env=environment, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        record['steps'].append(dict(command=command, exit_code=result.returncode, output=result.stdout))
        save()
        result.check_returncode()
        corpus_path = PARITY / corpus_name
        corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
        assert corpus['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        cases = corpus['cases']
        assert len(cases) == count
        matrix = dict(name=stem, inputs=[c['input'] for c in cases],
                      expected=[c['expected'] for c in cases],
                      registry_versions_preserved=True, registry_checksums_preserved=True)
        record['matrices'].append(matrix)
        payload = ''.join(json.dumps(case['input'], ensure_ascii=True) + '\n' for case in cases)
        rust = Path(target) / 'debug' / binary
        cpp = args.build_dir.resolve() / f'teaql_{stem}_probe'
        for key, executable in [('original', rust), ('cpp', cpp)]:
            result = subprocess.run([str(executable)], input=payload, env=environment,
                                    text=True, capture_output=True)
            matrix[key + '_process'] = dict(exit_code=result.returncode, stdout=result.stdout,
                                            stderr=result.stderr)
            save()
            result.check_returncode()
            matrix[key] = [json.loads(line) for line in result.stdout.split('\n') if line]
            assert len(matrix[key]) == count
        matrix['original_differences'] = [i for i, (a, b) in enumerate(zip(matrix['expected'], matrix['original'])) if not same(a, b)]
        matrix['cpp_differences'] = [i for i, (a, b) in enumerate(zip(matrix['original'], matrix['cpp'])) if not same(a, b)]
        save()
        assert not matrix['original_differences'] and not matrix['cpp_differences'], stem
        paths += [project / 'Cargo.toml', project / 'Cargo.lock', corpus_path, PARITY / driver, rust, cpp]
        print(f'{stem}: {count}/{count} complete source/native outcomes', flush=True)
    for name in ['round_trip_reference', 'document_round_trip', 'entity_reference']:
        paths.append(ROOT / f'.reference/teaql-rs/teaql-runtime/src/{name}.rs')
    paths += [ROOT / name for name in [
        'teaql_runtime/include/teaql/runtime/reference_model_debug.hpp',
        'teaql_runtime/src/round_trip_reference.cpp',
        'teaql_runtime/src/round_trip_reference_types.cpp',
        'teaql_runtime/src/document_snapshot_codec.cpp']]
    record['hashes'] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    record['complete'] = True
    save()


if __name__ == '__main__':
    main()
