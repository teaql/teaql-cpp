#!/usr/bin/env python3
"""Replay the complete private typed-aggregate helper, without output normalization."""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_scalar_baselines_5_0_6 import BASELINE, ROOT, PARITY, digest, read
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--cpp-prefix', default='teaql_')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(PARITY / 'build_typed_aggregate_oracle_5_0_6.py')], check=True)
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    historical_path = PARITY / 'typed_aggregate_scope_cases.json'
    controls_path = PARITY / 'typed_aggregate_controls_5_0_6.json'
    historical = read(historical_path)
    inputs = [case['input'] for case in historical['cases']] + read(controls_path)
    source = Path('/tmp/teaql_cpp_typed_aggregate_rust_506/debug/teaql_typed_aggregate_506')
    cpp = args.build_dir / (args.cpp_prefix + 'typed_aggregate_scope_probe')
    expected = observe(str(source), inputs)
    actual = observe(str(cpp), inputs)
    gaps = [dict(index=i, input=v, rust=a, cpp=b)
            for i, (v, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    failures = ROOT / 'docs/parity/typed_aggregate_5_0_6_initial_gaps.json'
    if gaps and not failures.exists():
        failures.write_text(json.dumps(dict(baseline=BASELINE, gaps=gaps), indent=2) + '\n')
    print(f'Complete original typed aggregate: {len(inputs) - len(gaps)}/{len(inputs)} exact observations')
    if gaps:
        print(json.dumps(gaps[:3], indent=2))
        raise RuntimeError(f'Typed aggregate gaps remain; see {failures}')
    provenance = read(ROOT / 'build_rust_typed_aggregate_5_0_6/provenance.json')
    corpus = dict(baseline=BASELINE, historical_corpus=historical_path.name,
                  historical_sha256=digest(historical_path),
                  controls_sha256=digest(controls_path),
                  provenance=provenance,
                  verifier_sha256=digest(Path(__file__)),
                  notice='The complete original runtime calls the private typed aggregate helper '
                         'through one append-only delegation bridge. Historical inputs remain '
                         'unchanged; separate controls declare intent explicitly. No result normalization.',
                  cases=[dict(input=v, expected=e) for v, e in zip(inputs, expected)])
    output = PARITY / 'typed_aggregate_scope_5_0_6_cases.json.gz'
    record = ROOT / 'docs/parity/typed_aggregate_5_0_6.json'
    summary = dict(schema=1, baseline=BASELINE, observations=len(inputs),
                   historical_inputs=len(historical['cases']),
                   explicit_controls=len(read(controls_path)),
                   changed_historical_source_outcomes=sum(
                       not same(case['expected'], value)
                       for case, value in zip(historical['cases'], expected)),
                   complete_original_runtime=True,
                   original_archive_files=len(provenance['files']),
                   append_only_bridges=1, original_registry_versions_preserved=True,
                   historical_sha256=digest(historical_path), controls_sha256=digest(controls_path),
                   original_method='teaql-runtime/src/data_service/resolved.rs::'
                                   'fetch_entities_with_relation_aggregates_internal',
                   original_module_sha256=provenance['files'][
                       'teaql-runtime/src/data_service/resolved.rs']['original_sha256'],
                   corpus=output.name, full_capability_acceptance=False)
    if args.record:
        output.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
        summary['corpus_sha256'] = digest(output)
        record.write_text(json.dumps(summary, indent=2) + '\n')
    else:
        if not same(corpus, read(output)):
            raise RuntimeError('Recorded original observations or source provenance changed')
        summary['corpus_sha256'] = digest(output)
        if not same(summary, read(record)):
            raise RuntimeError('Recorded verification summary changed')


if __name__ == '__main__':
    main()
