#!/usr/bin/env python3
"""Retain the exact private conflict classifier from frozen generated Rust."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'tests/generator/parity/candidates_rust/lib/src/runtime.rs'
DESTINATION = ROOT/'tests/parity/rust_field_layout/src/bootstrap_classifier_original.rs'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_generator_parity.py')], check=True)
    text = (ROOT/SOURCE).read_text()
    start = text.index('fn is_generated_bootstrap_retryable_conflict(')
    end = text.index('\n}\n', start)+3
    data = text[start:end]+'\n'
    provenance = json.loads((ROOT/'tests/generator/parity/candidates_rust/PROVENANCE.json').read_text())
    retained = dict(source=SOURCE, source_sha256=hashlib.sha256((ROOT/SOURCE).read_bytes()).hexdigest(),
                    extracted_sha256=hashlib.sha256(data.encode()).hexdigest(),
                    generator_commit=provenance['generator_commit'], runtime_commit=provenance['runtime_commit'],
                    note='Exact private generated function extracted without edits for a narrow classifier oracle; not a public bootstrap integration oracle.')
    manifest = DESTINATION.with_suffix('.json')
    if args.check:
        if not DESTINATION.is_file() or DESTINATION.read_text() != data or json.loads(manifest.read_text()) != retained:
            raise SystemExit('Retained original bootstrap classifier differs')
    else:
        DESTINATION.write_text(data)
        manifest.write_text(json.dumps(retained, indent=2)+'\n')
    print('Verified exact original bootstrap classifier' if args.check else 'Retained exact original bootstrap classifier')


if __name__ == '__main__':
    main()
