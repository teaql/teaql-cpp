"""Complete declared numeric/time stimuli; no expected-output selection."""
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DECLARED = {'unified_buffer_clone': ('tests/parity/unified_buffer_clone_baseline_5_0_6_cases.json.gz', '803a631889e7e7c170396ddddea921eb104c1864163ec4126bdd40e44c47907b'), 'system_time_arithmetic_scope': ('tests/parity/system_time_arithmetic_scope_baseline_5_0_6_cases.json.gz', '997d431f446fffca9fd2cfe62d9192df4efa749578109452b571dd6e12299fc3'), 'system_time_error_scope': ('tests/parity/system_time_error_scope_5_0_6_cases.json.gz', '718939229fa89c5f12cac582d38d732a409402bc6c0554dd4011b327a0ea4f61'), 'chrono_value': ('tests/parity/chrono_value_baseline_5_0_6_cases.json.gz', '2e4df03a6d60add3504fe273a7271c7bd8d4bedc9b9cb700dc0329f8d1f1230e'), 'system_time': ('tests/parity/system_time_baseline_5_0_6_cases.json.gz', '82c4bec26cdb1da4451b19f0c2683bb000cb9c607cec25162a3589433b7231b8'), 'decimal': ('tests/parity/decimal_5_0_6_cases.json.gz', 'e6f0a0d35d728cff05d446600fdd4787d66106c040f9a3929a59b01b88bbe15a'), 'decimal_extended': ('tests/parity/decimal_extended_5_0_6_cases.json.gz', '365bb76297d3399fbd6dd1fd08ed93edfe21ff88ee9d6dea75896e5eb3f40c3c'), 'decimal_inline': ('tests/parity/decimal_inline_5_0_6_cases.json.gz', '7dfc1852881dbb052dd3b350045d726eb2a2a98dcad938966a3695dc643bf08e'), 'core_aux_debug': ('tests/parity/core_aux_debug_5_0_6_cases.json.gz', '25654dd2cf61715dffa9c0a4283dbf9029b8098154d96903aaf526e266e1140f'), 'decimal_scale': ('tests/parity/decimal_scale_5_0_6_cases.json.gz', '6b9052e7c91ce30e7d8e07d0c7e479cad651b7e655a00a4dfb54e6afd668daf5'), 'decimal_parsing': ('tests/parity/decimal_parsing_5_0_6_cases.json.gz', '944db79fec0533af4b5bc108a9535409d39af77c72c4045d81cd57ab2bfb82be'), 'decimal_numeric': ('tests/parity/decimal_numeric_5_0_6_cases.json.gz', '4964863d1dfc2e84160b6f89be8e2798459cfb6737a9ca5478e171a8b4a4e347'), 'decimal_foundation': ('tests/parity/decimal_foundation_5_0_6_cases.json.gz', '92a8b0fb1a9ef964b1a01772bc7e4e1e37512cebd308731a7d3342a498bdd487'), 'decimal_wire_format': ('tests/parity/decimal_wire_format_5_0_6_cases.json.gz', '05f2041d9b377a4d5b4c30fb84bf1313747a8f7a71322271f83fe5cd33bedabf'), 'timestamp_api': ('tests/parity/timestamp_api_baseline_5_0_6_cases.json.gz', '0c1b9c35931c6382b39697601d5859d0826287f64d369050fd2bbe63e8dbf051')}


def declared_inputs(name):
    filename, expected = DECLARED[name]
    path = ROOT / filename
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected
    data = json.loads(gzip.decompress(raw) if path.suffix == ".gz" else raw)
    return [row["input"] for row in data["cases"]]


def unified_buffer_clone_inputs():
    return declared_inputs('unified_buffer_clone')


def system_time_arithmetic_scope_inputs():
    return declared_inputs('system_time_arithmetic_scope')


def system_time_error_scope_inputs():
    return declared_inputs('system_time_error_scope')


def chrono_value_inputs():
    return declared_inputs('chrono_value')


def system_time_inputs():
    return declared_inputs('system_time')


def decimal_inputs():
    return declared_inputs('decimal')


def decimal_extended_inputs():
    return declared_inputs('decimal_extended')


def decimal_inline_inputs():
    return declared_inputs('decimal_inline')


def core_aux_debug_inputs():
    return declared_inputs('core_aux_debug')


def decimal_scale_inputs():
    return declared_inputs('decimal_scale')


def decimal_parsing_inputs():
    return declared_inputs('decimal_parsing')


def decimal_numeric_inputs():
    return declared_inputs('decimal_numeric')


def decimal_foundation_inputs():
    return declared_inputs('decimal_foundation')


def decimal_wire_format_inputs():
    return declared_inputs('decimal_wire_format')


def timestamp_api_inputs():
    return declared_inputs('timestamp_api')
