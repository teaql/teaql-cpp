"""Compare current intent-complete examples with separately replayed 5.0.6 data."""
import hashlib
import json
from pathlib import Path
from compare_runtime_examples import main

if __name__ == '__main__':
    root = Path(__file__).resolve().parents[2]
    here = Path(__file__).resolve().parent
    corpus = json.loads((here / 'runtime_examples_5_0_6_cases.json').read_text())
    provenance = json.loads((here / 'runtime_examples_provenance_5_0_6.json').read_text())
    baseline = json.loads((root / 'upstream.lock.json').read_text())['commit']
    if corpus['baseline'] != baseline or provenance['baseline'] != baseline:
        raise SystemExit('Current example rows/provenance do not belong to the active baseline')
    if corpus['source_hashes'] != provenance['source_hashes']:
        raise SystemExit('Example rows and provenance have different source authority')
    for name, digest in corpus['source_hashes'].items():
        if hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
            raise SystemExit(f'Stale current example source: {name}')
    raise SystemExit(main('runtime_examples_5_0_6_cases.json',
                         'runtime_examples_provenance_5_0_6.json'))
