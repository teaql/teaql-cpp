#!/usr/bin/env python3
"""Isolated processes observe real stdout/stderr/files and upstream lazy state."""
import argparse
import json
import gzip
import os
import re
import subprocess
import tempfile
from pathlib import Path
from compare_values import same

RUST_HEADER = '👉 Output as structured JSON:   Set TEAQL_LOG_FORMAT=json (for ELK/Datadog)'
CPP_HEADER = '👉 Debug text output:           Set TEAQL_LOG_FORMAT=debug or json (both select DebugReaderFormatter)'


def normalize(text):
    # Only the volatile Human UTC prefix and the documented header correction.
    text = re.sub(r'^\[[0-9+-]+ [0-9:]+\.[0-9]{3}\](?=-\[)', '[TIME]', text, flags=re.M)
    return text.replace(RUST_HEADER, CPP_HEADER)


def observe(executable, inputs, emulator=()):
    executable = Path(executable).resolve()
    if not executable.is_file():
        raise FileNotFoundError(f'Probe executable is missing; build its target first: {executable}')
    outputs = []
    environment = {k: v for k, v in os.environ.items() if not k.startswith('TEAQL_')}
    for fixture in inputs:
        with tempfile.TemporaryDirectory(prefix='teaql-log-manager-') as directory:
            result = subprocess.run([*emulator, str(executable)], input=json.dumps(fixture) + '\n',
                                    cwd=directory, env=environment, text=True,
                                    capture_output=True, check=True)
            files = {}
            for path in sorted(Path(directory).rglob('*')):
                if path.is_file():
                    name = str(path.relative_to(directory))
                    if name == executable.name + '.log':
                        name = 'executable.log'
                    files[name] = normalize(path.read_text())
            outputs.append({'stdout': normalize(result.stdout), 'stderr': result.stderr, 'files': files})
    return outputs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    corpus = json.loads(gzip.decompress(Path(__file__).with_name('log_manager_baseline_5_0_6_cases.json.gz').read_bytes()))
    if corpus['baseline'] != json.loads((root / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Log manager observations belong to a different baseline')
    inputs = [c['input'] for c in corpus['cases']]
    expected = [c['expected'] for c in corpus['cases']]
    if args.rust:
        subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], cwd=root, check=True)
        if not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Stored manager observations differ from live Rust')
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else []
    actual = observe(args.cpp, inputs, emulator)
    failures = [(i, a, b) for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    for i, a, b in failures[:5]:
        print(f'Case {i}: {inputs[i]}\nRust: {str(a)[:3000]}\nC++: {str(b)[:3000]}')
    print(f'Log manager parity: {len(inputs) - len(failures)}/{len(inputs)} observations match')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
