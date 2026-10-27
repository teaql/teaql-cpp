#!/usr/bin/env python3
"""Recover the preceding error comparators and CMake test registry exactly."""

import gzip
import hashlib
import json
from pathlib import Path

from verify_runtime_error_cli_admission_5_0_6 import MILESTONE, versions

ROOT = Path(__file__).resolve().parents[2]
PREVIOUS = ROOT / 'docs/parity/helper_cli_previous_input_delta_5_0_6.json.gz'


def main():
    snapshot = json.loads(gzip.decompress(MILESTONE.read_bytes()))['input_hashes']
    comparator_versions = versions(snapshot)
    previous = json.loads(gzip.decompress(PREVIOUS.read_bytes()))
    expected = previous['changed_existing']['runtime:tests/CMakeLists.txt']['after']
    current = (ROOT / 'tests/CMakeLists.txt').read_text()
    marker = '\n\n# Rebind the retained complete runtime-error source execution to current native CLIs.\n'
    assert current.count(marker) == 1
    recovered = current.split(marker, 1)[0] + '\n'
    assert hashlib.sha256(recovered.encode()).hexdigest() == expected
    wrapper = ROOT / 'tests/parity/compare_runtime_check.py'
    assert hashlib.sha256(wrapper.read_bytes()).hexdigest() == snapshot[
        'runtime:' + wrapper.relative_to(ROOT).as_posix()]
    print('Recovered', len(comparator_versions),
          'preceding comparator implementations, wrapper and CMake registry byte for byte')


if __name__ == '__main__':
    main()
