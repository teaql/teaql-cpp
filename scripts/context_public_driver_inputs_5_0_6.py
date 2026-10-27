"""Complete declared public-Context stimuli; no expected-output selection."""
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DECLARED = {
    "context_locking": [
        "tests/parity/locking_baseline_5_0_6_cases.json.gz",
        "0021bfa65f327404e8264c3389c99aa7935d1b35756829515555b7b5d7d99b56"
    ],
    "audit_event_projection_5_0_6": [
        "tests/parity/audit_event_projection_5_0_6_cases.json.gz",
        "d46938d184af4ca28cb54afe50611902e15916cba99eed2aca75ef17c1b437d7"
    ],
    "i18n_debug_scope": [
        "tests/parity/i18n_debug_scope_baseline_5_0_6_cases.json.gz",
        "b913fcc734f2e8be2f73fb24f3f851f297d4765bf0514b5cd087dac773302f32"
    ],
    "i18n": [
        "tests/parity/i18n_baseline_5_0_6_cases.json.gz",
        "0eafc7fd72fd5617f249fb28f1d1a99d94bd81aa424b044f80de8d26cc614db4"
    ],
    "telemetry_model_scope": [
        "tests/parity/telemetry_model_scope_baseline_5_0_6_cases.json.gz",
        "0c214c0d7e8f84f53c7eca5ce67f37a07865de6e2c661735cfb9fcc5f6d8c198"
    ],
    "commit_audit_error": [
        "tests/parity/commit_audit_error_5_0_6_cases.json",
        "0b841766a06086e739f7bab782cdc2ca1c41c4c05cd6941520e77681327f0943"
    ],
    "intent_trace": [
        "tests/parity/intent_trace_5_0_6_cases.json",
        "9c19848f1cf9693c1ebc8632737b603c6060cc2215b605c99c843299e8b20771"
    ],
    "telemetry": [
        "tests/parity/telemetry_baseline_5_0_6_cases.json.gz",
        "183e95c3b6772f92684a67532bc2f2322477e9e9c02247f92b49eb8668261416"
    ],
    "context_logging": [
        "tests/parity/context_logging_baseline_5_0_6_cases.json.gz",
        "dace5e991827a995b3582530ea0019240584bfd0abac71150097e5ac43e6e1d9"
    ],
    "runtime_log": [
        "tests/parity/runtime_log_baseline_5_0_6_cases.json.gz",
        "c34eb0fa39cac03788d94bb6b107bd0657865c54455eeb5dddf1d5ae2269a65d"
    ],
    "business_service": [
        "tests/parity/business_service_baseline_5_0_6_cases.json.gz",
        "999b3c07758c58fc1eeee65819841369e1d1309b4a734ad15241ab4bc82979c8"
    ],
    "runtime_business_id": [
        "tests/parity/runtime_business_id_baseline_5_0_6_cases.json.gz",
        "e15f58a09471b0fd83dd41493f460827bef17680bd7995127c753890949b0e5f"
    ],
    "runtime_context": [
        "tests/parity/runtime_context_baseline_5_0_6_cases.json.gz",
        "97a476a4f8dd62889886b2aca934091b6795b513768aba1e73d5520dfda074a2"
    ],
    "generated_support": [
        "tests/parity/public_facets_5_0_6_cases.json.gz",
        "9728a3a11e6a0cb0271da09a9acd0e3f5d576f52d35f5655e25f610eb2702541"
    ],
    "public_facet_logs": [
        "tests/parity/public_facet_logs_5_0_6_cases.json.gz",
        "5af45b9238a799565de111422d52d96d29c83dc55ed9713b66d9ca05ab222632"
    ],
    "purposed_query_debug": [
        "tests/parity/purposed_query_debug_5_0_6_cases.json.gz",
        "764cf02249d9372603271403d76922616213f5d851d8b87dc50bcba43f71da5d"
    ],
    "data_store": [
        "tests/parity/data_store_5_0_6_cases.json.gz",
        "1185ca49e5349886fa7c17336a8fd167bc6721896a69d8ee42bae481127588dc"
    ]
}


def declared_inputs(name):
    filename, expected = DECLARED[name]
    path = ROOT / filename
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected
    data = json.loads(gzip.decompress(raw) if path.suffix == ".gz" else raw)
    return [row["input"] for row in data["cases"]]


def context_locking_inputs():
    return declared_inputs('context_locking')


def audit_event_projection_5_0_6_inputs():
    return declared_inputs('audit_event_projection_5_0_6')


def i18n_debug_scope_inputs():
    return declared_inputs('i18n_debug_scope')


def i18n_inputs():
    return declared_inputs('i18n')


def telemetry_model_scope_inputs():
    return declared_inputs('telemetry_model_scope')


def commit_audit_error_inputs():
    return declared_inputs('commit_audit_error')


def intent_trace_inputs():
    return declared_inputs('intent_trace')


def telemetry_inputs():
    return declared_inputs('telemetry')


def context_logging_inputs():
    return declared_inputs('context_logging')


def runtime_log_inputs():
    return declared_inputs('runtime_log')


def business_service_inputs():
    return declared_inputs('business_service')


def runtime_business_id_inputs():
    return declared_inputs('runtime_business_id')


def runtime_context_inputs():
    return declared_inputs('runtime_context')


def generated_support_inputs():
    return declared_inputs('generated_support')


def public_facet_logs_inputs():
    return declared_inputs('public_facet_logs')


def purposed_query_debug_inputs():
    return declared_inputs('purposed_query_debug')


def data_store_inputs():
    return declared_inputs('data_store')
