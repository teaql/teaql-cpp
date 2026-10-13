#!/usr/bin/env python3
"""Compare optional generated entity fields and typed Checker extension ordering."""
import argparse
import gzip
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    def typed(kind, value=None):
        return dict(kind=kind) if value is None else dict(kind=kind, value=value)
    null = typed('null')
    fields = {
        'text': [typed('text', v) for v in ['', ' \u0085雪\u3000 ', '\u200bvalue\u200b']],
        'active': [typed('bool', False), typed('bool', True)],
        'balance': [typed('decimal', v) for v in ['0', '-123.4500', '79228162514264337593543950335']],
        'height': [typed('f64', v) for v in ['-0.0', '1.23456789', '1e-300']],
        'ratio': [typed('f64', v) for v in ['-0.0', '1.23456789', '1e-45']],
        'count': [typed('i64', v) for v in ['0', '-17', '9223372036854775807']],
        'day': [typed('date', v) for v in ['1970-01-01', '1969-12-31', '2024-02-29']],
        'recorded_at': [typed('timestamp', v) for v in ['0', '-1', '1791692772000']],
        'payload': [dict(kind='json', value=None), typed('json', False), typed('json', {'雪': [0, False, None]})],
    }
    for name, values in fields.items():
        for loaded, initial, operand, op, status in itertools.product(
                [False, True], [null, values[-1]], [null, *values], ['none', 'update', 'overlay'],
                ['create', 'update', 'unknown']):
            yield dict(kind='optional', field=name, loaded=loaded, initial=initial, value=operand,
                       op=op, status=status, full=False, allow=True, fix=False)
        for loaded, full, operand in itertools.product([False, True], [False, True],
                [null, typed('typed_null'), typed('text', 'incompatible'), typed('bool', True)]):
            yield dict(kind='optional', field=name, loaded=loaded, initial=values[-1], value=operand,
                       op='overlay', status='update', full=full, allow=True, fix=False)
    for loaded, full, allow, fix, op, status in itertools.product(
            [False, True], [False, True], [False, True], [False, True],
            ['none', 'update', 'overlay'], ['create', 'update', 'unknown']):
        yield dict(kind='optional', field='text', loaded=loaded, initial=null,
                   value=typed('text', ' input '), op=op, status=status, full=full, allow=allow, fix=fix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected = observe(str(args.rust.resolve()), inputs)
    actual = observe(str(args.cpp.resolve()), inputs)
    failures = [(i, case, left, right) for i, (case, left, right) in
                enumerate(zip(inputs, expected, actual)) if not same(left, right)]
    for index, case, left, right in failures[:4]:
        print(f'Mismatch {index}: {case}\nRust: {left}\nC++: {right}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} optional observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/optional_types_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Unmodified original generated Reading entity and typed Checker, imported by an entity/checker-only harness. Exact optional getter/changed/eval/original/clone behavior and complete diagnostics in three naming profiles. Custom logic reads a UserContext resource after predefined checks. Original complete crate compilation failures are recorded separately; no source repairs or output normalization.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    data = (json.dumps(corpus, indent=2)+'\n').encode()
    Path(__file__).with_name('generated_optional_fields_5_0_6_cases.json.gz').write_bytes(gzip.compress(data, mtime=0))
    print(f'Compared {len(inputs)} direct optional Rust/C++ observations')


if __name__ == '__main__':
    main()
