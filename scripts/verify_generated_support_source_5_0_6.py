#!/usr/bin/env python3
"""Observe complete private generated-support state without modifying the reference."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
sys.path.insert(0, str(PARITY))
from compare_values import same
from generated_support_source_inputs_5_0_6 import cases


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--record-corpus', action='store_true')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    args = parser.parse_args()
    args.record = args.record.resolve()
    corpus = PARITY / 'generated_support_source_5_0_6_cases.json.gz'
    if args.record.exists() or (args.record_corpus and corpus.exists()):
        parser.error('Use fresh evidence paths')
    record = dict(complete=False, whole_source_accepted=False, full_goal_accepted=False,
                  steps=[], frontends={}, scope='Complete executable/private tree state and '
                  'source traversal boundaries; full service trait/delegation review remains separate.')
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
               TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(),
                                             compresslevel=1, mtime=0))

    def run(command, payload=None):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, input=payload,
                                text=True, capture_output=True)
        # Observation output is retained once per matrix rather than serialized
        # again into every subsequent command checkpoint.
        record['steps'].append(dict(command=command, exit_code=result.returncode,
                                    stdout=result.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                                    stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    run(['python3', 'scripts/verify_upstream.py'])
    inputs = cases()
    with tempfile.TemporaryDirectory(prefix='teaql_support_private_source_506_') as directory:
        directory = Path(directory)
        observer = directory / 'observer.rs'
        fixture = PARITY / 'query_full_state_5_0_6.rs'
        appendix = PARITY / 'generated_support_source_5_0_6.rs'
        observer.write_text('#[cfg(test)]\nmod generated_support_source_contracts_506 {\n' +
                            fixture.read_text() + '\n' + appendix.read_text() + '\n}\n')
        view = ReferenceRuntimeView(ROOT, directory / 'runtime',
                                    {'src/generated_support.rs': observer}, run)
        input_file, output_file = directory / 'inputs.json', directory / 'outputs.json'
        input_file.write_text(json.dumps(inputs))
        env.update(TEAQL_SUPPORT_SOURCE_INPUTS=str(input_file),
                   TEAQL_SUPPORT_SOURCE_OUTPUTS=str(output_file))
        output = run(['cargo', 'test', '--offline', '--locked', '--jobs', '2',
                      '--manifest-path', view.directory / 'Cargo.toml', '--lib',
                      'generated_support_source_contracts_506::',
                      '--', '--nocapture', '--test-threads=1'])
        assert '2 passed; 0 failed; 0 ignored;' in output
        original = json.loads(output_file.read_text())
        assert len(original) == len(inputs)
        record['source_view'] = view.provenance()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    data = dict(baseline=baseline, cases=[dict(input=i, expected=o)
                                       for i, o in zip(inputs, original)])
    compressed = gzip.compress((json.dumps(data, separators=(',', ':'))+'\n').encode(), mtime=0)
    if args.record_corpus:
        corpus.write_bytes(compressed)
    else:
        assert corpus.read_bytes() == compressed, 'Source observations changed'
    record['corpus_sha256'] = digest(corpus)
    save()
    for label, build, prefix in [('native', args.build_dir.resolve(), 'teaql_')] + (
            [('modules', args.module_build_dir.resolve(), 'teaql_module_')]
            if args.module_build_dir else []):
        target = prefix + 'generated_support_source_probe'
        run(['cmake', '--build', build, '--target', target, '--parallel', '4'])
        binary = (build / 'modules' if label == 'modules' else build) / target
        payload = ''.join(json.dumps(value)+'\n' for value in inputs)
        output = run([binary], payload)
        actual = [json.loads(line) for line in output.rstrip('\n').split('\n')]
        differences = [dict(index=i, input=inputs[i], original=a, actual=b)
                       for i, (a, b) in enumerate(zip(original, actual)) if not same(a, b)]
        record['frontends'][label] = dict(observations=len(actual), differences=differences,
                                          binary_sha256=digest(binary), outputs=actual)
        save()
        assert len(actual) == len(original) and not differences
        names = [prefix_name + name for name in ('generated_support_source_rust_parity',
                 'generated_support_source_context_contracts', 'generated_support_contracts')
                 for prefix_name in (('module_' if label == 'modules' else ''),)]
        junit = args.record.with_name(args.record.name + '.' + label + '.xml')
        run(['ctest', '--test-dir', build, '--output-on-failure', '--no-tests=error',
             '--output-junit', junit, '-R', '^('+'|'.join(names)+')$'])
        tests = ET.parse(junit).getroot().findall('.//testcase')
        assert {test.attrib['name'] for test in tests} == set(names)
        assert not any(test.find(tag) is not None for test in tests
                       for tag in ('failure', 'error', 'skipped'))
        record['frontends'][label]['junit'] = junit.read_text()
        assert 'no work to do' in run(['cmake', '--build', build, '--target', target,
                                      '--parallel', '4'])
        print(label, len(actual), 'exact source outcomes; three contracts passed', flush=True)
    record.update(complete=True, observations_per_frontend=len(inputs), normalization='none',
                  hashes={str(path.relative_to(ROOT)): digest(path) for path in (
                      Path(__file__).resolve(), PARITY / 'generated_support_source_5_0_6.rs',
                      PARITY / 'generated_support_source_probe.cpp',
                      PARITY / 'generated_support_source_inputs_5_0_6.py',
                      PARITY / 'query_full_state_5_0_6.rs',
                      ROOT / 'tests/support/query_full_state_fixture.hpp')})
    save()


if __name__ == '__main__':
    main()
