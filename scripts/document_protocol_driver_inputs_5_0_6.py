"""Complete immutable historical stimuli for current reference-protocol replay."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def historical_inputs(name, expected):
    path = ROOT / 'tests/parity' / name
    assert hashlib.sha256(path.read_bytes()).hexdigest() == expected
    # Historical expected values are deliberately not returned. The untouched
    # current Rust executable produces new results from these declared inputs.
    return [row['input'] for row in json.loads(path.read_text())['cases']]


def round_trip_reference_inputs():
    return historical_inputs('round_trip_reference_cases.json',
        '54ef79605c6174ae0667c51c23cbd3f630419cc006dcf5e24c71ef864040f932')


def entity_reference_inputs():
    return historical_inputs('entity_reference_cases.json',
        'afcac340aa52a0369e27f395f41c2500733507bfdfd681fb6dfa81f584e91965')
