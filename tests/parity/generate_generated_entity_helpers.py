#!/usr/bin/env python3
"""Observe same-model original generated scalar and optional/reverse relation APIs."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    for loaded, initial_null, op, null, text in itertools.product(
            [False, True], [False, True], ['none', 'update', 'update_self', 'overlay'], [False, True],
            ['', 'plain', ' \tvalue\n ', '\u0085\u00a0\u1680\u2003雪\u2028\u202f\u205f\u3000', '\u200bvalue\u200b']):
        yield dict(kind='scalar', loaded=loaded, initial_null=initial_null, op=op,
                   null=null, text=text, active=True, age=-17)
    for fk_loaded, foreign_id, selected, cached, forward, reverse in itertools.product(
            [False, True], [None, 0, 9, 13], [False, True], [False, True],
            ['absent', 'present', 'empty', 'wrong_type'], ['absent', 'present', 'empty', 'wrong_type']):
        yield dict(kind='relations', fk_loaded=fk_loaded, foreign_id=foreign_id,
                   selected=selected, cached=cached, forward=forward, reverse=reverse)
    def typed(kind, value=None):
        return dict(kind=kind) if value is None else dict(kind=kind, value=value)
    fields = {
        'balance': [typed('decimal', v) for v in ['0', '-123.4500', '0.0000001', '79228162514264337593543950335']],
        'height': [typed('f64', v) for v in ['0', '-0.0', '1.23456789', '1e-300', '1.7976931348623157e308']],
        'ratio': [typed('f64', v) for v in ['0', '-0.0', '1.23456789', '1e-45', '3.4028234e38']],
        'birthday': [typed('date', v) for v in ['1970-01-01', '1969-12-31', '2024-02-29', '0001-01-01']],
        'updated_at': [typed('timestamp', v) for v in ['0', '-1', '1791692772000']],
        'id': [typed('u64', v) for v in ['0', '7', '4294967296', '18446744073709551615']],
        'version': [typed('i64', v) for v in ['0', '-3', '9223372036854775807', '-9223372036854775808']],
        'payload': [typed('null'), dict(kind='json', value=None), typed('json', False),
                    typed('json', {'雪': [0, False, None, 'text']}), typed('json', ['a', {'x': 2}])],
    }
    for name, values in fields.items():
        for loaded, initial, operand, op in itertools.product([False, True], values, values, ['none', 'update', 'overlay']):
            yield dict(kind='field', field=name, loaded=loaded, initial=initial, value=operand, op=op)
        for loaded, operand in itertools.product([False, True], [typed('null'), typed('typed_null'), typed('text', 'incompatible'), typed('bool', True)]):
            yield dict(kind='field', field=name, loaded=loaded, initial=values[-1], value=operand, op='overlay')


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
        raise SystemExit(f'{len(failures)} / {len(inputs)} generated entity observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producers = {name: json.loads((ROOT/f'tests/generator/parity/{name}_rust/PROVENANCE.json').read_text())
                 for name in ['types', 'relations']}
    corpus = dict(baseline=lock['commit'], version=lock['version'], producers=producers,
                  note='Direct original generated same-model getters, changed/eval, optional NULL, mutation overlays and cloned views; optional forward and reverse to-one query views. Exact observations, no output normalization.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_entity_helpers_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2)+'\n')
    print(f'Compared {len(inputs)} direct generated Rust/C++ observations')


if __name__ == '__main__':
    main()
