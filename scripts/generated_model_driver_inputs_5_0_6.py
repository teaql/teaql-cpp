"""Complete generated-model stimuli; expected values are not selected."""
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DECLARED = {
    "generated_entity_helpers": [
        "tests/parity/generated_entity_helpers_5_0_6_cases.json",
        "fe8c2f0fa11288933907c56a6bd05b8e31778ea13f708254845fd89bbb4872c6"
    ],
    "generated_optional_fields": [
        "tests/parity/generated_optional_fields_5_0_6_cases.json.gz",
        "3c387e423801cdb94aa9a1bd7bd3fd80529000ac8f4960f742b780bc97091c84"
    ],
    "constant_governance": [
        "tests/parity/constant_governance_5_0_6_cases.json.gz",
        "389e722319e9a7e71e9c05833e1f2cfb7909d5efabd2cb5f6c8a930dfc850b34"
    ],
    "generated_constant_variants": [
        "tests/parity/generated_constant_variants_5_0_6_cases.json.gz",
        "03d5ec50507814f95fed12f2292b8813d286c0c855a8a3f472697c33f213fbff"
    ],
    "generated_bootstrap": [
        "tests/parity/generated_bootstrap_5_0_6_cases.json.gz",
        "bf80cde136d4c5c676becbc0e01ad0e93570c56cf1bcb3f4b1e618eddb654328"
    ],
    "generated_candidates": [
        "tests/parity/generated_candidates_5_0_6_cases.json.gz",
        "80976712e1f8f5310be51efb57d9cc995175573d94b11f3a58b90d209ce94af4"
    ],
    "generated_inheritance": [
        "tests/parity/generated_inheritance_5_0_6_cases.json.gz",
        "5e70eb175548fd5164b1cdaae0dc8256fd04196a84dafac261872f85ea84b118"
    ],
    "generated_inheritance_query": [
        "tests/parity/generated_inheritance_query_5_0_6_cases.json.gz",
        "4a95dd7dacfdb079397f6f42a9861bc3199442510b52489c5219d5dc1d6a39f3"
    ],
    "generated_inheritance_sqlite": [
        "tests/parity/generated_inheritance_sqlite_5_0_6_cases.json.gz",
        "1aa4f9c49bdc2aa8a27b8d357a02596ebf025e4a84272dca4cc06bc04481cccc"
    ],
    "generated_return_type": [
        "tests/parity/generated_return_type_5_0_6_cases.json.gz",
        "f7c2d260abcd8f50e9441756e42dc4c7d9a677269a0e619376843c01fec514b3"
    ],
    "generated_inheritance_save": [
        "tests/parity/generated_inheritance_save_5_0_6_cases.json.gz",
        "b3c97a91fe245733075995230a83c5e5aadf819ead2f1b871faff913f7677cbb"
    ],
    "generated_inheritance_graph": [
        "tests/parity/generated_inheritance_graph_5_0_6_cases.json.gz",
        "706669dc1d80d13054a62534e032f5ad345a7688b5bc5db1386c55c9b74b432f"
    ],
    "generated_inheritance_variants": [
        "tests/parity/generated_inheritance_variants_5_0_6_cases.json.gz",
        "eb2647ca2e3d1feddd0ef85b496a804726a60a2ba9a64697d57dbac4d8cecad0"
    ],
    "generated_inherited_stream": [
        "tests/parity/generated_inherited_stream_5_0_6_cases.json.gz",
        "abf457ea8b00c0f1109dd1938ad736e2b0e78001b96bddc53358fe791bfa55f2"
    ],
    "generated_checker_flow": [
        "tests/parity/generated_checker_flow_5_0_6_cases.json.gz",
        "d335e89bc75c1600b6fc7a201759c59b2174816de636a0b0a85d738f33c2cefd"
    ],
    "generated_checker_commit": [
        "tests/parity/generated_checker_commit_5_0_6_cases.json.gz",
        "ff7879e84ecdbf0f45edbd574daef8c5ed464668958861b2998c739d28f4e0b4"
    ],
    "generated_matching_any": [
        "tests/parity/generated_matching_any_5_0_6_cases.json.gz",
        "5400bd6502085a6c2280f83641cce8fa99ba356e4573c47d2d8744d7bf4d42bf"
    ],
    "generated_matching_conversion": [
        "tests/parity/generated_matching_conversion_5_0_6_cases.json.gz",
        "4b319806e4dee81bf5a1ffd7f68f4fe28132be7a8cff56ff3c5631cb57e32258"
    ],
    "generated_readonly_properties": [
        "tests/parity/generated_readonly_properties_5_0_6_cases.json.gz",
        "c610d4f8fd9aa6e03fe8db60e6ed84fe7d6799539846603b4a1e7ff0fd485c7e"
    ],
    "generated_dynamic_carrier": [
        "tests/parity/generated_dynamic_carrier_5_0_6_cases.json",
        "4334ac3a8af50e7de06a08b5c28539869c8ae6d7b1f176f11c51399ced6dd0ad"
    ],
    "generated_i18n": [
        "tests/parity/generated_i18n_5_0_6_cases.json.gz",
        "dffc51ea97d120e333a430a9e4fd3ad912e77664cb5a3ef1904676fbcce9d8de"
    ],
    "generated_runtime_assembly": [
        "tests/parity/generated_runtime_assembly_5_0_6_cases.json.gz",
        "753d91af6054dee9f6c1a6ce1e579625f823a6928919ca88497f9bc8a7a9f217"
    ],
    "generated_sample_execution": [
        "tests/parity/generated_sample_execution_5_0_6_cases.json.gz",
        "35ba3465ea6f92a2f8a4887db7a1d64c1d2a5c31a8b014060a93e79655fe15b4"
    ],
    "generated_sample_failures": [
        "tests/parity/generated_sample_failures_5_0_6_cases.json.gz",
        "d98dd03fa904f32a2c569ae03e4010e5e19b73fa0b9cbefc54a129b2524c253e"
    ],
    "generated_sample_state": [
        "tests/parity/generated_sample_state_5_0_6_cases.json",
        "a14557d239b5e71f863180ead2025eef6f24036598300069d7618f6c065f5781"
    ],
    "generated_expression_values": [
        "tests/parity/generated_expression_values_5_0_6_cases.json",
        "87c0cd775233d7a6dc49790fd2cda1a60ad09f9fe087fd67deecab9a87ae2ff5"
    ]
}


def declared_inputs(name):
    filename, expected = DECLARED[name]
    path = ROOT / filename
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected
    data = json.loads(gzip.decompress(raw) if path.suffix == ".gz" else raw)
    return [row["input"] for row in data["cases"]]


def generated_entity_helpers_inputs():
    return declared_inputs('generated_entity_helpers')


def generated_optional_fields_inputs():
    return declared_inputs('generated_optional_fields')


def constant_governance_inputs():
    return declared_inputs('constant_governance')


def generated_constant_variants_inputs():
    return declared_inputs('generated_constant_variants')


def generated_bootstrap_inputs():
    return declared_inputs('generated_bootstrap')


def generated_candidates_inputs():
    return declared_inputs('generated_candidates')


def generated_inheritance_inputs():
    return declared_inputs('generated_inheritance')


def generated_inheritance_query_inputs():
    return declared_inputs('generated_inheritance_query')


def generated_inheritance_sqlite_inputs():
    return declared_inputs('generated_inheritance_sqlite')


def generated_return_type_inputs():
    return declared_inputs('generated_return_type')


def generated_inheritance_save_inputs():
    return declared_inputs('generated_inheritance_save')


def generated_inheritance_graph_inputs():
    return declared_inputs('generated_inheritance_graph')


def generated_inheritance_variants_inputs():
    return declared_inputs('generated_inheritance_variants')


def generated_inherited_stream_inputs():
    return declared_inputs('generated_inherited_stream')


def generated_checker_flow_inputs():
    return declared_inputs('generated_checker_flow')


def generated_checker_commit_inputs():
    return declared_inputs('generated_checker_commit')


def generated_matching_any_inputs():
    return declared_inputs('generated_matching_any')


def generated_matching_conversion_inputs():
    return declared_inputs('generated_matching_conversion')


def generated_readonly_properties_inputs():
    return declared_inputs('generated_readonly_properties')


def generated_dynamic_carrier_inputs():
    return declared_inputs('generated_dynamic_carrier')


def generated_i18n_inputs():
    return declared_inputs('generated_i18n')


def generated_runtime_assembly_inputs():
    return declared_inputs('generated_runtime_assembly')


def generated_sample_execution_inputs():
    return declared_inputs('generated_sample_execution')


def generated_sample_failures_inputs():
    return declared_inputs('generated_sample_failures')


def generated_sample_state_inputs():
    return declared_inputs('generated_sample_state')


def generated_expression_values_inputs():
    return declared_inputs('generated_expression_values')
