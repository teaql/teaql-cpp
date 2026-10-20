#!/usr/bin/env python3
"""Record exact source/fixture bodies for current runtime root registrations.

This is a source and registration audit. Executed tests and semantic review are
separate evidence; a matching name or body hash is not behavioral acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'teaql-runtime/src/lib.rs'
OLD = '62940ae2b88e7acc32763853b8b4685a01c7e90a'
CURRENT = 'fe82a18982415f1746df9a2cf7c42f5e9abdc6d3'
OUTPUT = ROOT / 'docs/parity/v5_0_6/runtime_root_contracts.json'
RUST_RAW = re.compile(r'(?:b)?r(#{0,255})"')
CPP_RAW = re.compile(r'(?:u8|[uUL])?R"([^ ()\\\t\r\n]{0,16})\(')
CHAR_LITERAL = re.compile(r"'(?:[^'\\\n]|\\(?:u\{[0-9a-fA-F]+\}|x[0-9a-fA-F]{2}|.))'")


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def source_at(commit):
    return subprocess.check_output([
        'git', '-C', str(ROOT / '.reference/teaql-rs'), 'show', f'{commit}:{SOURCE}'], text=True)


def balanced_end(source, start):
    """Find a brace block end, ignoring Rust/C++ strings and nested comments."""
    if source[start] != '{':
        raise ValueError('Block must start with a brace')
    pos, depth = start, 0
    while pos < len(source):
        if source.startswith('//', pos):
            end = source.find('\n', pos)
            pos = len(source) if end < 0 else end + 1
            continue
        if source.startswith('/*', pos):
            pos += 2
            nesting = 1
            while nesting:
                if pos >= len(source):
                    raise ValueError('Unterminated block comment')
                if source.startswith('/*', pos):
                    nesting += 1
                    pos += 2
                elif source.startswith('*/', pos):
                    nesting -= 1
                    pos += 2
                else:
                    pos += 1
            continue
        rust_raw = RUST_RAW.match(source, pos)
        cpp_raw = CPP_RAW.match(source, pos)
        if rust_raw or cpp_raw:
            raw = rust_raw or cpp_raw
            marker = '"' + raw[1] if rust_raw else ')' + raw[1] + '"'
            end = source.find(marker, raw.end())
            if end < 0:
                raise ValueError('Unterminated raw string')
            pos = end + len(marker)
            continue
        if source[pos] == '"':
            pos += 1
            while pos < len(source):
                if source[pos] == '\\':
                    pos += 2
                elif source[pos] == '"':
                    pos += 1
                    break
                else:
                    pos += 1
            continue
        # A Rust lifetime such as 'a is not a character literal.
        char = CHAR_LITERAL.match(source, pos)
        if char:
            pos = char.end()
            continue
        if source[pos] == '{':
            depth += 1
        elif source[pos] == '}':
            depth -= 1
            if not depth:
                return pos + 1
        pos += 1
    raise ValueError('Unterminated function body')


def function(source, name, rust):
    pattern = (rf'^[ \t]*(?:async\s+)?fn\s+{re.escape(name)}\s*\([^{{;]*\)\s*\{{'
               if rust else
               rf'^[ \t]*{"int" if name == "main" else "static void"}\s+{re.escape(name)}\s*\([^{{;]*\)\s*\{{')
    matches = list(re.finditer(pattern, source, re.M))
    if len(matches) != 1:
        raise ValueError(f'Expected one function definition for {name}, found {len(matches)}')
    match = matches[0]
    start = source.rfind('{', match.start(), match.end())
    body = source[start:balanced_end(source, start)]
    return dict(line=source.count('\n', 0, match.start()) + 1, body=body)


def build():
    lock = json.loads((ROOT / 'upstream.lock.json').read_text())
    if lock['commit'] != CURRENT:
        raise RuntimeError('This audit is reviewed for the locked 5.0.6 source only')
    inventory = json.loads((ROOT / 'docs/parity/v5_0_6/upstream_inventory.json').read_text())
    if inventory['commit'] != CURRENT:
        raise RuntimeError('Active source inventory is stale')
    indexed = next(f for f in inventory['files'] if f['path'] == SOURCE)
    historical_path = ROOT / 'docs/module_runtime_root_contracts.json'
    historical = json.loads(historical_path.read_text())
    dispositions = json.loads((ROOT / 'docs/parity/source_test_dispositions.json').read_text())
    if historical['baseline'] != OLD or dispositions['commit'] != OLD:
        raise RuntimeError('Historical reviewed root records changed baseline')
    previous = {row['name']: row for row in historical['tests']}
    reviews = {row['name']: row for row in dispositions['tests'] if row['source'] == SOURCE}
    names = {row['name'] for row in indexed['tests']}
    if names != previous.keys() or names != reviews.keys():
        raise RuntimeError('Added/removed root tests require explicit mapping review')
    contracts = json.loads((ROOT / 'modules/contracts.json').read_text())['contracts']
    by_check = {'module_' + row['stem']: row for row in contracts if row['stem'].startswith('original_runtime_')}
    old_text, current_text = source_at(OLD), source_at(CURRENT)
    if digest(current_text) != indexed['sha256']:
        raise RuntimeError('Current root source differs from committed inventory')
    rows = []
    for item in indexed['tests']:
        name = item['name']
        old, current = function(old_text, name, True), function(current_text, name, True)
        inherited = previous[name]
        check = inherited['module_checks'][0]
        review = reviews[name]
        if check == 'module_original_runtime_optimistic_conflict_output_contracts':
            fixture = ROOT / 'tests/parity/teaql_runtime/optimistic_conflict_output.py'
            body = fixture.read_text()
            native = 'original_runtime_optimistic_conflict_output_contracts'
            fixture_line = 1
            calls = ['memory_data_service_supports_mutations_and_optimistic_locking']
        else:
            contract = by_check[check]
            fixture = ROOT / contract['source']
            text = fixture.read_text()
            parsed = function(text, name, False)
            body, fixture_line = parsed['body'], parsed['line']
            entry_body = function(text, 'main', False)['body']
            if not re.search(rf'^\s*{re.escape(name)}\(\);', entry_body, re.M):
                raise RuntimeError(f'Root counterpart is not called by its fixture entry: {name}')
            native = contract['root_check']
            calls = [name]
        if native not in review['checks']:
            raise RuntimeError(f'Historical review and actual native registration disagree: {name}')
        rows.append(dict(source=SOURCE, name=name, line=item['line'],
                         source_body_sha256=digest(current['body']),
                         historical_line=inherited['line'], historical_body_sha256=digest(old['body']),
                         body_changed=current['body'] != old['body'],
                         cpp_fixture=str(fixture.relative_to(ROOT)), cpp_line=fixture_line,
                         cpp_file_sha256=digest(fixture.read_text()), cpp_body_sha256=digest(body),
                         fixture_calls=calls, native_checks=[native], module_checks=[check],
                         historical_review=review['review_note'],
                         status='current_source_and_fixture_locators_reviewed_not_full_leaf_acceptance'))
    return dict(schema=1, baseline=CURRENT, previous_baseline=OLD,
                note='Exact committed root tests and actual called C++ fixture bodies. Registration '
                     'and hashes are structural evidence; executed test/review evidence is recorded '
                     'separately. Historical contracts remain unchanged. No complete runtime parity claim.',
                source_sha256=digest(current_text), historical_source_sha256=digest(old_text),
                historical_ledger_sha256=digest(historical_path.read_text()),
                tests=rows, changed_bodies=sum(row['body_changed'] for row in rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    report = build()
    text = json.dumps(report, indent=2) + '\n'
    if args.record:
        OUTPUT.write_text(text)
    elif OUTPUT.read_text() != text:
        raise RuntimeError('Runtime root source/fixture ledger is stale')
    print(f"Verified {len(report['tests'])} current root locators; "
          f"{report['changed_bodies']} source bodies changed; not full leaf acceptance")


if __name__ == '__main__':
    main()
