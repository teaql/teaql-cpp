#!/usr/bin/env python3
"""Audit actual public request names; declaration coverage is not semantic parity."""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
ADAPTATIONS = {
    'query': 'coreQuery',
    'into_query': 'intoQuery',
    'select_dynamic_fields_with': 'selectDynamicFields',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    source = ROOT / 'tests/generator/parity/query_field_branches_rust/lib/src/entry/request.rs'
    cpp = ROOT / 'tests/generated_query_field_branches/include/cpp_query_branches/queries.hpp'
    provenance = json.loads((source.parents[3] / 'PROVENANCE.json').read_text())
    assert hashlib.sha256(source.read_bytes()).hexdigest() == provenance['files'][
        source.relative_to(source.parents[3]).as_posix()]
    original = sorted(set(re.findall(r'pub\s+(?:async\s+)?fn\s+(\w+)', source.read_text())))
    text = cpp.read_text()
    # Do not match the forward declaration and accidentally inspect another class.
    match = re.search(r'class EntryRequest\s*\{', text)
    assert match
    start = text.index('{', match.start())
    end, depth = start + 1, 1
    while depth:
        if text[end] == '{':
            depth += 1
        elif text[end] == '}':
            depth -= 1
        end += 1
    body = text[start:end]
    mappings = {}
    for name in original:
        words = name.split('_')
        target = ADAPTATIONS.get(name, words[0] + ''.join(w.capitalize() for w in words[1:]))
        mappings[name] = target
    missing = {name: target for name, target in mappings.items()
               if not re.search(r'\b' + re.escape(target) + r'\s*\(', body)}
    report = dict(source_file=source.relative_to(ROOT).as_posix(),
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  cpp_file=cpp.relative_to(ROOT).as_posix(),
                  cpp_sha256=hashlib.sha256(cpp.read_bytes()).hexdigest(),
                  mappings=mappings, missing=missing, adaptations=ADAPTATIONS,
                  inventory_only=True, semantic_acceptance=False)
    if args.report:
        assert not args.report.exists(), 'Preserve historical declaration reviews'
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'Public request declaration inventory: {len(original) - len(missing)}/{len(original)}; '
          'semantic acceptance remains separate')
    assert not missing, missing


if __name__ == '__main__':
    main()
