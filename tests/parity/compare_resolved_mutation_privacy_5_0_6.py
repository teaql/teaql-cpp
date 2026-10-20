"""Compare actual mutation privacy propagation and version boundaries."""
import gzip
import json
from pathlib import Path
from compare_values import main
from resolved_mutation_privacy_inputs_5_0_6 import assert_privacy_contract


if __name__ == '__main__':
    corpus=Path(__file__).with_name('resolved_mutation_privacy_5_0_6_cases.json.gz')
    assert_privacy_contract(json.loads(gzip.decompress(corpus.read_bytes()))['cases'])
    raise SystemExit(main('resolved_mutation_privacy_5_0_6_cases.json.gz', 'Resolved mutation privacy 5.0.6'))
