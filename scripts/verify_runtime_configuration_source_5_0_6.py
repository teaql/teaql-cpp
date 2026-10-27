#!/usr/bin/env python3
"""Verify complete retained Rust 5.0.6 configuration records and source inputs."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_runtime_configuration_kernel_5_0_6 import SOURCE_RECORDS, load_original_record


def main():
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    cases = 0
    source_paths = set()
    for name in SOURCE_RECORDS:
        record = load_original_record(name)
        cases += len(record['cases'])
        source_paths.update(record['source_inputs'])
    print(f'Verified {len(SOURCE_RECORDS)} complete original configuration records, '
          f'{cases} cases and {len(source_paths)} distinct source inputs')


if __name__ == '__main__':
    main()
