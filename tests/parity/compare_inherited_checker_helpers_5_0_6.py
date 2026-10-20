#!/usr/bin/env python3
"""Compare inherited changed/eval snapshots and direct/governed Checker flow."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path

from compare_values import same
from sample_clock_driver import observe_fixed

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('inherited_checker_helpers_5_0_6_cases.json.gz')
MODELS = ('document', 'intermediate_record', 'invoice', 'receipt', 'special_invoice')


def cases():
    profiles = (('none', False, False, 'stored'),
                ('update', False, False, '\u0085\u00a0\u3000'),
                ('overlay', True, False, ''),
                ('self', False, True, '  spaced  '))
    modes = (('direct', 'create'), ('direct', 'update'), ('direct', 'unknown'),
             ('governed', 'create'), ('governed', 'update'))
    for model, (mode, status), root, tenant, loaded, profile, wall in itertools.product(
            MODELS, modes, (None, 42, 43), (0, 42, 43),
            ('all', 'identity', 'no_times', 'no_tenant'), profiles, (1001, 1704067200789)):
        op, reject, fix, name = profile
        yield dict(model=model, mode=mode, status=status, root=root, tenant=tenant,
                   loaded=loaded, op=op, reject=reject, fix=fix, name=name, wall_millis=wall)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp-dir', type=Path, required=True)
    parser.add_argument('--cpp-prefix', default='teaql_inherited_checker_helpers_')
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--clock', type=Path, required=True)
    parser.add_argument('--model', choices=MODELS)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and (not args.rust or args.model):
        parser.error('--record requires live Rust and the complete five-model matrix')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        expected = observe_fixed(args.rust, inputs, args.clock)
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if corpus['baseline'] != baseline or not same(inputs, [c['input'] for c in corpus['cases']]):
            raise RuntimeError('Inherited Checker corpus baseline/matrix differs')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(expected, observe_fixed(args.rust, inputs, args.clock)):
            raise RuntimeError('Inherited Checker corpus differs from live original execution')
    count = 0
    for model in MODELS:
        if args.model and args.model != model:
            continue
        indexes = [i for i, value in enumerate(inputs) if value['model'] == model]
        native = observe_fixed(args.cpp_dir / (args.cpp_prefix + model + '_probe'),
                               [inputs[i] for i in indexes], args.clock)
        gaps = [dict(input=inputs[index], rust=expected[index], cpp=value)
                for index, value in zip(indexes, native) if not same(expected[index], value)]
        if gaps:
            path = ROOT / 'docs/parity/inherited_checker_helpers_5_0_6_initial_gaps.json.gz'
            if not path.exists():
                path.write_bytes(gzip.compress(json.dumps(dict(model=model, gaps=gaps), indent=2)
                                               .encode(), mtime=0))
            print(json.dumps(gaps[0], indent=2))
            raise SystemExit(f'{model}: {len(gaps)}/{len(indexes)} inherited Checker differences')
        count += len(indexes)
        print(f'{model}: {len(indexes)} exact original/native Checker/helper outcomes', flush=True)
    if args.record:
        data = dict(baseline=baseline, scope='Complete unchanged original inheritance_variants domain. '
                    'Five models, generated direct Checker and audited-ledger preflight with denying '
                    'Policy; actual scalar changed/eval, shared snapshot views, hook ordering, '
                    'diagnostics in three naming profiles and ordered fix evidence. Fixed test wall '
                    'clock; no normalization. No committed-write claim.',
                    cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(data, separators=(',', ':')).encode(), mtime=0))
    print(f'Inherited Checker/helpers: {count} outcomes passed')


if __name__ == '__main__':
    main()
