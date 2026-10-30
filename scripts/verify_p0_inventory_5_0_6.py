#!/usr/bin/env python3
"""Audit the complete locked P0 source map and its current C++ target paths."""

import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT.parent / 'teaql_code_gen_cpp'
RUNTIME_REFERENCE = ROOT / '.reference/teaql-rs'
GENERATOR_REFERENCE = ROOT / '.reference/teaql_code_gen'
RUNTIME_COMMIT = 'fe82a18982415f1746df9a2cf7c42f5e9abdc6d3'
GENERATOR_COMMIT = '45e1572c4956ca963410d8ea436ead3c8857087a'
PREVIOUS_RUNTIME_COMMIT = '62940ae2b88e7acc32763853b8b4685a01c7e90a'
PREVIOUS_GENERATOR_COMMIT = '77b301e9b29a4928aabebdbe2ccc6d03d618f247'
CRATES = ['teaql-core', 'teaql-data-service', 'teaql-sql',
          'teaql-runtime', 'teaql-provider-sqlite']
EXTERNAL = {'teaql-runtime/src/telemetry_opentelemetry.rs',
            'teaql-runtime/tests/otlp_smoke.rs'}
HANDWRITTEN = [
    'examples/context_lifetime.cpp',
    'examples/custom_checker_behavior.cpp',
    'examples/generated_query_expression.cpp',
    'examples/models.hpp',
    'examples/module_imports.hpp',
    'examples/scenarios.hpp',
    'examples/sqlite_relations_graph.cpp',
    'examples/sqlite_schema_crud.cpp',
]


def require(condition, message):
    if not condition:
        raise RuntimeError('P0 inventory: ' + message)


def read(path):
    return json.loads(path.read_text())


def digest(data):
    return hashlib.sha256(data).hexdigest()


def git(directory, *arguments):
    return subprocess.check_output(['git', '-C', str(directory), *arguments])


def expand_braces(value):
    group = re.search(r'\{([^{}]+)\}', value)
    if group is None:
        return [value]
    return [item for part in group.group(1).split(',')
            for item in expand_braces(value[:group.start()] + part + value[group.end():])]


def target_paths(value):
    return [Path(item) for segment in value.split(';')
            for item in expand_braces(segment.strip())]


def check_existing_verifier(name):
    result = subprocess.run([sys.executable, '-E', '-B', str(ROOT / name)], cwd=ROOT,
                            capture_output=True, text=True, check=False)
    require(result.returncode == 0,
            'existing verifier failed: ' + name + ': ' + result.stderr[-1200:])
    return result.stdout.strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    require(args.record is None or not args.record.exists(), 'preserve previous receipt')
    require(sys.flags.optimize == 0, 'run the inventory gate with active checks')

    runtime_lock = read(ROOT / 'upstream.lock.json')
    generator_lock = read(GENERATOR / 'cpp_generator.lock.json')
    ledger = read(ROOT / 'docs/cpp_generator_parity_ledger.json')
    inventory = read(ROOT / 'docs/parity/v5_0_6/upstream_inventory.json')
    disposition_path = ROOT / 'docs/parity/v5_0_6/source_dispositions.json'
    dispositions = read(disposition_path)
    transition_path = ROOT / 'docs/parity/v5_0_6/source_disposition_path_transition.json'
    transition = read(transition_path)
    require(runtime_lock['version'] == '5.0.6' and runtime_lock['tag'] == 'v5.0.6'
            and runtime_lock['commit'] == RUNTIME_COMMIT
            and inventory['commit'] == dispositions['commit'] == RUNTIME_COMMIT
            and ledger['runtime_commit'] == generator_lock['runtime_rust_baseline']
            == RUNTIME_COMMIT, 'runtime lock or inventory differs')
    require(ledger['generator_commit'] == generator_lock['generator_baseline_commit']
            == GENERATOR_COMMIT
            and generator_lock['runtime_rust_version'] == '5.0.6'
            and ledger['reference_worktree'] == '.reference/teaql_code_gen',
            'generator lock differs')
    require(dispositions['previous_commit'] == PREVIOUS_RUNTIME_COMMIT
            and ledger['previous_generator_commit']
            == generator_lock['generator_workspace_base_commit']
            == PREVIOUS_GENERATOR_COMMIT
            and ledger['runtime_upgrade_audit'] == 'docs/baseline_5_0_6_upgrade.md'
            and (ROOT / ledger['runtime_upgrade_audit']).is_file(),
            'historical-to-current version audit changed')
    for reference, commit in [(RUNTIME_REFERENCE, RUNTIME_COMMIT),
                              (GENERATOR_REFERENCE, GENERATOR_COMMIT)]:
        require(git(reference, 'rev-parse', 'HEAD').decode().strip() == commit
                and not git(reference, 'status', '--porcelain'),
                'locked reference changed: ' + str(reference))
    git(RUNTIME_REFERENCE, 'cat-file', '-e', PREVIOUS_RUNTIME_COMMIT + '^{commit}')
    git(GENERATOR_REFERENCE, 'cat-file', '-e', PREVIOUS_GENERATOR_COMMIT + '^{commit}')
    require(not git(GENERATOR, 'status', '--porcelain')
            and git(GENERATOR, 'branch', '--show-current').decode().strip()
            == 'cpp_support'
            and git(GENERATOR, 'merge-base', '--is-ancestor',
                    generator_lock['generator_workspace_base_commit'], 'HEAD') == b'',
            'independent cpp_support workspace changed or lost its base')

    source_names = set(ledger['sources'])
    capabilities = ledger['capabilities']
    ids = {entry['id'] for entry in capabilities}
    mapped = set().union(*(set(entry['rust_sources']) for entry in capabilities))
    require(len(source_names) == 42 and len(capabilities) == 17
            and ids == {f'G{number:02d}' for number in range(1, 18)}
            and mapped == source_names,
            'generator source or capability mapping incomplete')
    for name, entry in ledger['sources'].items():
        require(digest((GENERATOR_REFERENCE / name).read_bytes()) == entry['sha256'],
                'locked generator source changed: ' + name)

    inventory_files = {item['path']: item for item in inventory['files']}
    source_rows = {item['source']: item for item in dispositions['sources']}
    require(inventory['crates'] == CRATES and len(inventory_files) == 124
            and len(dispositions['sources']) == 124
            and set(source_rows) == set(inventory_files),
            'runtime source inventory or disposition set changed')
    status = Counter(item['status'] for item in source_rows.values())
    require(status == {'accepted': 122, 'excluded_external': 2}
            and {name for name, item in source_rows.items()
                 if item['status'] == 'excluded_external'} == EXTERNAL,
            'runtime dispositions or edge exclusions changed')
    for name, row in source_rows.items():
        require(row['sha256'] == inventory_files[name]['sha256']
                == digest((RUNTIME_REFERENCE / name).read_bytes()),
                'runtime source hash changed: ' + name)
        if row['status'] == 'accepted':
            require(row.get('evidence') and all((ROOT / item).is_file()
                                                for item in row['evidence']),
                    'accepted source has missing evidence: ' + name)

    require(transition['schema'] == 1
            and transition['runtime_commit'] == RUNTIME_COMMIT
            and transition['historical_dispositions_sha256']
            == digest(disposition_path.read_bytes())
            and len(transition['updates']) == 1,
            'historical source ledger transition changed')
    update = transition['updates'][0]
    require(update == {
        'source': 'teaql-core/src/lib.rs',
        'old_cpp_target': 'teaql_core/include/teaql/core.hpp; tests/parity/core',
        'current_cpp_target': 'teaql_core/include/teaql/core.hpp; tests/parity/teaql_core',
    } and source_rows[update['source']]['cpp_target'] == update['old_cpp_target'],
            'module-move target correction changed')
    historical_missing = []
    current_missing = []
    target_count = 0
    for name, row in source_rows.items():
        if row['status'] == 'excluded_external':
            require(row['cpp_target'].startswith('none (external '),
                    'external owner has a C++ target: ' + name)
            continue
        original_target = row['cpp_target']
        current_target = (update['current_cpp_target'] if name == update['source']
                          else original_target)
        for target in target_paths(original_target):
            if not (ROOT / target).exists():
                historical_missing.append((name, str(target)))
        for target in target_paths(current_target):
            target_count += 1
            if not (ROOT / target).exists():
                current_missing.append((name, str(target)))
    require(historical_missing == [('teaql-core/src/lib.rs', 'tests/parity/core')]
            and not current_missing and target_count == 382,
            'active C++ source target map is incomplete')

    fixture_manifest = read(ROOT / 'tests/generator/parity/provenance.json')
    fixtures = fixture_manifest['fixtures']
    require(len(fixtures) == 4, 'representative model fixture set changed')
    for fixture in fixtures:
        source_ref = (RUNTIME_REFERENCE if fixture['source_commit'] == RUNTIME_COMMIT
                      else GENERATOR_REFERENCE)
        require(fixture['source_commit'] in (RUNTIME_COMMIT, GENERATOR_COMMIT)
                and set(fixture['capabilities']) <= ids,
                'model fixture source or responsibility changed')
        data = (ROOT / 'tests/generator/parity' / fixture['fixture']).read_bytes()
        require(digest(data) == fixture['sha256']
                and data == git(source_ref, 'show', fixture['source_commit']
                                + ':' + fixture['source']),
                'representative model fixture changed: ' + fixture['fixture'])
    for name in HANDWRITTEN:
        require((ROOT / name).is_file()
                and git(ROOT, 'ls-files', '--error-unmatch', name).strip(),
                'handwritten example baseline missing: ' + name)

    verifier_results = {
        name: check_existing_verifier(name)
        for name in ['scripts/verify_upstream.py',
                     'scripts/verify_generator_parity.py',
                     'scripts/verify_module_paths.py']
    }

    report = dict(complete=True, scope='P0 locked inventory and target mapping only',
                  verifier_sha256=digest(Path(__file__).read_bytes()),
                  runtime_commit=RUNTIME_COMMIT, generator_commit=GENERATOR_COMMIT,
                  generator_sources=len(source_names), mapped_generator_sources=len(mapped),
                  capabilities=len(capabilities), runtime_sources=len(source_rows),
                  runtime_status=dict(status), external_sources=sorted(EXTERNAL),
                  representative_fixtures=len(fixtures),
                  handwritten_examples=len(HANDWRITTEN),
                  expanded_current_targets=target_count,
                  historical_missing_targets=historical_missing,
                  current_missing_targets=current_missing,
                  historical_dispositions_sha256=digest(disposition_path.read_bytes()),
                  transition_sha256=digest(transition_path.read_bytes()),
                  existing_verifiers=verifier_results,
                  whole_goal_complete=False)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(report, indent=2) + '\n').encode(),
                                          mtime=0))
    print('P0 inventory: 42 generator sources, 17 capabilities, 124 runtime '
          'sources and 382 current C++ targets mapped')


if __name__ == '__main__':
    main()
