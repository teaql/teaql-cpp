#!/usr/bin/env python3
"""Reproduce the explicitly adapted inherited Business-ID source experiment."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_state_baselines_5_0_6 import versions

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tests/parity'))
from compare_values import same

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_inherited_business_binding_bridge_5_0_6'
TARGET = Path('/tmp/teaql_cpp_inherited_business_binding_bridge_506')
REPORT = ROOT / 'docs/parity/inherited_business_binding_bridge_5_0_6.json'
GOVERNED_REPORT = ROOT / 'docs/parity/inherited_business_governed_source_5_0_6.json'
CORPUS = PARITY / 'inherited_business_governed_adapted_source_5_0_6_cases.json.gz'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observe(executable, inputs, environment):
    result = subprocess.run([str(executable)], text=True, capture_output=True,
                            input=''.join(json.dumps(value) + '\n' for value in inputs),
                            env=environment)
    if result.returncode:
        raise RuntimeError(f'{executable} failed ({result.returncode}): {result.stderr}')
    output = [json.loads(line) for line in result.stdout.splitlines()]
    if len(output) != len(inputs):
        raise RuntimeError(f'Expected {len(inputs)} observations, got {len(output)}')
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--cpp', type=Path, help='Compare a successfully built C++ governed observer')
    parser.add_argument('--updates-cpp', type=Path,
                        help='Also compare inherited updates and failed-write retries')
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    subprocess.run([sys.executable, str(PARITY / 'build_inherited_business_binding_bridge_5_0_6.py')],
                   check=True)
    original_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    lock = PROJECT / 'Cargo.lock'
    expected = versions(original_lock)
    for name, values in versions(lock).items():
        if name in {'teaql_cpp_inherited_business_binding_bridge', 'cpp-inheritance-business-ids-core'}:
            continue
        if not values <= expected.get(name, set()):
            raise RuntimeError(f'Observer dependency differs from reference: {name}: {values}')
    # Dependency edge lists can differ with selected features. Compare the actual
    # registry name/version/source/checksum identities independently of edges.
    import re
    def identities(path):
        result = set()
        for block in path.read_text().split('[[package]]')[1:]:
            if '\nsource = "registry+' in block:
                result.add(tuple(re.search(rf'^{field} = "([^"]+)"', block, re.M)[1]
                                 for field in ('name', 'version', 'source', 'checksum')))
        return result
    if not identities(lock) <= identities(original_lock):
        raise RuntimeError('Observer registry identity/checksum differs from reference')
    environment = dict(os.environ, CARGO_TARGET_DIR=str(TARGET),
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off', TEAQL_BUSINESS_ID_KEY_VERSION='1',
                       TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())
    subprocess.run(['cargo', 'build', '--offline', '--locked', '--manifest-path',
                    str(PROJECT / 'Cargo.toml'), '--bins'], env=environment, check=True)
    result = subprocess.run([str(TARGET / 'debug/teaql_cpp_inherited_business_binding_bridge')],
                            text=True, capture_output=True, env=environment, check=True)
    output = json.loads(result.stdout)
    if len(output['models']) != 5 or not all(
            row['shared_type'] and row['initial_not_loaded'] and
            row['after_shared_update'] == 'replacement' for row in output['models']):
        raise RuntimeError('Five-model helper execution differs')
    report = json.loads((ROOT / 'build_inherited_business_binding_bridge_5_0_6/provenance.json')
                        .read_text())
    report.update(qualification_status='adapted compiler and five-model helper execution passed; '
                  'paired C++ acceptance remains pending', compile_exit_code=0, execution_exit_code=0,
                  source_registry_versions_preserved=True, source_dependency_checksums_preserved=True,
                  observer_sha256=digest(PROJECT / 'src/main.rs'), lock_sha256=digest(lock),
                  output=output, cpp_comparison=False, source_artifact_acceptance=False,
                  inheritance_guard_removed=False)
    inputs = [dict(model=model, name=name, number=number, fault=fault)
              for model in ('document', 'intermediate_record', 'invoice', 'receipt', 'special_invoice')
              for name in ('valid inherited object', '', 'x' * 101)
              for number in (None, 'invalid-number')
              for fault in ('none', 'trigger')]
    outputs = observe(TARGET / 'debug/governed', inputs, environment)
    if args.cpp:
        native = observe(args.cpp.resolve(), inputs, environment)
        gaps = [dict(index=index, input=value, rust=source, cpp=actual)
                for index, (value, source, actual) in enumerate(zip(inputs, outputs, native))
                if not same(source, actual)]
        if gaps:
            path = ROOT / 'docs/parity/inherited_business_governed_initial_gaps_5_0_6.json.gz'
            if not path.exists():
                path.write_bytes(gzip.compress((json.dumps(gaps, indent=2) + '\n').encode(), mtime=0))
            raise RuntimeError(f'{len(gaps)} adapted source/C++ governed-save differences; see {path}')
    corpus = dict(baseline=report['runtime_commit'], runtime_commit=report['runtime_commit'],
                  generator_commit=report['generator_commit'],
                  notice=report['adaptation'], untouched_producer_acceptance=False,
                  cpp_comparison=bool(args.cpp), observer_sha256=digest(PROJECT / 'src/bin/governed.rs'),
                  lock_sha256=digest(lock),
                  cases=[dict(input=value, expected=observed)
                         for value, observed in zip(inputs, outputs)])
    governed = dict(schema=1, runtime_commit=report['runtime_commit'],
                    generator_commit=report['generator_commit'], adapted_source_only=True,
                    observations=len(inputs), successes=sum(row['saved'] is not None for row in outputs),
                    errors=sum(row['error'] is not None for row in outputs),
                    corpus=str(CORPUS.relative_to(ROOT)), observer_sha256=corpus['observer_sha256'],
                    lock_sha256=digest(lock), reference_lock_sha256=digest(original_lock),
                    source_registry_versions_preserved=True, source_dependency_checksums_preserved=True,
                    cpp_comparison=bool(args.cpp), source_artifact_acceptance=False, inheritance_guard_removed=False)
    if args.cpp:
        governed['cpp_observer_sha256'] = digest(PARITY / 'inherited_business_governed_probe.cpp')
    if args.record:
        REPORT.write_text(json.dumps(report, indent=2) + '\n')
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n')
                                         .encode(), mtime=0))
        GOVERNED_REPORT.write_text(json.dumps(governed, indent=2) + '\n')
    else:
        if json.loads(REPORT.read_text()) != report:
            raise RuntimeError('Recorded helper bridge evidence differs')
        if json.loads(gzip.decompress(CORPUS.read_bytes())) != corpus:
            raise RuntimeError('Recorded adapted governed-save observations differ')
        if json.loads(GOVERNED_REPORT.read_text()) != governed:
            raise RuntimeError('Recorded governed-save evidence differs')
    comparison = 'exact adapted source/C++ outcomes' if args.cpp else 'adapted source observations'
    print(f'Five adapted source models and {len(inputs)} {comparison} reproduced; '
          'original artifact acceptance and complete inheritance qualification remain open')
    if args.updates_cpp:
        update_inputs = [dict(model=model, name='inherited object', number=None,
                              fault='trigger' if operation == 'retry' else 'none', operation=operation)
                         for model in ('document', 'intermediate_record', 'invoice', 'receipt', 'special_invoice')
                         for operation in ('rename', 'replace_number', 'same_number', 'retry')]
        source_updates = observe(TARGET / 'debug/updates', update_inputs, environment)
        native_updates = observe(args.updates_cpp.resolve(), update_inputs, environment)
        gaps = [dict(index=index, input=value, rust=source, cpp=actual)
                for index, (value, source, actual) in enumerate(
                    zip(update_inputs, source_updates, native_updates)) if not same(source, actual)]
        if gaps:
            path = ROOT / 'docs/parity/inherited_business_update_initial_gaps_5_0_6.json.gz'
            if not path.exists():
                path.write_bytes(gzip.compress((json.dumps(gaps, indent=2) + '\n').encode(), mtime=0))
            raise RuntimeError(f'{len(gaps)} inherited update/retry differences; see {path}')
        updates = dict(baseline=report['runtime_commit'], generator_commit=report['generator_commit'],
                       adaptation=report['adaptation'], untouched_producer_acceptance=False,
                       observer_sha256=digest(PROJECT / 'src/bin/updates.rs'),
                       cpp_observer_sha256=digest(PARITY / 'inherited_business_update_probe.cpp'),
                       lock_sha256=digest(lock),
                       cases=[dict(input=value, expected=observed)
                              for value, observed in zip(update_inputs, source_updates)])
        update_corpus = PARITY / 'inherited_business_update_adapted_source_5_0_6_cases.json.gz'
        update_record = ROOT / 'docs/parity/inherited_business_update_source_5_0_6.json'
        summary = dict(schema=1, baseline=report['runtime_commit'],
                       generator_commit=report['generator_commit'], observations=len(update_inputs),
                       follow_up_successes=sum(value['follow_up']['saved'] is not None for value in source_updates),
                       follow_up_errors=sum(value['follow_up']['error'] is not None for value in source_updates),
                       corpus=str(update_corpus.relative_to(ROOT)),
                       observer_sha256=updates['observer_sha256'], cpp_observer_sha256=updates['cpp_observer_sha256'],
                       original_files_unchanged=True, source_artifact_acceptance=False,
                       source_registry_versions_preserved=True, source_dependency_checksums_preserved=True,
                       normalization='none', inheritance_guard_removed=False)
        if args.record:
            update_corpus.write_bytes(gzip.compress((json.dumps(updates, separators=(',', ':')) + '\n')
                                                   .encode(), mtime=0))
            update_record.write_text(json.dumps(summary, indent=2) + '\n')
        else:
            if not same(json.loads(gzip.decompress(update_corpus.read_bytes())), updates):
                raise RuntimeError('Recorded inherited update/retry observations differ')
            if not same(json.loads(update_record.read_text()), summary):
                raise RuntimeError('Recorded inherited update/retry evidence differs')
        print(f'{len(update_inputs)} exact adapted source/C++ inherited update/retry outcomes reproduced')


if __name__ == '__main__':
    main()
