#!/usr/bin/env python3
"""Compare all retained original graph-outcome model observations verbatim."""
import argparse
import gzip
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--original', type=Path, default=ROOT / 'docs/parity/graph_outcome_models_original_5_0_6.json.gz')
    args = parser.parse_args()
    original = json.loads(gzip.decompress(args.original.read_bytes()))
    if not original['complete'] or len(original['observations']) != 15:
        raise SystemExit('Incomplete original graph-outcome evidence')
    provenance = original['provenance']
    if not provenance['all_original_bodies_preserved'] or provenance['source_files_sha256'].get('src/data_service/graph.rs') != '552c99766ae688454029c212dfa01f4e6ee27e836e955fa00fe34a8b3b8ca952':
        raise SystemExit('Graph-outcome evidence has a different source baseline')
    result = subprocess.run([args.cpp], capture_output=True, text=True, check=True)
    observed = [json.loads(line) for line in result.stdout.splitlines()]
    if observed != original['observations']:
        for index, (expected, actual) in enumerate(zip(original['observations'], observed)):
            if expected != actual:
                print(json.dumps({'index': index, 'expected': expected, 'actual': actual}, indent=2))
                break
        raise SystemExit(f'Graph-outcome observations differ: expected 15, observed {len(observed)}')
    print('Matched all 15 original graph-outcome model observations')


if __name__ == '__main__':
    main()
