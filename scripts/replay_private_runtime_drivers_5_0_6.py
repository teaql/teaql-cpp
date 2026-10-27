#!/usr/bin/env python3
"""Replay every private-runtime observer against a fresh append-only locked archive."""
import argparse
import gzip
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
import build_private_save_oracles_5_0_6 as builder
from compare_values import json_lines, same
from run_native_integration_epoch_5_0_6 import capture_inputs, references
from verify_constant_closure_5_0_6 import registry_identities


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def builder_inputs():
    return {str(Path(module.__file__).resolve().relative_to(ROOT)): digest(Path(module.__file__))
            for module in list(sys.modules.values()) if getattr(module, '__file__', None)
            and Path(module.__file__).resolve().parent == ROOT / 'tests/parity'
            and Path(module.__file__).name.startswith('build_')}


def expected_extractions():
    source = ROOT / '.reference/teaql-rs/teaql-runtime/src'

    def part(filename, start, end):
        text = (source / filename).read_text()
        first = text.index(start)
        return text[first:text.index(end, first)]

    base = part('data_service/base.rs', '    pub(crate) fn new(', '    pub(crate) async fn insert(')
    context = part('data_service/context.rs', '    async fn observe<T, F, N>(', '    pub(crate) async fn insert(')
    comments = part('data_service/context.rs', '    pub(crate) fn resolve_final_comment(', '\n}\n')
    outputs = {
        'runtime_base_query.rs': "impl<'a,M,E> RuntimeDataService<'a,M,E> where M: MetadataStore,E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Sync {\n" + base + '\n}',
        'runtime_context_query.rs': "impl<'a,E> ContextDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n" + context + '\n' + comments + '\n}'}
    wrapper = "impl<'a,E> EntityDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n"
    relation = part('data_service/relation.rs', '#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]', "impl<'a, E> EntityDataService<'a, E>")
    for start, end in [
        ('pub(super) fn relation_bucket_key(', 'pub(super) fn aggregation_cache_namespace('),
        ('pub(super) fn graph_identity_key(', 'pub(super) fn ensure_relation_target<'),
        ('pub(super) fn ensure_projection(', 'pub(super) fn attach_empty_relation_aggregate(')]:
        relation += part('data_service/helpers.rs', start, end)
    relation += wrapper
    for start, end in [
        ('    fn query_for_plan(', '    async fn fetch_relation_rows('),
        ('    async fn fetch_native_relation_rows(', '    async fn relation_facets('),
        ('    fn base_relation_query(', '    fn ensure_stable_top_n_order('),
        ('    fn ensure_stable_top_n_order(', '    fn query_for_compact_plan('),
        ('    fn query_for_compact_plan(', '    fn attach_relation_rows(')]:
        relation += part('data_service/relation.rs', start, end)
    outputs['relation_execution.rs'] = relation + '}\n'
    enhancement = ''
    for start, end in [
        ('pub(super) fn default_aggregate_value(', 'pub(super) fn aggregation_cache_namespace('),
        ('pub(super) fn graph_identity_key(', 'pub(super) fn ensure_relation_target<'),
        ('pub(super) fn ensure_projection(', '#[cfg(test)]\nmod aggregation_cache_key_tests'),
        ('pub(super) fn attach_relation_aggregate_rows(', 'pub(super) fn ensure_initial_version(')]:
        enhancement += part('data_service/helpers.rs', start, end)
    enhancement += wrapper
    for start, end in [
        ('    pub(crate) fn relation_aggregate_key_rows(', '    fn build_relation_plans('),
        ('    fn build_relation_plans_from_loads(', "    fn enhance_plan<'b>("),
        ('    fn relation_child_repo(', '    fn query_for_plan(')]:
        enhancement += part('data_service/relation.rs', start, end)
    outputs['relation_enhancement.rs'] = enhancement + '}\n'
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    registry = read(args.contracts)
    contracts = registry['contracts'] + registry['repeated_contracts']
    assert len(registry['contracts']) == 36 and len(registry['repeated_contracts']) == 3
    assert {c['target'] for c in contracts} == {n + '_5_0_6' for n in builder.FAMILIES}
    record = dict(complete=False, whole_goal_complete=False, registry=registry, steps=[], results=[],
                  input_hashes_before=capture_inputs(), references_before=references(),
                  builder_inputs=builder_inputs(), appendices=builder.APPENDS,
                  free_bytes_before=shutil.disk_usage(ROOT).free)
    environment = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off', PYTHONDONTWRITEBYTECODE='1')

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    def run(label, command, payload=None):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, env=environment, input=payload,
                                text=True, capture_output=True)
        index = len(record['steps'])
        record['steps'].append(dict(label=label, command=command, exit_code=result.returncode,
            stdout=result.stdout, stderr=result.stderr,
            stdin_sha256=None if payload is None else hashlib.sha256(payload.encode()).hexdigest(),
            stdin_bytes=None if payload is None else len(payload.encode())))
        save()
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout, index

    try:
        run('upstream', ['python3', 'scripts/verify_upstream.py'])
        run('rustc', ['rustc', '-vV'])
        run('cargo', ['cargo', '--version'])
        run('native_build', ['cmake', '--build', '--preset', 'native', '--parallel', '4',
            '--target', *dict.fromkeys(c['native_target'] for c in contracts)])
        archive = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql-rs'),
                                           'archive', registry['runtime_commit']])
        record['archive_sha256'] = hashlib.sha256(archive).hexdigest()
        with tempfile.TemporaryDirectory(prefix='teaql_private_runtime_driver_506_') as temporary:
            owned = Path(temporary)
            record['owned_directory'] = temporary
            environment['CARGO_TARGET_DIR'] = str(owned / 'target')
            record['cargo_environment'] = {k: environment[k] for k in [
                'CARGO_TARGET_DIR', 'CARGO_INCREMENTAL', 'CARGO_PROFILE_DEV_DEBUG',
                'CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER']}
            original = owned / 'build_rust_private_save_5_0_6/original'
            record['archive_files'] = {}
            with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
                for member in stream:
                    if not member.isfile():
                        assert member.isdir()
                        continue
                    path = original / member.name
                    assert path.resolve().is_relative_to(original)
                    prefix = stream.extractfile(member).read()
                    appendix = builder.APPENDS.get(member.name, '').encode()
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(prefix + appendix)
                    record['archive_files'][member.name] = dict(
                        original_sha256=hashlib.sha256(prefix).hexdigest(),
                        compiled_sha256=digest(path), appended_bytes=len(appendix))
            assert builder.APPENDS.keys() <= record['archive_files'].keys()
            project = owned / 'tests/parity/rust_private_save_5_0_6'
            project.mkdir(parents=True)
            source_project = ROOT / registry['project']
            record['project_files'] = {name: source_project.with_name(name).read_text()
                                       for name in ['Cargo.toml', 'Cargo.lock', 'build.rs']}
            for name in ['Cargo.toml', 'Cargo.lock']:
                (project / name).write_text(record['project_files'][name])
            links = {'src': source_project.parent / 'src', 'build.rs': source_project.parent / 'build.rs',
                     '../rust_values': ROOT / 'tests/parity/rust_values'}
            for name, destination in links.items():
                (project / name).symlink_to(destination, target_is_directory=destination.is_dir())
            record['source_links'] = {name: str(destination) for name, destination in links.items()}
            record['fixture_inputs'] = {str(p.relative_to(ROOT)): digest(p) for p in [
                ROOT / 'tests/parity/rust_values/src/fixture_value.rs',
                ROOT / 'tests/parity/rust_values/src/fixture_sql.rs']}
            assert registry_identities(project / 'Cargo.lock') <= registry_identities(original / 'Cargo.lock')
            run('resolve_owned_metadata', ['cargo', 'metadata', '--offline', '--format-version', '1',
                                           '--manifest-path', project / 'Cargo.toml'])
            metadata, record['metadata_step'] = run('locked_metadata', ['cargo', 'metadata',
                '--offline', '--locked', '--format-version', '1', '--manifest-path', project / 'Cargo.toml'])
            record['compiled_lock'] = (project / 'Cargo.lock').read_text()
            assert registry_identities(project / 'Cargo.lock') <= registry_identities(original / 'Cargo.lock')
            package = next(p for p in json.loads(metadata)['packages']
                           if p['manifest_path'] == str(project / 'Cargo.toml'))
            record['resolved_target_paths'] = {t['name']: str(Path(t['src_path']).resolve())
                                               for t in package['targets']}
            _, record['build_step'] = run('locked_build', ['cargo', 'build', '--offline', '--locked',
                '--manifest-path', project / 'Cargo.toml', '--bins', '-j2'])
            outputs = list((owned / 'target/debug/build').glob('teaql_cpp_private_save_oracle-*/out'))
            outputs = [p for p in outputs if (p / 'runtime_base_query.rs').exists()]
            assert len(outputs) == 1
            record['extracted_files'] = {}
            for name, expected in expected_extractions().items():
                path = outputs[0] / name
                assert path.read_bytes() == expected.encode(), name
                record['extracted_files'][name] = dict(text=expected, sha256=digest(path))
            for c in contracts:
                corpus = read(ROOT / c['corpus'])
                assert digest(ROOT / c['corpus']) == c['corpus_sha256']
                inputs = [row['input'] for row in corpus['cases']]
                expected = [row['expected'] for row in corpus['cases']]
                payload = ''.join(json.dumps(i, ensure_ascii=True) + '\n' for i in inputs)
                binary = owned / 'target/debug' / c['target']
                raw, source_step = run(c['name'] + '_source', [binary], payload)
                assert same(json_lines(raw), expected), c['name'] + ': source differs'
                raw, native_step = run(c['name'] + '_native', c['native_command'], payload)
                assert same(json_lines(raw), expected), c['name'] + ': native differs'
                record['results'].append(dict(name=c['name'],count=c['count'],source_step=source_step,
                    native_step=native_step,source_binary_sha256=digest(binary),
                    native_binary_sha256=digest(Path(c['native_command'][0]))))
                save()
            record['owned_bytes_removed'] = sum(p.stat().st_size for p in owned.rglob('*')
                                               if p.is_file() and not p.is_symlink())
        record['input_hashes_after'] = capture_inputs()
        record['references_after'] = references()
        assert record['input_hashes_before'] == record['input_hashes_after']
        assert record['references_before'] == record['references_after']
        record['complete'] = True
    finally:
        record['owned_directory_removed'] = not Path(record.get('owned_directory', '/')).exists()
        save()
    print('All 34 source targets and 39 corpus/native matrices match; repeated corpora stay separate.')


if __name__ == '__main__':
    main()
