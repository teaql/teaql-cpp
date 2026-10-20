#!/usr/bin/env python3
"""Verify complete original commit-audit tests and private helper observations.

A disposable copy retains every original runtime source/test body. Only the
observer module is appended. Optional external-exporter test dependencies are
omitted with their feature disabled; core runtime dependencies remain authentic.
Build the native and optional module C++ probes before running this helper.
"""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--rust-target', type=Path,
                        default=Path('/tmp/teaql_cpp_state_baselines_506'))
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error(f'Record already exists: {args.record}')
    record = {'complete': False, 'steps': [], 'full_goal_accepted': False}

    def save():
        if args.record:
            args.record.parent.mkdir(parents=True, exist_ok=True)
            data = (json.dumps(record, indent=2) + '\n').encode()
            args.record.write_bytes(gzip.compress(data, mtime=0)
                                    if args.record.suffix == '.gz' else data)

    environment = dict(os.environ, CARGO_TARGET_DIR=str(args.rust_target.resolve()),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')

    def run(command, input_text=None, timeout=600):
        command = [str(part) for part in command]
        start = time.monotonic()
        result = subprocess.run(command, cwd=ROOT, env=environment, input=input_text,
                                text=True, capture_output=True, timeout=timeout)
        record['steps'].append({'command': command, 'exit_code': result.returncode,
                                'elapsed_seconds': time.monotonic() - start,
                                'stdout': result.stdout, 'stderr': result.stderr})
        save()
        result.check_returncode()
        return result.stdout

    run([sys.executable, ROOT / 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    corpus_path = PARITY / 'commit_audit_source_5_0_6_cases.json.gz'
    corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
    source = ROOT / '.reference/teaql-rs/teaql-runtime'
    observer = PARITY / 'commit_audit_source_observer_5_0_6.rs'
    if corpus['baseline'] != baseline or len(corpus['cases']) != 99:
        raise RuntimeError('Unexpected source baseline or corpus geometry')
    if digest(source / 'src/commit_audit.rs') != corpus['source_sha256']:
        raise RuntimeError('Original complete commit-audit source changed')
    if digest(observer) != corpus['observer_sha256']:
        raise RuntimeError('Private-source observer changed')
    frontends = [('native', args.build_dir.resolve() / 'teaql_commit_audit_source_probe')]
    if args.module_build_dir:
        frontends.append(('modules', args.module_build_dir.resolve() /
                          'modules/teaql_module_commit_audit_source_probe'))
    for _, binary in frontends:
        if not binary.is_file():
            raise FileNotFoundError(f'Build the probe first: {binary}')
    metadata = json.loads(run(['cargo', 'metadata', '--offline', '--locked', '--no-deps',
                              '--format-version', '1', '--manifest-path', source / 'Cargo.toml']))
    package = next(row for row in metadata['packages']
                   if Path(row['manifest_path']) == source / 'Cargo.toml')
    with tempfile.TemporaryDirectory(prefix='teaql_commit_audit_source_506_') as directory:
        project = Path(directory)
        originals = {}
        for name in ('src', 'tests', 'test-vectors'):
            shutil.copytree(source / name, project / name)
            for path in (source / name).rglob('*'):
                if path.is_file():
                    relative = path.relative_to(source).as_posix()
                    originals[relative] = digest(path)
                    if digest(project / relative) != originals[relative]:
                        raise RuntimeError(f'Copied source differs: {relative}')
        original_commit_audit = (project / 'src/commit_audit.rs').read_bytes()
        (project / 'src/commit_audit.rs').write_bytes(original_commit_audit + b'\n' + observer.read_bytes())
        manifest = '[package]\nname="teaql-runtime"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
        for kind, heading in ((None, 'dependencies'), ('dev', 'dev-dependencies')):
            manifest += '\n[' + heading + ']\n'
            for dependency in package['dependencies']:
                if dependency['kind'] != kind:
                    continue
                if kind == 'dev' and dependency['name'] != 'teaql-macros':
                    continue
                fields = ['version=' + json.dumps(dependency['req'])]
                for key in ('path', 'features', 'optional'):
                    if dependency.get(key):
                        fields.append(key + '=' + json.dumps(dependency[key]))
                if not dependency.get('uses_default_features', True):
                    fields.append('default-features=false')
                manifest += dependency['name'] + '={' + ','.join(fields) + '}\n'
        manifest += '\n[features]\nopentelemetry=["dep:opentelemetry"]\n'
        (project / 'Cargo.toml').write_text(manifest)
        reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
        shutil.copyfile(reference_lock, project / 'Cargo.lock')
        run(['cargo', 'metadata', '--offline', '--format-version', '1',
             '--manifest-path', project / 'Cargo.toml'])
        if not registry_identities(project / 'Cargo.lock') <= registry_identities(reference_lock):
            raise RuntimeError('Private-source reference dependencies differ')
        reference_versions = versions(reference_lock)
        for name, resolved in versions(project / 'Cargo.lock').items():
            if not resolved <= reference_versions.get(name, set()):
                raise RuntimeError(f'Private-source package version differs: {name}')
        inputs = [row['input'] for row in corpus['cases']]
        (project / 'inputs.json').write_text(json.dumps(inputs))
        environment.update(TEAQL_COMMIT_AUDIT_INPUTS=str(project / 'inputs.json'),
                           TEAQL_COMMIT_AUDIT_OUTPUTS=str(project / 'outputs.json'))
        output = run(['cargo', 'test', '--offline', '--locked', '--jobs', '2',
                      '--manifest-path', project / 'Cargo.toml', '--lib', 'commit_audit::',
                      '--', '--nocapture', '--test-threads=1'], timeout=1200)
        if '6 passed; 0 failed' not in output:
            raise RuntimeError('Five original tests and the observer did not all pass')
        original = json.loads((project / 'outputs.json').read_text())
        if not same(original, [row['expected'] for row in corpus['cases']]):
            raise RuntimeError('Actual original outcomes differ from retained observations')
        for name, value in originals.items():
            content = (project / name).read_bytes()
            if name == 'src/commit_audit.rs':
                if content != original_commit_audit + b'\n' + observer.read_bytes():
                    raise RuntimeError('Original source prefix or appended observer changed')
            elif hashlib.sha256(content).hexdigest() != value:
                raise RuntimeError(f'Original source/test body changed: {name}')
        record.update(baseline=baseline, source_files_sha256=originals,
                      observer=observer.read_text(), manifest=manifest,
                      lock=(project / 'Cargo.lock').read_text(),
                      original_tests=5, added_observer_tests=1, original=original,
                      corpus_sha256=digest(corpus_path), verifier_sha256=digest(Path(__file__)),
                      frontends={})
        for label, binary in frontends:
            output = run([binary], ''.join(json.dumps(value, ensure_ascii=True) + '\n'
                                          for value in inputs), timeout=180)
            if not output.endswith('\n'):
                raise RuntimeError(f'Missing final LF: {binary}')
            actual = [json.loads(line) for line in output.split('\n')[:-1]]
            if not same(original, actual):
                raise RuntimeError(f'Complete source outcomes differ: {label}')
            record['frontends'][label] = {'binary_sha256': digest(binary), 'outcomes': actual}
            print(f'Commit audit {label}: 99/99 exact source outcomes', flush=True)
        record.update(complete=True, observations=99)
        save()
    print('All five original commit-audit tests and exact retained observations pass.')


if __name__ == '__main__':
    main()
