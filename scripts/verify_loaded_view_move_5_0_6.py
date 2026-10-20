#!/usr/bin/env python3
"""Compare consuming loaded-view transfer against the actual fixed Rust runtime."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_loaded_view_move_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from loaded_view_move_inputs_5_0_6 import cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--witness', action='store_true', help='Record original/native differences before repair')
    parser.add_argument('--modules', action='store_true')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--bootstrap-corpus', action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, whole_entity_runtime_accepted=False,
                  scope='Consuming loaded-view transfer only', steps=[], profiles={})
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off',
               ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))

    def run(command, payload=None):
        command = [str(part) for part in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                                    stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    inputs = cases()
    assert len(inputs) == 512
    payload = ''.join(json.dumps(row)+'\n' for row in inputs)

    def observe(binary):
        result = [json.loads(line) for line in run([binary], payload).splitlines()]
        assert len(result) == len(inputs)
        return result

    run(['python3', 'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    with tempfile.TemporaryDirectory(prefix='teaql_loaded_view_move_506_') as temporary:
        directory = Path(temporary)
        view = ReferenceRuntimeView(ROOT, directory/'runtime',
                                    {'src/entity_runtime.rs': PARITY/'loaded_view_move_source_5_0_6.rs'}, run)
        project = directory/'observer'
        project.mkdir()

        def absolute_path(match):
            target = (PROJECT/match[1]).resolve()
            if target == ROOT/'.reference/teaql-rs/teaql-runtime':
                target = view.directory
            return 'path = '+json.dumps(str(target))

        manifest = re.sub(r'path\s*=\s*"([^"]+)"', absolute_path, (PROJECT/'Cargo.toml').read_text())
        (project/'Cargo.toml').write_text(manifest)
        shutil.copyfile(PROJECT/'Cargo.lock', project/'Cargo.lock')
        run(['cargo', 'build', '--offline', '--locked', '--manifest-path', project/'Cargo.toml'])
        binary = Path(env['CARGO_TARGET_DIR'])/'debug/loaded_view_move_source_506'
        original = observe(binary)
        assert same(observe(binary), original), 'Original observations must reproduce'
        record['original_runtime_view'] = view.provenance()
        record['original_binary_sha256'] = digest(binary)
        record['observer_manifest'] = manifest
        record['observer_lock_sha256'] = digest(project/'Cargo.lock')
    corpus_path = PARITY/'loaded_view_move_5_0_6_cases.json.gz'
    corpus = dict(baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
                  driver='tests/parity/loaded_view_move_driver_5_0_6.rs',
                  driver_sha256=digest(PARITY/'loaded_view_move_driver_5_0_6.rs'),
                  observer='tests/parity/loaded_view_move_source_5_0_6.rs',
                  observer_sha256=digest(PARITY/'loaded_view_move_source_5_0_6.rs'),
                  cases=[dict(input=value, expected=output) for value, output in zip(inputs, original)])
    if corpus_path.exists():
        assert same(json.loads(gzip.decompress(corpus_path.read_bytes())), corpus)
    for profile in ['native']+(['modules_native'] if args.modules else [])+(['sanitize'] if args.sanitize else []):
        modules = profile == 'modules_native'
        build = ROOT/('build_modules_native' if modules else 'build_sanitize_current' if profile == 'sanitize'
                      else 'build_native_current')
        target = ('teaql_module_' if modules else 'teaql_')+'loaded_view_move_probe'
        run(['cmake', '--preset', profile])
        run(['cmake', '--build', build, '--target', target, '--parallel', '4'])
        binary = build/('modules/'+target if modules else target)
        outputs = observe(binary)
        gaps = [dict(index=i, input=inputs[i], original=a, actual=b)
                for i, (a, b) in enumerate(zip(original, outputs)) if not same(a, b)]
        record['profiles'][profile] = dict(binary_sha256=digest(binary), differences=gaps,
                                            observations=len(outputs))
        save()
        if args.witness:
            assert gaps, 'Expected a real original/native difference before repair'
        else:
            assert not gaps, (profile, len(gaps))
            run([binary, '--self-test'])
        assert 'no work to do' in run(['cmake', '--build', build, '--target', target, '--parallel', '4'])
        print(profile, len(inputs), 'observations;', len(gaps), 'differences', flush=True)
    if args.bootstrap_corpus and not args.witness and not corpus_path.exists():
        corpus_path.write_bytes(gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    record.update(complete=True, normalization='none', observations_per_frontend=len(inputs),
                  original_outcomes=corpus, controller_sha256=digest(Path(__file__).resolve()))
    save()


if __name__ == '__main__':
    main()
