#!/usr/bin/env python3
"""Compile actual original/C++ entities after ignored character expressions."""
from pathlib import Path
from xml.sax.saxutils import escape
import hashlib

import audit_generated_privacy_contracts_5_0_6 as compiled
import audit_container_metadata_contracts_5_0_6 as descriptors


def scenarios():
    base = descriptors.scenarios()[0][1]
    expressions = [
        ('unicode_character', "'雪'"),
        ('supplementary_character', "'🧊'"),
        ('ascii_escape', r"'\x7f'"),
        ('unicode_escape', r"'\u{10FFFF}'"),
        ('byte_escape', r"b'\xff'"),
        ('byte_pattern', r"|b'\xff'| 1"),
        ('byte_const_default', r"for<const N: usize=b'\xff'> || 1"),
    ]
    return [(name, base.replace('_audit_value_max_len="17"',
            '_audit_value_max_len="' + escape(
                '17, audit_value_max_len=' + expression + ', audit_value_max_len=21',
                {'"': '&quot;'}) + '"')) for name, expression in expressions]


if __name__ == '__main__':
    compiled.main(scenarios=scenarios(), rust_main=descriptors.RUST_MAIN,
                  cpp_main=descriptors.CPP_MAIN,
                  scope='Complete original generated entities and C++ descriptors after character, '
                        'byte-character, closure-pattern and const-default expressions; '
                        'lexical rejection is covered by the separate original/parser matrix.',
                  extra_provenance={'driver_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()})
