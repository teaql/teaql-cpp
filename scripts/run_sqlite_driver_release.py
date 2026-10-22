#!/usr/bin/env python3
"""Build the matched driver benchmark in an owned Release directory and reclaim it."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs', type=int, default=2)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    receipt = root / 'docs/parity/sqlite_driver_release_build_5_0_6.json.gz'
    report = {'profile': 'native preset with Release override', 'steps': [],
              'free_bytes_before': shutil.disk_usage(root).free}
    source = root / 'teaql_provider_sqlite/tests/driver_query_benchmark.cpp'
    report['source_sha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
    inputs = ('teaql_provider_sqlite/tests/driver_query_benchmark.cpp',
              'tests/support/sqlite_driver_benchmark_fixture.hpp',
              'tests/support/generated_allocation_counter.hpp',
              'scripts/verify_sqlite_driver_samples.py', 'toolchains.lock.json',
              'upstream.lock.json')
    def hashes():
        return {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in inputs}
    report['input_hashes_before'] = hashes()
    status = 1
    try:
        with tempfile.TemporaryDirectory(prefix='teaql_sqlite_driver_release_') as directory:
            build = Path(directory)
            report['temporary_directory'] = directory
            commands = [
                ['cmake', '--preset', 'native', '-B', directory, '-DCMAKE_BUILD_TYPE=Release'],
                ['cmake', '--build', directory, '--target', 'teaql_sqlite_driver_query_benchmark',
                 '-j', str(args.jobs)],
                ['python3', 'scripts/verify_sqlite_driver_samples.py', '--cpp',
                 str(build / 'teaql_sqlite_driver_query_benchmark'), '--build-type', 'Release',
                 '--output', 'docs/parity/sqlite_driver_release_samples_5_0_6.json.gz'],
            ]
            try:
                for command in commands:
                    print('Running:', ' '.join(command), flush=True)
                    started = time.monotonic()
                    result = subprocess.run(command, cwd=root, text=True, capture_output=True)
                    report['steps'].append({'command': command, 'exit_code': result.returncode,
                                            'seconds': time.monotonic() - started,
                                            'stdout': result.stdout, 'stderr': result.stderr})
                    if result.returncode:
                        raise RuntimeError(f'command failed: {command[0]} ({result.returncode})')
                status = 0
            finally:
                cache = build / 'CMakeCache.txt'
                if cache.exists():
                    report['cmake_cache'] = cache.read_text()
                report['reclaimed_bytes'] = sum(p.stat().st_size for p in build.rglob('*')
                                                 if p.is_file() and not p.is_symlink())
    except Exception as error:
        report['error'] = str(error)
    report['input_hashes_after'] = hashes()
    if report['input_hashes_before'] != report['input_hashes_after']:
        status = 1
        report['error'] = 'Benchmark qualification inputs changed during execution'
    report['exit_code'] = status
    report['temporary_directory_removed'] = not Path(report.get('temporary_directory', '/nonexistent/teaql_release')).exists()
    report['free_bytes_after'] = shutil.disk_usage(root).free
    with gzip.open(receipt, 'wt') as stream:
        json.dump(report, stream, indent=2)
    print(f'Release gate exit={status}; receipt={receipt}', flush=True)
    return status


if __name__ == '__main__':
    raise SystemExit(main())
