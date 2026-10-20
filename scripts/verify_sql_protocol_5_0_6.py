#!/usr/bin/env python3
"""Qualify current request constructors and execution without rewriting old inputs."""
import argparse
from copy import deepcopy
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_scalar_baselines_5_0_6 import BASELINE, ROOT, PARITY, digest, read
from compare_values import same

MATRICES = (
    ('executor_defaults_scope', 'executor_defaults_protocol', 'executor_defaults_scope_probe'),
    ('sql_query', 'sql_query_protocol', 'sql_query_probe'),
    ('sql_stream', 'sql_stream_protocol', 'sql_stream_probe'),
    ('sql_execution', 'sql_execution_protocol', 'sql_execution_protocol_probe'),
    ('sql_transaction', 'sql_execution_protocol', 'sql_execution_protocol_probe'),
    ('sql_executor_kind', 'sql_execution_protocol', 'sql_execution_protocol_probe'),
    ('sql_cached_partition', 'sql_cached_partition', 'sql_cached_partition_probe'),
)


def observe(executable, inputs):
    result = subprocess.run([str(executable)], input=''.join(json.dumps(value)+'\n' for value in inputs),
                            text=True, capture_output=True)
    lines = result.stdout.split('\n')
    if lines and not lines[-1]:
        lines.pop()
    if result.returncode or len(lines) != len(inputs):
        failure = dict(executable=str(executable), exit_code=result.returncode,
                       outputs=len(lines), expected=len(inputs), stdout=result.stdout, stderr=result.stderr)
        path = ROOT / 'docs/parity/sql_protocol_5_0_6_process_failure.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(failure, indent=2)+'\n').encode(), mtime=0))
        raise RuntimeError(f'Failed/incomplete observer: {executable}; see {path}')
    return [json.loads(line) for line in lines]


def explicit_intent(stem, input):
    value = deepcopy(input)
    if stem == 'executor_defaults_scope':
        value.update(purpose='Compare executor defaults', audit='Compare mutation defaults')
    elif stem == 'sql_query':
        for index, item in enumerate(value['requests']):
            item.update(comment=f'Inspect query request {index}', purpose='Compare SQL query mechanics')
    elif stem == 'sql_stream':
        value.update(comment='Inspect streamed rows', purpose='Compare SQL stream mechanics')
    else:
        def decorate(item, path='root'):
            item['comment'] = f'Inspect mutation {path}'
            for index, child in enumerate(item.get('children', [])):
                decorate(child, f'{path}.{index}')
        decorate(value['request'])
    return value


def negative_controls(stem, first):
    result = []
    for invalid in (None, '', ' \t\n', '\u2003\u00a0', 7):
        value = deepcopy(first)
        if stem == 'sql_query':
            value['requests'] = value['requests'][:1]
            value['requests'][0]['purpose'] = invalid
        elif stem in ('executor_defaults_scope', 'sql_stream'):
            value['purpose'] = invalid
        else:
            value['request']['comment'] = invalid
        result.append(value)
    if stem == 'executor_defaults_scope':
        for invalid in (None, '', ' \t\n'):
            value = deepcopy(first); value['audit'] = invalid; result.append(value)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--cpp-prefix', default='teaql_')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    assert read(ROOT / 'upstream.lock.json')['commit'] == BASELINE
    manifest = PARITY / 'rust_sql_protocol_5_0_6/Cargo.toml'
    target = Path('/tmp/teaql_cpp_sql_baselines_506')
    env = dict(os.environ, CARGO_TARGET_DIR=str(target), CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    command = ['cargo', 'build', '--offline', '--locked', '--manifest-path', str(manifest)]
    for bin in sorted({bin for _, bin, _ in MATRICES}):
        command += ['--bin', bin]
    subprocess.run(command, env=env, check=True)
    dependencies = [Path(__file__), PARITY / 'rust_values/src/fixture_value.rs', PARITY / 'rust_values/src/fixture_sql.rs',
                    PARITY / 'rust_field_layout/src/bin/sql_execution_5_0_6.rs', PARITY / 'sql_execution_probe.cpp']
    helpers = {str(path.relative_to(ROOT)): digest(path) for path in dependencies}
    staged, summaries, differences = [], [], []
    for stem, driver, probe in MATRICES:
        historical = PARITY / f'{stem}_cases.json'
        if stem == 'sql_cached_partition':
            historical = PARITY / 'sql_grouped_partition_inputs_5_0_6.json'
            old = read(historical); old_hash = digest(historical)
            legacy_inputs, valid_inputs, negative = [], [], []
            for case in old['cases']:
                value = case['input']; q = value['query']
                if q['group'] != ['owner','category'] or 'filter' in q or q['slice'] not in ({'offset':0,'limit':1},{'offset':1,'limit':1}):
                    continue
                queries = [deepcopy(q) for _ in range(3)]
                minimum = queries[0]['having']['right']['value']['value']
                queries[1]['having']['right']['value']['value'] = '3' if minimum == '2' else '2'
                queries[2]['having']['right']['value']['value'] = '1'
                valid_inputs.append(dict(seed_sql=value['seed_sql'],entity=value['entity'],queries=queries))
            assert len(valid_inputs) == 8
        else:
            if not historical.exists():
                historical = historical.with_suffix('.json.gz')
            old = read(historical); old_hash = digest(historical)
            legacy_inputs = [case['input'] for case in old['cases']]
            valid_inputs = [explicit_intent(stem, value) for value in legacy_inputs]
            negative = negative_controls(stem, valid_inputs[0])
        inputs = legacy_inputs + valid_inputs + negative
        original = observe(target / 'debug' / driver, inputs)
        actual = observe(args.build_dir / (args.cpp_prefix + probe), inputs)
        gaps = [dict(matrix=stem, index=i, input=value, rust=a, cpp=b)
                for i, (value, a, b) in enumerate(zip(inputs, original, actual)) if not same(a, b)]
        differences.extend(gaps)
        source = PARITY / f'rust_sql_protocol_5_0_6/{driver}.rs'
        native = PARITY / f'{probe}.cpp'
        corpus = dict(baseline=BASELINE, historical_corpus=historical.name, historical_sha256=old_hash,
                      historical_inputs=len(legacy_inputs), explicit_intent_controls=len(valid_inputs), negative_controls=len(negative),
                      driver=str(source.relative_to(ROOT)), driver_sha256=digest(source), native_probe_sha256=digest(native),
                      manifest_sha256=digest(manifest), lock_sha256=digest(manifest.with_name('Cargo.lock')), helper_sha256=helpers,
                      notice='Historical inputs unchanged; new driver construction follows actual 5.0.6 intent protocol. Separate explicit-intent and negative controls; exact outputs, no normalization.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, original)])
        active = PARITY / f'{stem}_protocol_5_0_6_cases.json.gz'
        if not args.record and not gaps and not same(read(active), corpus):
            raise RuntimeError(f'{stem}: recorded corpus differs from actual source')
        assert digest(historical) == old_hash
        staged.append((active, corpus))
        summaries.append(dict(matrix=stem, observations=len(inputs), historical_inputs=len(legacy_inputs),
                              explicit_intent_controls=len(valid_inputs), negative_controls=len(negative),
                              native_differences=len(gaps), active=str(active.relative_to(ROOT)),
                              historical_constructor_rejections=sum('intent_error' in value or any('intent_error' in item for item in value.get('results', [])) for value in original[:len(legacy_inputs)])))
        print(f'{stem}: {len(inputs)-len(gaps)}/{len(inputs)} exact constructor/execution outcomes', flush=True)
    if differences:
        path = ROOT / 'docs/parity/sql_protocol_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress((json.dumps(dict(count=len(differences), differences=differences), indent=2)+'\n').encode(), mtime=0))
        print(json.dumps(differences[0], indent=2)); raise SystemExit(f'{len(differences)} differences; no corpus activated')
    if args.record:
        for path, corpus in staged:
            path.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':'))+'\n').encode(), mtime=0))
        record = dict(schema=1, baseline=BASELINE, matrices=summaries, observations=sum(row['observations'] for row in summaries),
                      normalization='none', helper_sha256=helpers, complete_capability_acceptance=False)
        (ROOT / 'docs/parity/sql_protocol_5_0_6.json').write_text(json.dumps(record, indent=2)+'\n')
    print('Current SQL protocol replay complete; historical inputs and drivers retained')


if __name__ == '__main__':
    main()
