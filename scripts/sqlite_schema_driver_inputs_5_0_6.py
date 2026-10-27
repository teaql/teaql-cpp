#!/usr/bin/env python3
"""Reuse the original schema input recipe without reading expected outputs."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from observe_sqlite_schema import cases


def schema_inputs():
    return [row['input'] for row in cases()]
