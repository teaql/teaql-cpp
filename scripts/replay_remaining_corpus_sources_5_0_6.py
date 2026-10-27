#!/usr/bin/env python3
"""Fresh original/native replay of the seven remaining baseline-only source corpora."""
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

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import json_lines, same
from corpus_input_contracts_5_0_6 import generate
from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    registry_path = ROOT / 'tests/parity/remaining_corpus_source_contracts_5_0_6.json'
    registry = json.loads(registry_path.read_text())
    contracts = registry['contracts']
    assert len(contracts) == 7 and sum(c['count'] for c in contracts) == 6281
    assert registry['runtime_commit'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert registry['generator_commit'] == json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']
    names = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z']).split(b'\0')
    paths = {ROOT / os.fsdecode(n) for n in names if n and Path(os.fsdecode(n)).suffix in ('.cpp', '.hpp', '.ixx', '.cppm', '.rs')}
    paths.update([Path(__file__), registry_path, ROOT / 'scripts/corpus_input_contracts_5_0_6.py',
                  ROOT / 'scripts/reference_runtime_view_5_0_6.py', ROOT / 'tests/parity/compare_values.py',
                  ROOT / 'upstream.lock.json', ROOT / 'toolchains.lock.json', ROOT / 'CMakePresets.json',
                  ROOT / 'CMakeLists.txt', ROOT / 'tests/CMakeLists.txt'])
    paths.update(ROOT / c[key] for c in contracts for key in ('source', 'corpus'))
    paths.update(ROOT / c['input_builder']['path'] for c in contracts)
    projects = sorted({c['project'] for c in contracts if c['project']})
    paths.update(ROOT / p for project in projects for p in (project, str(Path(project).with_name('Cargo.lock'))))
    hashes = lambda: {str(p): digest(p) for p in sorted(paths) if p.is_file()}
    reference = ROOT / '.reference/teaql-rs'
    reference_names = subprocess.check_output(['git', '-C', str(reference), 'ls-files', '-z']).split(b'\0')
    reference_files = {os.fsdecode(n): digest(reference / os.fsdecode(n)) for n in reference_names
                       if n and (os.fsdecode(n).endswith('.rs') or Path(os.fsdecode(n)).name in ('Cargo.toml', 'Cargo.lock'))}
    record = dict(complete=False, whole_goal_complete=False, scope=__doc__, registry=registry,
                  steps=[], contracts=[], projects={}, reference_source_files=reference_files,
                  input_hashes_before=hashes(), free_bytes_before=shutil.disk_usage(ROOT).free,
                  native_directory=str(ROOT / 'build_native_current'))
    environment = dict(os.environ, TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1',
                       CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0', CARGO_PROFILE_TEST_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    def run(command, payload=None):
        command = [str(v) for v in command]
        result = subprocess.run(command, cwd=ROOT, text=True, input=payload, capture_output=True, env=environment)
        index = len(record['steps'])
        record['steps'].append(dict(command=command, exit_code=result.returncode, stdout=result.stdout,
                                    stderr=result.stderr, stdin_sha256=None if payload is None else hashlib.sha256(payload.encode()).hexdigest(),
                                    stdin_bytes=None if payload is None else len(payload.encode())))
        save()
        print(' '.join(command[:5]), 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout, index

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        run(['cargo', '--version'])
        run(['rustc', '-vV'])
        run(['cmake', '--build', '--preset', 'native', '--parallel', '2', '--target',
             *dict.fromkeys(c['native_target'] for c in contracts)])
        generated = {}
        for c in contracts:
            inputs, proof = generate(c)
            corpus = json.loads(gzip.decompress((ROOT / c['corpus']).read_bytes()))
            assert corpus['baseline'] == registry['runtime_commit']
            assert same(inputs, [case['input'] for case in corpus['cases']])
            assert digest(ROOT / c['source']) == c['source_sha256']
            generated[c['name']] = (inputs, proof, [case['expected'] for case in corpus['cases']])
        with tempfile.TemporaryDirectory(prefix='teaql_remaining_corpus_source_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = str(owned)
            environment['CARGO_TARGET_DIR'] = str(owned / 'target')
            record['cargo_environment'] = {k: environment[k] for k in environment if k.startswith('CARGO_')}
            support_inputs = owned / 'support_inputs.json'
            support_output = owned / 'support_outputs.json'
            support_inputs.write_text(json.dumps(generated['generated_support_source'][0]))
            environment.update(TEAQL_SUPPORT_SOURCE_INPUTS=str(support_inputs), TEAQL_SUPPORT_SOURCE_OUTPUTS=str(support_output))
            support = owned / 'support_observer.rs'
            support.write_text('#[cfg(test)]\nmod generated_support_source_contracts_506 {\n' +
                               (ROOT / 'tests/parity/query_full_state_5_0_6.rs').read_text() + '\n' +
                               (ROOT / 'tests/parity/generated_support_source_5_0_6.rs').read_text() + '\n}\n')
            privacy_inputs = owned / 'inputs.json'
            privacy_inputs.write_text(json.dumps(generated['mutation_intent_privacy'][0]) + '\n')
            privacy = owned / 'privacy_observer.rs'
            privacy_template = (ROOT / 'tests/parity/mutation_intent_privacy_source_5_0_6.rs').read_text()
            assert privacy_template.count('__TEAQL_INTENT_PRIVACY_INPUTS__') == 1
            privacy.write_text(privacy_template.replace('__TEAQL_INTENT_PRIVACY_INPUTS__', str(privacy_inputs)))

            def view_run(command):
                return run(command)[0]

            view = ReferenceRuntimeView(ROOT, owned / 'runtime', {
                'src/generated_support.rs': support, 'src/data_service/mod.rs': privacy}, view_run)
            record['runtime_view'] = view.provenance()
            save()
            outputs, step = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                                 view.directory / 'Cargo.toml', '-j2', '--lib',
                                 'generated_support_source_contracts_506::', '--', '--nocapture', '--test-threads=1'])
            selected = re.findall(r'^test (\S+) \.\.\.', outputs, re.M)
            assert set(selected) == {'generated_support::generated_support_source_contracts_506::observe_private_sources',
                                     'generated_support::generated_support_source_contracts_506::source_interface_contracts'}
            assert '2 passed; 0 failed' in outputs
            support_text = support_output.read_text()
            observations = json.loads(support_text)
            originals = {'generated_support_source': dict(step=step, output_kind='file', raw_output=support_text,
                output_sha256=digest(support_output), tests=selected, observations=observations)}
            outputs, step = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                                 view.directory / 'Cargo.toml', '-j2', '--lib',
                                 'data_service::cpp_mutation_intent_privacy_observer::', '--', '--nocapture', '--test-threads=1'])
            selected = re.findall(r'^test (\S+) \.\.\.', outputs, re.M)
            assert selected == ['data_service::cpp_mutation_intent_privacy_observer::replacing_intent_preserves_existing_privacy']
            assert '1 passed; 0 failed' in outputs
            observations = [json.loads(line.split('TEAQL_INTENT_PRIVACY_JSON ', 1)[1]) for line in outputs.split('\n')
                            if 'TEAQL_INTENT_PRIVACY_JSON ' in line]
            originals['mutation_intent_privacy'] = dict(step=step, output_kind='marker_stdout', tests=selected,
                                                        prefix='TEAQL_INTENT_PRIVACY_JSON ', observations=observations)
            view.verify()
            record['runtime_view'] = view.provenance()
            for project in projects:
                manifest = ROOT / project
                lock = manifest.with_name('Cargo.lock')
                reference_lock = reference / 'Cargo.lock'
                assert registry_identities(lock) <= registry_identities(reference_lock)
                known = versions(reference_lock)
                assert all(v <= known.get(k, set()) for k, v in versions(lock).items() if k != 'query_full_state_source_506'
                           and k != 'schema_dynamic_source_506' and k != 'inmemory_source_506')
                metadata, metadata_step = run(['cargo', 'metadata', '--offline', '--locked', '--format-version', '1', '--manifest-path', manifest])
                package = next(p for p in json.loads(metadata)['packages'] if p['manifest_path'] == str(manifest))
                members = [c for c in contracts if c['project'] == project]
                command = ['cargo', 'build', '--offline', '--locked', '--manifest-path', manifest, '-j2']
                for c in members:
                    target = next(t for t in package['targets'] if t['name'] == c['reference_target'])
                    assert Path(target['src_path']).resolve() == (ROOT / c['source']).resolve()
                    command += ['--example' if c['source_kind'] == 'example' else '--bin', c['reference_target']]
                _, build_step = run(command)
                record['projects'][project] = dict(manifest=manifest.read_text(), lock=lock.read_text(),
                                                   metadata_step=metadata_step, build_step=build_step)
                for c in members:
                    executable = owned / 'target/debug' / ('examples' if c['source_kind'] == 'example' else '') / c['reference_target']
                    inputs = generated[c['name']][0]
                    payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
                    output, step = run([executable], payload)
                    originals[c['name']] = dict(step=step, output_kind='stdout', binary_sha256=digest(executable),
                                                observations=json_lines(output))
            for c in contracts:
                inputs, proof, expected = generated[c['name']]
                original = originals[c['name']]
                assert len(original['observations']) == c['count']
                assert same(original['observations'], expected), c['name'] + ': original source changed'
                payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
                native = ROOT / 'build_native_current' / c['native_target']
                output, step = run([native], payload)
                actual = json_lines(output)
                assert len(actual) == c['count'] and same(actual, original['observations']), c['name'] + ': native differs'
                record['contracts'].append(dict(name=c['name'], count=c['count'], corpus_sha256=digest(ROOT / c['corpus']),
                    input_proof=proof, reference=original, native_step=step, native_binary_sha256=digest(native)))
                save()
                print(c['name'], c['count'], 'fresh source/native match', flush=True)
            view.verify()
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file() and not p.is_symlink())
        record['input_hashes_after'] = hashes()
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert reference_files == {name: digest(reference / name) for name in reference_files}
        run(['python3', 'scripts/verify_upstream.py'])
        record['complete'] = True
    except BaseException as error:
        record['error'] = dict(type=type(error).__name__, message=str(error))
        raise
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        record['free_bytes_after'] = shutil.disk_usage(ROOT).free
        save()
    print('Seven fresh original callers / 6281 current native observations match; whole SQ02 remains open')


if __name__ == '__main__':
    main()
