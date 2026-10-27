#!/usr/bin/env python3
"""Retain complete query-store extraction, raw outputs and actual Context evidence."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same
from run_native_integration_epoch_5_0_6 import capture_inputs, references


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def observer_source():
    """Only locate the repository and retain additional execution evidence."""
    text = (ROOT / 'scripts/verify_query_stores_source_5_0_6.py').read_text()
    text = text.replace('import gzip\n', 'import gzip\nimport hashlib\n', 1)
    needle = 'ROOT = Path(__file__).resolve().parents[1]'
    assert text.count(needle) == 1
    text = text.replace(needle, 'ROOT = Path(' + repr(str(ROOT)) + ')')
    needle = "'stdout': result.stdout, 'stderr': result.stderr})"
    assert text.count(needle) == 1
    text = text.replace(needle,
        "'stdout': result.stdout, 'stderr': result.stderr, "
        "'stdin_sha256': None if input_text is None else hashlib.sha256(input_text.encode()).hexdigest(), "
        "'stdin_bytes': None if input_text is None else len(input_text.encode())})")
    needle = "        record['actual_context'] = {'contracts_passed': 2,"
    assert text.count(needle) == 1
    evidence = """        record['actual_context_files'] = {
            'directory': str(project),
            'inputs': (project / 'inputs.json').read_text(),
            'outputs': (project / 'outputs.json').read_text(),
            'inputs_sha256': digest(project / 'inputs.json'),
            'outputs_sha256': digest(project / 'outputs.json'),
            'environment': {k: environment[k] for k in (
                'TEAQL_STORE_CONTEXT_INPUTS', 'TEAQL_STORE_CONTEXT_OUTPUTS')}}
"""
    return text.replace(needle, evidence + needle)


def extracted_sources():
    """Reconstruct every output of the unchanged build script from locked source."""
    source = ROOT / '.reference/teaql-rs/teaql-runtime/src'
    helpers = (source / 'data_service/helpers.rs').read_text()
    resolved = (source / 'data_service/resolved.rs').read_text()
    pagination = (source / 'context/pagination.rs').read_text()

    def part(text, start, end):
        return text[text.index(start):text.index(end, text.index(start))]

    keys = part(helpers, 'pub(super) fn aggregation_cache_namespace(',
                'pub(super) fn invalidate_aggregation_cache_namespace(')
    keys += part(helpers, 'pub(super) fn aggregation_cache_key(', 'pub(super) fn ensure_projection(')
    keys += 'impl KeyRepo {\n' + part(resolved, '    fn continuous_page_query_key(',
                                   '    async fn prepare_id_set_page(') + '}\n'
    locks = 'use std::{collections::HashMap,sync::{Arc,Mutex,OnceLock}};\n'
    locks += part(pagination, 'pub(super) fn id_set_build_lock(', '\nimpl UserContext {')
    observations = 'impl UserContext {\n'
    for start, end in [('    pub fn continuous_page_plan(',
                        '    pub(crate) fn continuous_page_cursor_store('),
                       ('    pub fn id_set_plan(', '    pub(crate) fn id_set_store(')]:
        observations += part(pagination, start, end)
    observations += '}\n'
    return {'query_keys.rs': keys,
            'query_stores.rs': pagination[:pagination.index('pub(super) fn id_set_build_lock(')],
            'id_set_build_lock.rs': locks,
            'pagination_observations.rs': observations,
            'aggregation_cache.rs': (source / 'data_service/cache.rs').read_text()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    registry = read(args.contracts)
    assert len(registry['contracts']) == 3 and sum(c['count'] for c in registry['contracts']) == 477
    record = dict(complete=False, whole_goal_complete=False, registry=registry,
                  input_hashes_before=capture_inputs(), references_before=references(), steps=[])
    record['observer_adapter_source'] = observer_source()
    reference = ROOT / '.reference/teaql-rs'
    reference_names = subprocess.check_output(['git', '-C', str(reference), 'ls-files', '-z']).split(b'\0')
    reference_paths = [n.decode() for n in reference_names if n and (
        n.decode().endswith('.rs') or Path(n.decode()).name in ['Cargo.toml', 'Cargo.lock'])]
    record['reference_source_files'] = {name: digest(reference / name) for name in reference_paths}
    environment = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                       PYTHONDONTWRITEBYTECODE='1', TEAQL_LOG_ENDPOINT='off')
    environment['PYTHONPATH'] = str(ROOT / 'scripts') + ':' + str(ROOT / 'tests/parity') + (':' + environment['PYTHONPATH'] if environment.get('PYTHONPATH') else '')

    def run(label, command):
        result = subprocess.run(list(map(str, command)), cwd=ROOT, env=environment,
                                capture_output=True, text=True, timeout=1200)
        record['steps'].append(dict(label=label, command=list(map(str, command)),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        run('upstream', ['python3', 'scripts/verify_upstream.py'])
        run('rustc', ['rustc', '-vV'])
        run('cargo', ['cargo', '--version'])
        project = ROOT / registry['project']
        record['project_metadata'] = run('metadata', ['cargo', 'metadata', '--offline', '--locked',
            '--format-version', '1', '--manifest-path', project])
        record['project_files'] = {name: (project.parent / name).read_text()
                                   for name in ['Cargo.toml', 'Cargo.lock', 'observers.json']}
        run('native_build', ['cmake', '--build', '--preset', 'native', '--target',
            *[c['native_target'] for c in registry['contracts']], '-j', '4'])
        with tempfile.TemporaryDirectory(prefix='teaql_query_store_driver_replay_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = temporary
            children = owned / 'children'
            children.mkdir()
            environment['TMPDIR'] = str(children)
            record['cargo_environment'] = {k: environment[k] for k in
                                          ['CARGO_INCREMENTAL', 'CARGO_PROFILE_DEV_DEBUG']}
            record['cargo_environment']['CARGO_TARGET_DIR'] = str(owned / 'target')
            driver = owned / 'observer.py'
            driver.write_text(record['observer_adapter_source'])
            inner = owned / 'execution.json.gz'
            run('actual_source_and_native', ['python3', driver, '--rust-target', owned / 'target',
                                            '--record', inner])
            record['source_execution'] = read(inner)
            expected = extracted_sources()
            outputs = [p for p in (owned / 'target/debug/build').glob(
                'teaql_query_stores_original_source_506-*/out') if (p / 'query_stores.rs').is_file()]
            assert len(outputs) == 1
            record['extracted_files'] = {}
            for name, text in expected.items():
                path = outputs[0] / name
                assert path.read_bytes() == text.encode(), name
                record['extracted_files'][name] = dict(text=text, sha256=digest(path))
            assert record['source_execution']['complete'] is True
            assert record['source_execution']['actual_context']['contracts_passed'] == 2
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*')
                                               if p.is_file() and not p.is_symlink())
        record['input_hashes_after'] = capture_inputs()
        record['references_after'] = references()
        assert record['reference_source_files'] == {name: digest(reference / name) for name in reference_paths}
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert record['references_before'] == record['references_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Three store/lock/model matrices and actual Context supplement match; source outputs retained.')


if __name__ == '__main__':
    main()
