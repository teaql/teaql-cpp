#!/usr/bin/env python3
"""Run the frozen archive verifier with its assertions active under Python -O."""

import hashlib
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
FROZEN_SOURCE = ROOT / 'scripts/archive_driver_provenance_5_0_6.py'
FROZEN_SHA256 = '720bd4c6f154c0b13eaa5d6fe6aa6742bd8adc9e20f9765321991557c3dba509'


def load_checked_archive_driver():
    source = FROZEN_SOURCE.read_bytes()
    if hashlib.sha256(source).hexdigest() != FROZEN_SHA256:
        raise RuntimeError('Frozen archive driver source changed')
    module = ModuleType('archive_driver_provenance_5_0_6_checked')
    module.__file__ = str(FROZEN_SOURCE)
    exec(compile(source, str(FROZEN_SOURCE), 'exec', optimize=0), module.__dict__)
    return module


if __name__ == '__main__':
    load_checked_archive_driver().main()
