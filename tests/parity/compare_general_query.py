#!/usr/bin/env python3
import gzip
import json
from pathlib import Path
from compare_values import main
ROOT = Path(__file__).resolve().parents[2]
if __name__ == '__main__':
    with gzip.open(Path(__file__).with_name('general_query_5_0_6_cases.json.gz'), 'rt') as source:
        corpus = json.load(source)
    producer = json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    if corpus['producer'] != producer:
        raise SystemExit('General query producer provenance differs')
    raise SystemExit(main('general_query_5_0_6_cases.json.gz', 'General query'))
