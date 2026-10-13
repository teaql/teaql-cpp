#!/usr/bin/env python3
"""Verify locked generator references and ledger integrity, not behavioral parity."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
GENERATED_FIXTURES = ['business_ids', 'types', 'relations', 'optional_types',
                      'checkers', 'inheritance', 'inheritance_graph', 'inheritance_variants',
                      'inheritance_business_ids', 'candidates', 'constants_scalar', 'constants_root',
                      'constants_numeric', 'constants_catalog', 'dynamic_json', 'sample_fields', 'sample_fields_runtime', 'sample_fields_temporal']


def git(reference, *arguments):
    return subprocess.check_output(['git', '-C', str(reference), *arguments])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generator-dir', type=Path,
                        default=ROOT.parent / 'teaql_code_gen_cpp')
    parser.add_argument('--reference', type=Path,
                        default=ROOT / '.reference/teaql_code_gen')
    parser.add_argument('--require-complete', action='store_true')
    parser.add_argument('--skip-business-id-fixture', action='store_true',
                        help='For controlled original-fixture regeneration only')
    parser.add_argument('--regenerating-fixtures', nargs='+', choices=GENERATED_FIXTURES,
                        default=[], help='Skip only the specified artifacts during controlled regeneration')
    args = parser.parse_args()
    runtime = json.loads((ROOT / 'upstream.lock.json').read_text())
    producer = json.loads((args.generator_dir / 'cpp_generator.lock.json').read_text())
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    if ledger['runtime_commit'] != runtime['commit']:
        raise SystemExit('Generator ledger uses a different runtime baseline')
    if (ledger['generator_commit'] != producer['generator_baseline_commit'] or
            producer['runtime_rust_baseline'] != runtime['commit']):
        raise SystemExit('Generator lock and capability ledger disagree')
    if git(args.reference, 'rev-parse', 'HEAD').decode().strip() != ledger['generator_commit']:
        raise SystemExit('Generator reference HEAD differs from its lock')
    if git(args.reference, 'status', '--porcelain').strip():
        raise SystemExit('Generator reference must be clean')
    for path, entry in ledger['sources'].items():
        data = git(args.reference, 'show', ledger['generator_commit'] + ':' + path)
        if hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise SystemExit(f'Stale locked generator source: {path}')
        lines = data.decode().splitlines()
        for declaration in entry['public_declarations']:
            if lines[declaration['line'] - 1].strip() != declaration['source']:
                raise SystemExit(f'Stale source anchor: {path}:{declaration["line"]}')
    provenance = json.loads(git(ROOT / '.reference/teaql-rs', 'show',
                               runtime['commit'] + ':' + ledger['release_generated_fixture']))
    if (provenance['producerCommit'] != ledger['generator_commit'] or
            provenance['runtimeVersion'] != runtime['version'] or provenance['handEdited']):
        raise SystemExit('Release fixture provenance differs from the generator/runtime locks')
    allowed = {'missing', 'partial', 'rejected', 'pending_audit', 'pending_review',
               'implemented', 'qualified'}
    ids = set()
    pending = []
    for entry in ledger['capabilities']:
        if entry['id'] in ids or entry['status'] not in allowed:
            raise SystemExit(f'Duplicate or invalid capability: {entry["id"]}')
        ids.add(entry['id'])
        if not set(entry['rust_sources']) <= ledger['sources'].keys():
            raise SystemExit(f'Unindexed source reference: {entry["id"]}')
        for path, digest in entry.get('runtime_source_hashes', {}).items():
            data = git(ROOT / '.reference/teaql-rs', 'show', runtime['commit'] + ':' + path)
            if hashlib.sha256(data).hexdigest() != digest:
                raise SystemExit(f'Stale runtime dependency: {entry["id"]}:{path}')
        if entry['status'] == 'qualified':
            if not entry['cpp_api_mapping'] or not entry['evidence']:
                raise SystemExit(f'Qualification needs API mappings and evidence: {entry["id"]}')
            for evidence in entry['evidence']:
                if not (ROOT / evidence).is_file():
                    raise SystemExit(f'Missing capability evidence: {evidence}')
        else:
            pending.append(entry['id'])
    fixtures = json.loads((ROOT / 'tests/generator/parity/provenance.json').read_text())
    for entry in fixtures['fixtures']:
        path = ROOT / 'tests/generator/parity' / entry['fixture']
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise SystemExit(f'Changed locked model input: {entry["fixture"]}')
        reference = (ROOT / '.reference/teaql-rs' if entry['source_commit'] == runtime['commit']
                     else args.reference)
        original = git(reference, 'show', entry['source_commit'] + ':' + entry['source'])
        if data != original or not set(entry['capabilities']) <= ids:
            raise SystemExit(f'Invalid model input provenance: {entry["fixture"]}')
    generated = ROOT / 'tests/generator/parity/business_ids_rust'
    if (args.skip_business_id_fixture or args.regenerating_fixtures) and args.require_complete:
        raise SystemExit('Completion cannot skip generated fixture verification')
    if args.skip_business_id_fixture:
        print(f'Verified locked sources; original Business-ID fixture is being regenerated')
        return
    for fixture in GENERATED_FIXTURES:
        if fixture in args.regenerating_fixtures:
            continue
        generated = ROOT / 'tests/generator/parity' / f'{fixture}_rust'
        provenance = json.loads((generated / 'PROVENANCE.json').read_text())
        if (provenance['generator_commit'] != ledger['generator_commit'] or
                provenance['runtime_commit'] != runtime['commit'] or provenance['hand_edited'] or
                hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() !=
                provenance['model_sha256']):
            raise SystemExit(f'{fixture} original generated fixture provenance differs')
        actual = {path.relative_to(generated).as_posix() for path in generated.rglob('*')
                  if path.is_file() and path.name != 'PROVENANCE.json'}
        if actual != set(provenance['files']):
            raise SystemExit(f'{fixture} original generated fixture file set differs')
        for path, digest in provenance['files'].items():
            if hashlib.sha256((generated / path).read_bytes()).hexdigest() != digest:
                raise SystemExit(f'Changed original generated {fixture} fixture: {path}')
    if args.require_complete and pending:
        raise SystemExit('Unqualified generator capabilities: ' + ', '.join(pending))
    print(f'Verified {len(ledger["sources"])} locked generator sources and '
          f'{len(ids)} capability records; {len(pending)} remain unqualified')


if __name__ == '__main__':
    main()
