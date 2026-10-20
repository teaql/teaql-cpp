#!/usr/bin/env python3
"""Replay retained Entity/Eval corpora before refreshing a manifest-only anchor.

The sole manifest change adds the independent metadata observer. No expected
outcome, observer body, dependency lock or Rust production source may change.
"""
import argparse
from contextlib import redirect_stdout
import base64
import gzip
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
PROJECT = ROOT / 'tests/parity/rust_core_primitives_reference_5_0_6'
MANIFEST = PROJECT / 'Cargo.toml'
OLD_MANIFEST = '3bbddd629ee449e2084c56919dc540626fb56bc29452212508a2d5271eb796e0'
METADATA_BIN = (b'\n[[bin]]\nname = "meta_api_reference_5_0_6"\n'
                b'path = "../rust_field_layout/src/bin/meta_api_5_0_6.rs"\n')
MATRICES = [('entity_source', 'entity_source_reference_5_0_6', 'teaql_entity_source_probe'),
            ('eval_indexed', 'eval_indexed_reference_5_0_6', 'teaql_eval_indexed_probe')]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh evidence path')
    record = dict(complete=False, complete_generator_goal=False, steps=[], matrices={})

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2) + '\n').encode(), mtime=0))

    def run(command, environment=None):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                text=True, capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command],
                                    exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr))
        save()
        result.check_returncode()

    run([sys.executable, 'scripts/verify_upstream.py'])
    manifest = MANIFEST.read_bytes()
    assert manifest.endswith(METADATA_BIN)
    assert sha(manifest[:-len(METADATA_BIN)]) == OLD_MANIFEST
    relative = str(MANIFEST.relative_to(ROOT))
    current_manifest = sha(manifest)
    record['manifest_change'] = dict(path=relative, before_sha256=OLD_MANIFEST,
                                     after_sha256=current_manifest,
                                     appended_bytes=METADATA_BIN.decode())
    identities = registry_identities(PROJECT / 'Cargo.lock')
    assert identities <= registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    record['exact_reference_registry_identities'] = len(identities)
    frozen = {}
    pending = []
    with tempfile.TemporaryDirectory(prefix='teaql_core_manifest_506_') as directory:
        temporary = Path(directory)
        for stem, rust, cpp in MATRICES:
            path = ROOT / f'tests/parity/{stem}_5_0_6_cases.json.gz'
            before = path.read_bytes()
            data = json.loads(gzip.decompress(before))
            assert data['source_hashes'][relative] in (OLD_MANIFEST, current_manifest)
            for source, digest in data['source_hashes'].items():
                actual = (ROOT / source).read_bytes()
                frozen[source] = sha(actual)
                if source != relative:
                    assert sha(actual) == digest, source
            candidate = json.loads(json.dumps(data))
            candidate['source_hashes'][relative] = current_manifest
            candidate_path = temporary / path.name
            candidate_bytes = gzip.compress((json.dumps(candidate, indent=2) + '\n').encode(), mtime=0)
            candidate_path.write_bytes(candidate_bytes)
            record['matrices'][stem] = dict(before_sha256=sha(before),
                before_gzip_base64=base64.b64encode(before).decode(),
                candidate_sha256=sha(candidate_bytes), inputs=len(data['inputs']),
                expected_outcomes_unchanged=True, replay=[])
            pending.append((path, before, candidate_bytes, candidate_path, stem, rust, cpp))
        environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
            CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', MANIFEST,
             *[argument for _, rust, _ in MATRICES for argument in ['--bin', rust]]], environment)
        for preset, build in [('native', 'build_native_current'), ('modules_native', 'build_modules_native')]:
            prefix = 'teaql_module_' if preset == 'modules_native' else 'teaql_'
            run(['cmake', '--build', '--preset', preset, '--target',
                 *[cpp.replace('teaql_', prefix, 1) for _, _, cpp in MATRICES]])
            for path, before, candidate_bytes, candidate_path, stem, rust, cpp in pending:
                comparator = ROOT / f'tests/parity/compare_{stem}_5_0_6.py'
                spec = importlib.util.spec_from_file_location(f'teaql_{stem}_requalification', comparator)
                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                module.CORPUS = candidate_path
                receipt = temporary / f'{stem}_{preset}.json.gz'
                binary_directory = ROOT / build / 'modules' if preset == 'modules_native' else ROOT / build
                command = [str(comparator), '--cpp', str(binary_directory / cpp.replace('teaql_', prefix, 1)),
                           '--rust', str(Path(environment['CARGO_TARGET_DIR']) / 'debug' / rust),
                           '--record', str(receipt)]
                old_argv = sys.argv
                output = io.StringIO()
                try:
                    sys.argv = command
                    with redirect_stdout(output):
                        module.main()
                finally:
                    sys.argv = old_argv
                evidence = json.loads(gzip.decompress(receipt.read_bytes()))
                assert evidence['complete'] and evidence['actual_original_replayed']
                record['matrices'][stem]['replay'].append(dict(profile=preset,
                    comparator_sha256=sha(comparator.read_bytes()), output=output.getvalue(),
                    result=evidence))
                save()
        assert all(sha((ROOT / name).read_bytes()) == digest for name, digest in frozen.items())
        assert all(path.read_bytes() == before for path, before, *_ in pending)
        for path, before, candidate_bytes, *_ in pending:
            if json.loads(gzip.decompress(before)) != json.loads(gzip.decompress(candidate_bytes)):
                path.write_bytes(candidate_bytes)
    record.update(complete=True, only_manifest_anchors_refreshed=True,
                  inputs_expected_and_other_source_hashes_unchanged=True,
                  whole_native_epoch_requalified=False)
    save()
    print('Entity 230 and Eval 18/180: original/native/imported match; manifest anchors refreshed.')


if __name__ == '__main__':
    main()
