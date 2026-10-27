#!/usr/bin/env python3
"""Pin complete retained Rust 5.0.6 sample and configuration observations."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
RECORD_SHA256 = {
    'tests/parity/sample_types_5_0_6_cases.json.gz':
        '36c77f06f17ab898b9df7ec5c6c2b9fd3d80aaac997c95322e7216b57fc2334a',
    'tests/parity/sample_types_5_0_6_focused_cases.json.gz':
        '19f32e7ec87cda4e566e035dfb57c92626ce8a24b4d9f50f01e95947ead38bf6',
    'tests/parity/custom_sample_5_0_6_cases.json.gz':
        '5d86b3a4a166ce9c618b32d372491b3bef4d35f3d8d9642910564f7a5e601e25',
    'tests/parity/custom_sample_5_0_6_focused_cases.json.gz':
        'ef42329daacb6730b9e7e52e3785d886b25c01b4543c629abf306d65f1088b3f',
    'tests/parity/catalog_sample_5_0_6_cases.json.gz':
        'b1b61503c44d603837d4c0c5a582d3b5f20879c5fe5afdda14f2b36b5851aadb',
    'tests/parity/catalog_sample_5_0_6_focused_cases.json.gz':
        'b29930af2174fc333ef2cdf4a2b2bd313631f676e15dae8121ddaa5cfa006e41',
    'tests/parity/business_sample_5_0_6_cases.json.gz':
        'db29f37ce2d27d48cecfffbba6bee7c50889e45c8f3f95886fe1b7828699db4d',
    'docs/parity/business_runtime_configuration_original_5_0_6.json':
        '39727fcf1a511350b188885083ce9a0137e0c7e90c658c453e8fb916a406d5b9',
    'docs/parity/business_environment_source_boundaries_5_0_6.json.gz':
        'f26bc77f9a2ba10abeeb9e9fea9a8b02d3512f77c8e2035807bcb295398f393d',
    'docs/parity/business_environment_source_filesystem_5_0_6.json.gz':
        '973022d1b5c54e07e95cb7ac5e4a1242115b916bbf0bb55eb41783a871a3e548',
}


def verify_record(path):
    path = Path(path).resolve()
    name = path.relative_to(ROOT).as_posix()
    assert name in RECORD_SHA256, name + ': unqualified original observation record'
    assert hashlib.sha256(path.read_bytes()).hexdigest() == RECORD_SHA256[name], (
        name + ': original observation record changed')


def main():
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_upstream.py')], check=True)
    subprocess.run([sys.executable, str(ROOT / 'scripts/verify_generator_parity.py')], check=True)
    for name in RECORD_SHA256:
        verify_record(ROOT / name)
    with tempfile.TemporaryDirectory(prefix='teaql_legacy_observation_control_') as owned:
        fake_root = Path(owned)
        for name in RECORD_SHA256:
            path = fake_root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes((ROOT / name).read_bytes() + b'\n')
            with patch.dict(globals(), ROOT=fake_root):
                try:
                    verify_record(path)
                except AssertionError:
                    continue
            raise AssertionError(name + ': modified original observation accepted')
    print(f'Verified {len(RECORD_SHA256)} original observation identities and '
          f'{len(RECORD_SHA256)} changed-record rejections')


if __name__ == '__main__':
    main()
