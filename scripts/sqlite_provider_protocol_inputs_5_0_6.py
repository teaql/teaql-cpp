"""Complete declared SQLite provider stimuli; expected values are not selected."""
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DECLARED = {
    'sqlite_dynamic_storage': ('tests/parity/sqlite_dynamic_storage_5_0_6_cases.json',
        '210860b33333e878658d9f50d905d17922b01ad2a86e444d68d364f6d3b4c1e5'),
    'sqlite_dynamic_storage_boundaries': ('tests/parity/sqlite_dynamic_storage_boundaries_5_0_6_cases.json.gz',
        '4f3e82d734b56a7ccf614f9f20762ed0b90b3871de92e3a982b7005b95bb107f'),
    'sqlite_provider_business_id': ('tests/parity/sqlite_business_id_5_0_6_cases.json.gz',
        '639e12ef89a85de5db5d11b6c278576f7bd411c620d8269636d9ab267a36544f'),
    'sqlite_provider_id': ('tests/parity/sqlite_id_5_0_6_cases.json.gz',
        '1c92235233abaa247d9a4526ac7f6ddd575ab1fe9f690aa062b69b66233dfc83'),
    'sqlite_sync_transaction': ('tests/parity/sqlite_sync_transaction_5_0_6_cases.json.gz',
        'd4a2840382959a5a7b674fd508b36dfe5db6e9cf726fa371b334ff4a0e880496'),
    'sqlite_provider_transport': ('tests/parity/sqlite_transport_5_0_6_cases.json.gz',
        '33fc95044fa7e2ce75a9c309c12b5a7744d78ed18eb22474f5564becf9cbe95c'),
    'sqlite_bindings': ('tests/parity/sqlite_bindings_5_0_6_cases.json.gz',
        '6a0b4dfef8c18b63e0555ff86e0e187b01044ddb9e161c69a1788edb44afe0fc'),
    'sqlite_error_model': ('tests/parity/sqlite_error_model_5_0_6_cases.json.gz',
        '308a54e1382733becb5d8fb8495cd701b45c705b5abc3b5067396fa175108091'),
    'sqlite_native_diagnostics': ('tests/parity/sqlite_native_diagnostics_5_0_6_cases.json.gz',
        '65fe1f5c2876f4dbb62f6ce925ddbd662c3666bbc88e6583ddcebc09d5e63db6'),
    'sqlite_simple_error': ('tests/parity/sqlite_simple_error_5_0_6_cases.json.gz',
        'bfdb27048c2a6a7fd5e3b3abe62f6957d20d5550c824e8e1b5e4aaf4dc05e909'),
    'sqlite_from_sql_error': ('tests/parity/sqlite_from_sql_error_5_0_6_cases.json.gz',
        '196276a97a14d54645ce212878f22f3fd392aab85e46e6a41fb97768842ea849'),
}


def declared_inputs(name):
    filename, expected = DECLARED[name]
    path = ROOT / filename
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected
    data = json.loads(gzip.decompress(raw) if path.suffix == ".gz" else raw)
    return [row["input"] for row in data["cases"]]


def sqlite_dynamic_storage_inputs():
    return declared_inputs('sqlite_dynamic_storage')


def sqlite_dynamic_storage_boundaries_inputs():
    return declared_inputs('sqlite_dynamic_storage_boundaries')


def sqlite_provider_business_id_inputs():
    return declared_inputs('sqlite_provider_business_id')


def sqlite_provider_id_inputs():
    return declared_inputs('sqlite_provider_id')


def sqlite_sync_transaction_inputs():
    return declared_inputs('sqlite_sync_transaction')


def sqlite_provider_transport_inputs():
    return declared_inputs('sqlite_provider_transport')


def sqlite_bindings_inputs():
    return declared_inputs('sqlite_bindings')


def sqlite_error_model_inputs():
    return declared_inputs('sqlite_error_model')


def sqlite_native_diagnostics_inputs():
    return declared_inputs('sqlite_native_diagnostics')


def sqlite_simple_error_inputs():
    return declared_inputs('sqlite_simple_error')


def sqlite_from_sql_error_inputs():
    return declared_inputs('sqlite_from_sql_error')
