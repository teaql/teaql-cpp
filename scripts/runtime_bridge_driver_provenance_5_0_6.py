#!/usr/bin/env python3
"""Verify complete original driver bodies through explicit private access bridges."""
import argparse
import ast
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

import archive_driver_provenance_5_0_6 as archive
from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from run_native_integration_epoch_5_0_6 import capture_inputs, references

ROOT = archive.ROOT


def source_files(registry):
    names = set(registry['support_files']) | {c['corpus'] for c in registry['contracts']}
    names |= {n for c in registry['contracts'] for n in c['extra_corpora']}
    names |= {s['path'] for s in registry['observers']}
    return {name: archive.digest(ROOT / name) for name in sorted(names)}


def values(contract):
    cases = []
    for name in [*contract['extra_corpora'], contract['corpus']]:
        cases.extend(archive.read(ROOT / name)['cases'])
    return [r['input'] for r in cases], [r['expected'] for r in cases]


def check_inputs(registry):
    assert registry['schema'] == 1
    assert registry['runtime_commit'] == archive.read(ROOT / 'upstream.lock.json')['commit']
    assert registry['generator_commit'] == archive.read(ROOT / 'docs/cpp_generator_parity_ledger.json')['generator_commit']
    assert registry['source_hashes'] == source_files(registry)
    assert {c['profile'] for c in registry['contracts']} == {'debug'}
    assert registry['base_verifier_sha256'] == archive.digest(ROOT / 'scripts/runtime_protocol_driver_provenance_5_0_6.py')
    assert len({c['name'] for c in registry['contracts']}) == len(registry['contracts'])
    for c in registry['contracts']:
        corpus = archive.read(ROOT / c['corpus'])
        assert corpus['baseline'] == registry['runtime_commit']
        assert len(corpus['cases']) == c['count'] > 0
        assert c['native_command'] == [str(ROOT / 'build_native_current' / c['native_target'])]
        assert c['registered_command'][c['registered_command'].index('--cpp') + 1] == c['native_command'][0]


def literal_constant(path, name):
    tree = ast.parse(path.read_text())
    values = [node.value for node in tree.body if isinstance(node, ast.Assign)
              and any(isinstance(t, ast.Name) and t.id == name for t in node.targets)]
    assert len(values) == 1
    return ast.literal_eval(values[0])


def render(registry, owned):
    result = {}
    for spec in registry['observers']:
        path = ROOT / spec['path']
        if spec['kind'] == 'constant':
            value = literal_constant(path, spec['constant'])
            text = value[spec['key']] if 'key' in spec else value
            assert type(text) is str
        elif spec['kind'] == 'bridge':
            text = path.read_text()
        else:
            assert spec['kind'] == 'driver'
            body = path.read_text()
            for original, replacement in spec['replacements'].items():
                assert body.count(original) == 1
                body = body.replace(original, '#[path = ' + json.dumps(str(owned / replacement)) + ']')
            input_path = owned / 'inputs' / (spec['contract'] + '.json')
            argument = '&input' if spec['borrow_input'] else 'input'
            test = ('\nuse crate as teaql_runtime;\n#[test]\nfn replay_original_driver() {\n'
                    '    let inputs: Vec<serde_json::Value> = serde_json::from_str(include_str!('
                    + json.dumps(str(input_path)) + ')).unwrap();\n'
                    '    for input in inputs { println!("' + spec['marker']
                    + '{}", evaluate(' + argument + ')); }\n}\n')
            text = ('#[cfg(test)]\n#[allow(dead_code)]\nmod ' + spec['module']
                    + ' {\n' + body + test + '}\n')
        result[spec['runtime_file']] = result.get(spec['runtime_file'], '') + '\n' + text
    return result


def manifest_text(metadata, runtime_manifest):
    package = next(p for p in metadata['packages'] if p['manifest_path'] == str(runtime_manifest))
    assert package['version'] == '5.0.6' and package['edition'] == '2024'
    assert package['features'] == {'opentelemetry': ['dep:opentelemetry']}
    text = '[package]\nname="teaql-runtime"\nversion="5.0.6"\nedition="2024"\n[workspace]\n'
    omitted = []
    for kind, heading in [(None, 'dependencies'), ('dev', 'dev-dependencies')]:
        text += '\n[' + heading + ']\n'
        for d in package['dependencies']:
            if d['kind'] != kind:
                continue
            if kind == 'dev' and d['name'] != 'teaql-macros':
                omitted.append(d['name'])
                continue
            assert not d.get('target')
            fields = ['version=' + json.dumps(d['req'])]
            for key in ['path', 'features', 'optional']:
                if d.get(key):
                    fields.append(key + '=' + json.dumps(d[key]))
            if not d.get('uses_default_features', True):
                fields.append('default-features=false')
            text += d['name'] + '={' + ','.join(fields) + '}\n'
    assert set(omitted) == {'opentelemetry_sdk', 'opentelemetry-otlp'}
    return text + '\n[features]\nopentelemetry=["dep:opentelemetry"]\n'


def selected_tests(output):
    return re.findall(r'^test (\S+) \.\.\.', output, flags=re.M)


def test_command(binary, selection):
    return [str(binary), selection, '--nocapture', '--test-threads=1']


def parse_source(contract, stdout, file_values, inputs):
    if contract['output_kind'] == 'file':
        result = file_values
    else:
        rows = [json.loads(line.split(contract['marker'], 1)[1])
                for line in stdout.split('\n') if contract['marker'] in line]
        if contract['output_kind'] == 'echo':
            assert archive.same([r['input'] for r in rows], inputs)
            result = [r['output'] for r in rows]
        else:
            result = rows
    assert type(result) is list and len(result) == len(inputs)
    return result


def replay(registry, output):
    check_inputs(registry)
    assert not output.exists()
    record = dict(complete=False, whole_goal_complete=False, registry=registry,
                  source_files=source_files(registry), steps=[], profiles={}, results=[],
                  input_hashes_before=capture_inputs(), references_before=references())

    def save():
        output.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    def run(label, command, payload=None, env=None):
        command = [str(x) for x in command]
        p = subprocess.run(command, cwd=ROOT, env=env or dict(os.environ, TEAQL_LOG_ENDPOINT='off'),
                           input=payload, text=True, capture_output=True)
        step = dict(label=label, command=command, exit_code=p.returncode,
                    stdout=p.stdout, stderr=p.stderr)
        if payload is not None:
            step.update(stdin_sha256=hashlib.sha256(payload.encode()).hexdigest(),
                        stdin_bytes=len(payload.encode()))
        record['steps'].append(step)
        save()
        print(label, 'exit', p.returncode, flush=True)
        p.check_returncode()
        return p.stdout

    save()
    try:
        run('upstream', ['python3', 'scripts/verify_upstream.py'])
        targets = sorted({c['native_target'] for c in registry['contracts']})
        run('native_build', ['cmake', '--build', '--preset', 'native', '--parallel', '4', '--target', *targets])
        data = archive.archive_bytes(registry)
        originals = archive.archive_files(data)
        record.update(archive_sha256=hashlib.sha256(data).hexdigest(),
                      archive_files={n: hashlib.sha256(v).hexdigest() for n, v in originals.items()})
        with tempfile.TemporaryDirectory(prefix='teaql_runtime_bridge_506_') as tmp:
            owned = Path(tmp)
            record['owned_directory'] = tmp
            original_root = owned / '.reference/teaql-rs'
            for name, content in originals.items():
                p = original_root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_bytes(content)
            for name, expected in record['source_files'].items():
                p = owned / name
                p.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT / name, p)
                assert archive.digest(p) == expected
            (owned / 'inputs').mkdir()
            (owned / 'children').mkdir()
            for c in registry['contracts']:
                (owned / 'inputs' / (c['name'] + '.json')).write_text(json.dumps(values(c)[0]))
            generated = render(registry, owned)
            observer_files = {}
            for i, (name, text) in enumerate(sorted(generated.items())):
                p = owned / ('observer_' + str(i) + '.rs')
                p.write_text(text)
                observer_files[name] = p
            env = dict(os.environ, TEAQL_LOG_ENDPOINT='off', CARGO_TARGET_DIR=str(owned / 'target'),
                       CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TMPDIR=str(owned / 'children'), PYTHONDONTWRITEBYTECODE='1')
            env.pop('TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS', None)
            record['cargo_environment'] = {k: env[k] for k in [
                'CARGO_TARGET_DIR', 'CARGO_INCREMENTAL', 'CARGO_PROFILE_DEV_DEBUG',
                'CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER', 'TMPDIR']}
            view = ReferenceRuntimeView(owned, owned / 'runtime', observer_files,
                                        lambda command: run('view_setup', command, env=env))
            record['view'] = view.provenance()
            record['compiled_source_hashes'] = {n: archive.digest(view.directory / n) for n in view.originals}
            metadata = run('locked_metadata', ['cargo', 'metadata', '--offline', '--locked',
                '--format-version', '1', '--manifest-path', view.directory / 'Cargo.toml'], env=env)
            record['project_metadata'] = metadata
            for profile in ['debug']:
                flags = ['--release'] if profile == 'release' else []
                index = len(record['steps'])
                stdout = run('compile_' + profile, ['cargo', 'test', '--offline', '--locked',
                    '--manifest-path', view.directory / 'Cargo.toml', '-j2', '--lib', '--no-run',
                    '--message-format=json', *flags], env=env)
                artifacts = [json.loads(line) for line in stdout.splitlines() if line.startswith('{')]
                bins = [v for v in artifacts if v.get('reason') == 'compiler-artifact'
                        and v.get('executable') and v['target']['name'] == 'teaql_runtime'
                        and v['target']['kind'] == ['lib'] and v['profile']['test']]
                assert len(bins) == 1
                artifact = bins[0]
                assert artifact['profile']['debug_assertions'] == (profile == 'debug')
                binary = Path(artifact['executable'])
                assert binary.is_relative_to(owned / 'target' / profile / 'deps')
                list_index = len(record['steps'])
                listing = run('list_' + profile, [binary, '--list'], env=env)
                names = [s.removesuffix(': test') for s in listing.splitlines() if s.endswith(': test')]
                record['profiles'][profile] = dict(compile_step=index, list_step=list_index, artifact=artifact,
                    binary=str(binary), binary_sha256=archive.digest(binary), test_names=names)
                for c in registry['contracts']:
                    if c['profile'] != profile:
                        continue
                    inputs, expected = values(c)
                    selected = [name for name in names if c['test_filter'] in name]
                    assert len(selected) == c['test_count'] and c['observer_test'] in selected
                    path = owned / 'inputs' / (c['name'] + '_outputs.json')
                    child_env = dict(env, TEAQL_COMMIT_AUDIT_INPUTS=str(owned / 'inputs' / (c['name'] + '.json')),
                                     TEAQL_COMMIT_AUDIT_OUTPUTS=str(path))
                    step_index = len(record['steps'])
                    stdout = run(c['name'] + '_source', test_command(binary, c['test_filter']), env=child_env)
                    assert sorted(selected_tests(stdout)) == sorted(selected)
                    assert re.search(r'test result: ok\. ' + str(len(selected)) + r' passed; 0 failed; 0 ignored;', stdout)
                    file_values = json.loads(path.read_text()) if c['output_kind'] == 'file' else None
                    observed = parse_source(c, stdout, file_values, inputs)
                    assert archive.same(observed, expected), c['name'] + ': source differs'
                    payload = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in inputs)
                    native_index = len(record['steps'])
                    raw = run(c['name'] + '_native', c['native_command'], payload)
                    assert archive.same(archive.json_lines(raw), expected), c['name'] + ': native differs'
                    record['results'].append(dict(name=c['name'], count=c['count'], input_count=len(inputs),
                        source_step=step_index, native_step=native_index, file_output=file_values,
                        native_binary_sha256=archive.digest(Path(c['native_command'][0]))))
                    save()
            view.verify()
            assert record['view'] == view.provenance()
            assert record['compiled_source_hashes'] == {n: archive.digest(view.directory / n) for n in view.originals}
            assert record['archive_files'] == {n: archive.digest(original_root / n) for n in originals}
            assert record['source_files'] == {n: archive.digest(owned / n) for n in record['source_files']}
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*') if p.is_file())
        record['owned_directory_removed'] = not owned.exists()
        record['input_hashes_after'] = capture_inputs()
        record['references_after'] = references()
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert record['references_before'] == record['references_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        save()


def validate(record, registry):
    check_inputs(registry)
    assert record['complete'] is True and record['whole_goal_complete'] is False
    assert record['registry'] == registry and record['source_files'] == source_files(registry)
    assert record['input_hashes_before'] == record['input_hashes_after']
    assert record['references_before'] == record['references_after'] == {
        'teaql-rs': registry['runtime_commit'], 'teaql_code_gen': registry['generator_commit']}
    for name in [__file__, archive.__file__, ROOT / 'scripts/reference_runtime_view_5_0_6.py', ROOT / 'scripts/runtime_protocol_driver_provenance_5_0_6.py']:
        key = 'runtime:' + str(Path(name).relative_to(ROOT))
        assert record['input_hashes_after'][key] == archive.digest(Path(name))
    owned = Path(record['owned_directory'])
    assert owned.parent == Path('/tmp') and owned.name.startswith('teaql_runtime_bridge_506_')
    assert record['owned_directory_removed'] and not owned.exists()
    assert record['cargo_environment'] == dict(CARGO_TARGET_DIR=str(owned / 'target'),
        CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
        CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TMPDIR=str(owned / 'children'))
    data = archive.archive_bytes(registry)
    originals = archive.archive_files(data)
    assert record['archive_sha256'] == hashlib.sha256(data).hexdigest()
    assert record['archive_files'] == {n: hashlib.sha256(v).hexdigest() for n, v in originals.items()}
    rendered = render(registry, owned)
    view = record['view']
    expected_originals = {name.removeprefix('teaql-runtime/'): value
                          for name, value in record['archive_files'].items()
                          if name.startswith(('teaql-runtime/src/', 'teaql-runtime/tests/', 'teaql-runtime/test-vectors/'))}
    assert view['source_files_sha256'] == expected_originals
    assert view['all_original_bodies_preserved'] is True
    compiled = {}
    for name in expected_originals:
        raw = originals['teaql-runtime/' + name]
        if name in rendered:
            raw += b'\n' + rendered[name].encode()
        compiled[name] = hashlib.sha256(raw).hexdigest()
    assert record['compiled_source_hashes'] == compiled
    assert set(view['omitted_dev_dependencies']) == {'opentelemetry_sdk', 'opentelemetry-otlp'}
    assert set(view['observers']) == set(rendered)
    for name, text in rendered.items():
        assert view['observers'][name]['text'] == text
        assert view['observers'][name]['sha256'] == hashlib.sha256(text.encode()).hexdigest()
    steps = record['steps']
    assert steps and all(s['exit_code'] == 0 for s in steps)
    original_manifest = owned / '.reference/teaql-rs/teaql-runtime/Cargo.toml'
    setup = next(s for s in steps if s['command'][:5] == ['cargo', 'metadata', '--offline', '--locked', '--no-deps'])
    assert view['manifest'] == manifest_text(json.loads(setup['stdout']), original_manifest)
    locked_metadata = next(s for s in steps if s['label'] == 'locked_metadata')
    assert locked_metadata['command'] == ['cargo', 'metadata', '--offline', '--locked', '--format-version', '1', '--manifest-path', str(owned / 'runtime/Cargo.toml')]
    assert locked_metadata['stdout'] == record['project_metadata']
    metadata = json.loads(record['project_metadata'])
    expected = {owned / 'runtime/Cargo.toml'} | {
        owned / '.reference/teaql-rs' / name / 'Cargo.toml' for name in registry['runtime_packages'] if name != 'teaql-runtime'}
    assert {Path(p['manifest_path']) for p in metadata['packages'] if p['source'] is None} == expected
    assert all(p['version'] == '5.0.6' for p in metadata['packages'] if p['source'] is None)
    lock = view['lock']
    identities = set()
    for item in lock.split('[[package]]')[1:]:
        fields = {key: value for key, value in re.findall(r'^([a-z_]+) = "([^"\n]+)"$', item, re.M)}
        if 'source' in fields:
            identities.add(tuple(fields[key] for key in ['name', 'version', 'source', 'checksum']))
    assert identities and identities <= archive.registry_identities(ROOT / '.reference/teaql-rs/Cargo.lock')
    assert {(p['name'], p['version'], p['source']) for p in metadata['packages'] if p['source']} <= {(n, v, src) for n, v, src, checksum in identities}
    for profile, info in record['profiles'].items():
        command = ['cargo', 'test', '--offline', '--locked', '--manifest-path', str(owned / 'runtime/Cargo.toml'),
                   '-j2', '--lib', '--no-run', '--message-format=json'] + (['--release'] if profile == 'release' else [])
        step = steps[info['compile_step']]
        assert step['command'] == command
        artifacts = [json.loads(line) for line in step['stdout'].splitlines() if line.startswith('{')]
        assert info['artifact'] in artifacts
        assert info['artifact']['profile']['debug_assertions'] == (profile == 'debug')
        assert info['artifact']['executable'] == info['binary']
        assert Path(info['binary']).is_relative_to(owned / 'target' / profile / 'deps')
        assert len(info['binary_sha256']) == 64
        listing = steps[info['list_step']]
        assert listing['command'] == [info['binary'], '--list']
        assert info['test_names'] == [s.removesuffix(': test') for s in listing['stdout'].splitlines() if s.endswith(': test')]
    assert set(record['profiles']) == {'debug'}
    assert [r['name'] for r in record['results']] == [c['name'] for p in ['debug'] for c in registry['contracts'] if c['profile'] == p]
    by_name = {c['name']: c for c in registry['contracts']}
    for result in record['results']:
        c = by_name[result['name']]
        inputs, expected = values(c)
        assert result['count'] == c['count'] and result['input_count'] == len(inputs)
        info = record['profiles'][c['profile']]
        step = steps[result['source_step']]
        assert step['command'] == test_command(info['binary'], c['test_filter'])
        names = [n for n in info['test_names'] if c['test_filter'] in n]
        assert len(names) == c['test_count'] and c['observer_test'] in names
        assert sorted(selected_tests(step['stdout'])) == sorted(names)
        assert re.search(r'test result: ok\. ' + str(len(names)) + r' passed; 0 failed; 0 ignored;', step['stdout'])
        assert archive.same(parse_source(c, step['stdout'], result['file_output'], inputs), expected)
        native = steps[result['native_step']]
        payload = ''.join(json.dumps(v, ensure_ascii=True) + '\n' for v in inputs)
        assert native['command'] == c['native_command']
        assert native['stdin_sha256'] == hashlib.sha256(payload.encode()).hexdigest()
        assert native['stdin_bytes'] == len(payload.encode())
        assert archive.same(archive.json_lines(native['stdout']), expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['replay', 'verify'])
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--execution', type=Path)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    registry = archive.read(args.contracts)
    if args.mode == 'replay':
        assert args.record is not None
        replay(registry, args.record)
        validate(archive.read(args.record), registry)
    else:
        assert args.execution is not None and (args.record is None or not args.record.exists())
        validate(archive.read(args.execution), registry)
        if args.record:
            args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
                contracts=len(registry['contracts']), observations=sum(c['count'] for c in registry['contracts']),
                registry_sha256=archive.digest(args.contracts), execution_sha256=archive.digest(args.execution))).encode(), mtime=0))
    print('Private runtime prefixes, observer recipes, profiles and complete results verify.')


if __name__ == '__main__':
    main()
