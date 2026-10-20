#!/usr/bin/env python3
"""Verify original result guards and the exact preserved runtime repair chain."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from verify_predicate_projection_source_5_0_6 import closing_brace

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'generator/src/main/resources/generator/rust-lib-core/request/index.stg'
RUNTIME = 'teaql_runtime/include/teaql/runtime/entity_query.hpp'
RECORD = ROOT / 'docs/parity/query_result_marker_source_review_5_0_6.json'


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def span(text, start, end):
    body = text[start:end]
    return dict(start_line=text[:start].count('\n') + 1,
                end_line=text[:end].count('\n') + 1, sha256=digest(body), body=body)


def report():
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    source = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql_code_gen'),
                                     'show', ledger['generator_commit'] + ':' + SOURCE]).decode()
    assert digest(source) == ledger['sources'][SOURCE]['sha256']
    start = source.index('impl\\<R\\> <objectDescriptor.rustStructName>Request\\<R\\> {')
    end = source.index('impl\\<R: teaql_core::Entity\\> crate::PurposedQuery', start)
    names = ['new', 'return_type', 'query', 'relation_selections', 'relation_filters',
             'child_enhancements', 'query_options', 'into_query', 'purpose',
             'matching_any_of', 'match_any_of', 'enhance_child']
    functions = []
    for name in names:
        match = re.search(r'\bfn ' + name + r'(?:\\<[^\n]*?\\>)?\(', source[start:end])
        assert match, name
        begin = start + match.start()
        opening = source.index('{', begin)
        functions.append(dict(name=name, guard='unconstrained R',
                              **span(source, begin, closing_brace(source, opening))))
    assert 'return_type\\<T\\>' in next(row['body'] for row in functions if row['name'] == 'return_type')
    purposed_start = end
    purposed_end = closing_brace(source, source.index('{', purposed_start))
    purposed = span(source, purposed_start, purposed_end)
    for name in ['execute_for_list', 'execute_for_rows', 'execute_for_stream', 'execute_for_first',
                 'execute_for_one', 'execute_for_page', 'execute_for_count', 'execute_for_exists',
                 'new_entity']:
        assert re.search(r'\bfn ' + name + r'(?:\\<[^\n]*?\\>)?\(', purposed['body']), name

    before = json.loads(gzip.decompress((ROOT / 'docs/parity/query_result_marker_runtime_before_5_0_6.json.gz').read_bytes()))
    assert before['source'] == RUNTIME and digest(before['body']) == before['sha256']
    current = (ROOT / RUNTIME).read_text()
    expected = before['body'].replace('bool Purposed = false, Entity R = T', 'bool Purposed = false, class R = T')
    expected = expected.replace('template <Entity, class, bool, Entity>', 'template <Entity, class, bool, class>')
    expected = expected.replace('template <Entity Result>', 'template <class Result>')
    expected = expected.replace('Entity ChildResult>', 'class ChildResult>')
    expected = expected.replace('requires Purposed', 'requires Purposed && Entity<R>')
    expected = expected.replace(
        '    // Adapt decoding without changing the source entity, projections, Context\n'
        '    // binding or creation model. Lvalues retain an independent description.\n',
        '    // The result is an unconstrained marker until public execution/creation,\n'
        '    // where the source facade requires Entity. No marker object is constructed.\n'
        '    // Lvalues retain an independent Context-owned description.\n')
    formatter = ROOT / '.tools/clang-format/bin/clang-format'
    expected = subprocess.check_output([str(formatter), '--style=file:' + str(ROOT / '.clang-format')],
                                       input=expected.encode()).decode()
    if current != expected:
        # Keep the historical marker review immutable. Independently prove the
        # complete later header delta before replaying that exact checkpoint.
        checkpoint = json.loads(gzip.decompress(
            (ROOT / 'docs/parity/stream_shapes_before_5_0_6.json.gz').read_bytes()))
        previous = checkpoint['files'][RUNTIME]
        assert digest(previous['text']) == previous['sha256']
        assert previous['text'] == expected, 'Stream checkpoint changes the marker repair'
        old = ('    EntityQuery& stream(std::size_t chunk_size) & {\n'
               '        description().query.stream(chunk_size);\n')
        new = ('    EntityQuery& stream(std::size_t chunk_size) & {\n'
               '        if (chunk_size == 0) {\n'
               '            throw std::invalid_argument("stream chunk size must be positive");\n'
               '        }\n'
               '        description().query.stream(chunk_size);\n')
        assert expected.count(old) == 1
        stream_checkpoint = expected.replace(old, new)
        if current != stream_checkpoint:
            from lookup_failure_qualification import verify
            assert verify(ROOT) == stream_checkpoint, 'Lookup checkpoint changes the stream repair'
            print('Verified exact subsequent lookup-result adapters in both runtime headers')
        print('Verified the preserved positive stream-chunk guard and its complete follow-up chain; '
              'replaying the historical marker checkpoint below')
        current = previous['text']
    assert current == expected, 'Repair changes more than reviewed marker constraints'
    assert current.count('Entity<R>') == 9
    return dict(generator_commit=ledger['generator_commit'], runtime_commit=ledger['runtime_commit'],
                source=SOURCE, source_sha256=digest(source), request_guard='impl<R>',
                request_functions=functions, public_execution_impl=purposed,
                runtime_source=RUNTIME, runtime_sha256=digest(current),
                disposition='Runtime constraint repair only; generated facade migration remains required.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--check-original-compiler', action='store_true')
    args = parser.parse_args()
    current = report()
    if args.record:
        assert not RECORD.exists(), 'Preserve the earlier source review'
        RECORD.write_text(json.dumps(current, indent=2) + '\n')
    else:
        assert current == json.loads(RECORD.read_text()), 'Reviewed source/runtime guard bodies changed'
    if args.check_original_compiler:
        env = dict(os.environ, CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                   CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506')
        command = ['cargo', 'rustc', '--offline', '--locked', '--manifest-path',
                   str(ROOT / 'tests/parity/rust_request_state_5_0_6/Cargo.toml'),
                   '--bin', 'query_result_marker']
        negative = subprocess.run(command + ['--', '--cfg', 'teaql_marker_negative'],
                                  cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
        print(negative.stdout)
        assert negative.returncode != 0
        assert negative.stdout.count('error[E0599]') == 9
        assert 'NotAnEntity: teaql_core::Entity' in negative.stdout
        for name in ['execute_for_list', 'execute_for_rows', 'execute_for_stream',
                     'execute_for_first', 'execute_for_one', 'execute_for_page',
                     'execute_for_count', 'execute_for_exists', 'new_entity']:
            assert f'method `{name}`' in negative.stdout, name
        subprocess.run(command, cwd=ROOT, env=env, check=True)
        print('Verified all 9 original execution/creation refusals and rebuilt the positive observer')
    print('Verified 12 complete unconstrained request methods, complete Entity-guarded public facade, and exact runtime-only constraint diff')


if __name__ == '__main__':
    main()
