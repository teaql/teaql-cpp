#!/usr/bin/env python3
"""Compare original runtime equality without changing payloads or normalizing NaNs."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_entity_runtime_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same
from entity_runtime_equality_inputs_5_0_6 import cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--witness', action='store_true')
    parser.add_argument('--modules', action='store_true')
    parser.add_argument('--bootstrap-corpus', action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False, full_goal_accepted=False, steps=[], profiles={})
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(), mtime=0))

    def run(command, payload=None):
        result = subprocess.run([str(v) for v in command], cwd=ROOT, env=env,
                                input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=[str(v) for v in command], exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    inputs = cases()
    assert len(inputs) == 338
    payload = ''.join(json.dumps(v)+'\n' for v in inputs)
    run(['python3', 'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT/'Cargo.toml',
         '--bin', 'entity_runtime_source_506'])
    original_binary = Path(env['CARGO_TARGET_DIR'])/'debug/entity_runtime_source_506'
    original = [json.loads(line) for line in run([original_binary], payload).splitlines()]
    assert len(original) == len(inputs)
    assert same(original, [json.loads(line) for line in run([original_binary], payload).splitlines()])
    corpus = dict(baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
                  driver='tests/parity/entity_runtime_equality_source_5_0_6.rs',
                  driver_sha256=hashlib.sha256((PARITY/'entity_runtime_equality_source_5_0_6.rs').read_bytes()).hexdigest(),
                  cases=[dict(input=value, expected=output) for value, output in zip(inputs, original)])
    for profile, build in [('native', 'build_native_current')]+([('modules_native', 'build_modules_native')] if args.modules else []):
        module = profile == 'modules_native'
        target = ('teaql_module_' if module else 'teaql_')+'entity_runtime_equality_probe'
        run(['cmake', '--preset', profile])
        run(['cmake', '--build', build, '--target', target, '--parallel', '4'])
        binary = ROOT/build/('modules/'+target if module else target)
        output = [json.loads(line) for line in run([binary], payload).splitlines()]
        assert len(output) == len(inputs)
        gaps = [dict(index=i, input=inputs[i], original=a, actual=b)
                for i, (a, b) in enumerate(zip(original, output)) if not same(a, b)]
        record['profiles'][profile] = dict(cases=len(output), differences=gaps)
        save()
        assert bool(gaps) == args.witness, (profile, len(gaps))
        print(profile, len(inputs), 'observations;', len(gaps), 'differences', flush=True)
    path = PARITY/'entity_runtime_equality_5_0_6_cases.json.gz'
    if args.bootstrap_corpus and not args.witness:
        assert not path.exists()
        path.write_bytes(gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    elif path.exists():
        assert same(json.loads(gzip.decompress(path.read_bytes())), corpus)
    record.update(complete=True, normalization='none', original=corpus)
    save()


if __name__ == '__main__':
    main()
