"""Admit exact active comparator copies while retaining historical contracts."""

import hashlib
from pathlib import Path


TRANSITIONS = {
    "tests/parity/compare_generated_expression_diagnostics.py": (
        "bd066186381094eec002198205930e40fafc80b709d34c99003bca70dab8f66c",
        "tests/parity/compare_generated_expression_diagnostics_hardened_5_0_6.py",
        "bed54afa685f87e0d1505260cd4fdde4a88fd86afd81f0ffa0168502d84c1a69",
    ),
    "tests/parity/compare_generated_expression_values.py": (
        "17894017dc552c2147d308a89a036ea807d7f0fd552812ba83fba7d55a23e763",
        "tests/parity/compare_generated_expression_values_hardened_5_0_6.py",
        "7ce9d86d348ed69c0666122857765bb1f36d10a9518793c3945f14213bb26fef",
    ),
    "tests/parity/compare_generated_runtime_assembly.py": (
        "d86c1f07f94d1a57d1c802620af94236a22cccf1116ce1342b89c42cab384814",
        "tests/parity/compare_generated_runtime_assembly_hardened_5_0_6.py",
        "4a5af0e30ccc70d9b8dba13e7e0c3b56599f44ed021fb0b8b7d49f78e725ed94",
    ),
    "tests/parity/compare_generated_sample_execution.py": (
        "06dbd193bed464b3faa40b429c7be4c7ea4f3440bb66220690a4f9a79bc652b3",
        "tests/parity/compare_generated_sample_execution_hardened_5_0_6.py",
        "18bc515a5320c2bb81936fe481de37448d0456783899ca473beee2ba7cfee515",
    ),
    "tests/parity/compare_generated_sample_failures.py": (
        "07631a38c962aac7456cbd56a597a99eab24e2a26ac0bee26a5d7f8b1f43ea06",
        "tests/parity/compare_generated_sample_failures_hardened_5_0_6.py",
        "bba5d7f6644248264286dff0035450e169c38435d1c8296ff8e1ef53f2ed08b8",
    ),
    "tests/parity/compare_generated_sample_state.py": (
        "16da65576541f3d46b3db051a9f3af38606779c09c6695524442f0d56b4a896d",
        "tests/parity/compare_generated_sample_state_hardened_5_0_6.py",
        "045a8d4df4954c95200f952f9dd0c737e0d5da694bb88bd8917fbbfff83d925c",
    ),
    "tests/parity/compare_sqlite_from_sql_raw_debug.py": (
        "38cac95ee1dc71d49181185a831f3b8f0b2db7802f97f2fba3f205e0baca6f81",
        "tests/parity/compare_sqlite_from_sql_raw_debug_hardened_5_0_6.py",
        "e37c46c6107f2deaae5148e5d2f2a5c32a014e1c1f6551ace5c1dea025bdb8f8",
    ),
}


def current_comparator(root, historical):
    if historical in {row[1] for row in TRANSITIONS.values()}:
        raise RuntimeError('Expected a historical comparator identity')
    if historical not in TRANSITIONS:
        return historical
    before_hash, current, after_hash = TRANSITIONS[historical]
    root = Path(root)
    for relative, wanted in ((historical, before_hash), (current, after_hash)):
        if hashlib.sha256((root / relative).read_bytes()).hexdigest() != wanted:
            raise RuntimeError('Comparator source transition changed: ' + relative)
    return current
