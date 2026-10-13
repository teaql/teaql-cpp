#!/usr/bin/env python3
"""Replay unchanged example models and business operations with explicit intent."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from compare_values import observe

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    inputs = [dict(mode='schema'), dict(mode='relations')]
    rows = observe(args.rust, inputs)
    logs = observe(args.rust, [dict(item, logs=True) for item in inputs])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    sources = [
        '.reference/teaql-rs/examples/src/lib.rs',
        '.reference/teaql-rs/examples/src/bin/sqlite_schema_crud.rs',
        '.reference/teaql-rs/examples/src/bin/sqlite_relations_graph.rs',
        'tests/parity/rust_field_layout/src/bin/runtime_examples_5_0_6.rs',
    ]
    hashes = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in sources}
    note = ('Unchanged locked Rust example models and original business operations; '
            'the driver supplies required query comments at caller entry. This does not '
            'claim the unchanged stock example binaries accept missing request intent.')
    artifacts = {
        'runtime_examples_5_0_6_cases.json': dict(baseline=baseline, note=note,
            source_hashes=hashes, cases=[dict(input=item, expected=result)
                                      for item, result in zip(inputs, rows)]),
        'runtime_examples_provenance_5_0_6.json': dict(baseline=baseline, note=note,
            source_hashes=hashes, **{item['mode']: result for item, result in zip(inputs, logs)}),
    }
    for name, value in artifacts.items():
        path = HERE / name
        if args.check:
            if json.loads(path.read_text()) != value:
                raise SystemExit(f'{name}: recorded data differs from live original calls')
        else:
            path.write_text(json.dumps(value, indent=2) + '\n')
    print(f'Replayed {len(rows)} business scenarios and {sum(map(len, logs))} ordered SQL provenance entries')


if __name__ == '__main__':
    main()
