"""Complete declared SQL/SQLite stimuli; current Rust independently supplies results."""
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DECLARED = {
    'sql_log_query_source': ('tests/parity/sql_log_query_source_5_0_6_cases.json.gz',
        '5e1d2abfd6cb98c000bc706581e557160f9f107d3c9c2d9b37b15fc94b3bce31'),
    'sqlite_chunk_capacity': ('tests/parity/sqlite_chunk_capacity_5_0_6_cases.json.gz',
        '5d1226738688e6492b9fdfaf119bf5d179b3840077694951acab60ba3c9a3089'),
    'sqlite_lease_binding': ('tests/parity/sqlite_lease_binding_5_0_6_cases.json.gz',
        '09d962c5396b746e7d0b51fcf8b90245f5e230d82ee192a9af4a61bdc8c79a83'),
    'sqlite_lease_binding_guard': ('tests/parity/sqlite_lease_binding_guard_5_0_6_cases.json.gz',
        '68019373109a248f0dd3212e7ee309ffb88714f057d3cf3b4039e33ae3fe9b0b'),
    'sqlite_async_schedule': ('tests/parity/sqlite_async_schedule_5_0_6_cases.json.gz',
        '2b6ef7b9494256458befa9dd82c8eacf53f7f002f3fcebdab398440b10935859'),
    'sqlite_async_lease': ('tests/parity/sqlite_async_lease_5_0_6_cases.json',
        'e0d969a91210a6be64ac1971e47720b3c9298c5901ffc11fd7636553d789f6ac'),
    'sql_execution_5_0_6': ('tests/parity/sql_execution_5_0_6_cases.json.gz',
        '4f85b5f051ad683884fe4597adbcb3d01108dfffbee0ea0dce2b7fb25dd8e9bb'),
    'sql_dialect_helpers': ('tests/parity/sql_dialect_helper_5_0_6_cases.json.gz',
        '12c83f48810dd0d75c07944327488261def11d48a9b4b13986c6f3ce12f1f832'),
    'sql_dialect_in': ('tests/parity/sql_dialect_in_5_0_6_cases.json.gz',
        '16c1be704baf2b97f18151bd4f58ab1a54f94410dde38e3e35102975746091d1'),
    'sql_grouped_partition': ('tests/parity/sql_grouped_partition_5_0_6_cases.json.gz',
        '61cf9733af10c83548a855ebc8ef8444bed0cb83d7402e29aa68c1fa2c6183e6'),
    'sql_projection_scope': ('tests/parity/sql_projection_scope_5_0_6_cases.json.gz',
        '963a21d064a271eba7e63b5d4f56114041b835dcfa617797989195a6fc88e836'),
    'sql_schema': ('tests/parity/sql_schema_5_0_6_cases.json.gz',
        'e7fc84266e70cd0f8a4e07de01fbb7536d3f068d22ccda13896bc71fcee42bd9'),
    'sql_mutation': ('tests/parity/sql_mutation_5_0_6_cases.json.gz',
        'b0c10a437844a18ad0faf76ced80f8b7605363735a0cf307296cefe68939c616'),
    'sql_dialect_hooks': ('tests/parity/sql_dialect_hook_5_0_6_cases.json.gz',
        'a1ee7a9586eb7728f3a2fa66867a44288ecdd4bc293c809e3b1dbc257babecde'),
    'sql_dialect': ('tests/parity/sql_dialect_5_0_6_cases.json.gz',
        '388a6620c577a41bab6f140465314aedd9a50f899d505c86e417de2eb1e04dd7'),
    'sql_bindings_source': ('tests/parity/sql_bindings_source_5_0_6_cases.json.gz',
        '5aeeb70a2e5ee23e2a176d117312f14c1b0292be19ad7ce85658b871eb8eec15'),
    'sql_types_boundaries': ('tests/parity/sql_types_boundaries_5_0_6_cases.json.gz',
        'cc816a6502967c46003545066a22fa6d6e1d435d9e8db755438b32932999a2e1'),
    'sql_type_models_boundaries': ('tests/parity/sql_type_models_boundaries_5_0_6_cases.json.gz',
        '69518743ded92ebc94803d324350504550907abc753a3ed67bb452012b4b62fc'),
}


def declared_inputs(name):
    filename, expected = DECLARED[name]
    path = ROOT / filename
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected
    data = json.loads(gzip.decompress(raw) if path.suffix == ".gz" else raw)
    return [row["input"] for row in data["cases"]]


def sql_log_query_source_inputs():
    return declared_inputs('sql_log_query_source')


def sqlite_chunk_capacity_inputs():
    return declared_inputs('sqlite_chunk_capacity')


def sqlite_lease_binding_inputs():
    return declared_inputs('sqlite_lease_binding')


def sqlite_lease_binding_guard_inputs():
    return declared_inputs('sqlite_lease_binding_guard')


def sqlite_async_schedule_inputs():
    return declared_inputs('sqlite_async_schedule')


def sqlite_async_lease_inputs():
    return declared_inputs('sqlite_async_lease')


def sql_execution_5_0_6_inputs():
    return declared_inputs('sql_execution_5_0_6')


def sql_dialect_helpers_inputs():
    return declared_inputs('sql_dialect_helpers')


def sql_dialect_in_inputs():
    return declared_inputs('sql_dialect_in')


def sql_grouped_partition_inputs():
    return declared_inputs('sql_grouped_partition')


def sql_projection_scope_inputs():
    return declared_inputs('sql_projection_scope')


def sql_schema_inputs():
    return declared_inputs('sql_schema')


def sql_mutation_inputs():
    return declared_inputs('sql_mutation')


def sql_dialect_hooks_inputs():
    return declared_inputs('sql_dialect_hooks')


def sql_dialect_inputs():
    return declared_inputs('sql_dialect')


def sql_bindings_source_inputs():
    return declared_inputs('sql_bindings_source')


def sql_types_boundaries_inputs():
    return declared_inputs('sql_types_boundaries')


def sql_type_models_boundaries_inputs():
    return declared_inputs('sql_type_models_boundaries')
