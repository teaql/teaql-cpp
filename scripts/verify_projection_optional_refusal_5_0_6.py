#!/usr/bin/env python3
"""Verify the original generated query rejects unsupported Option operands."""
import os
from pathlib import Path
import subprocess
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions

ROOT=Path(__file__).resolve().parents[1]
project=ROOT/'tests/parity/rust_projection_optional_failure_5_0_6'
reference=ROOT/'.reference/teaql-rs/Cargo.lock'
expected=versions(reference)
for name,resolved in versions(project/'Cargo.lock').items():
    if name not in {'teaql_projection_optional_failure_506','cpp-dynamic-json-core'}:
        assert resolved<=expected.get(name,set()),name
assert registry_identities(project/'Cargo.lock')<=registry_identities(reference)
environment=dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
                 CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
result=subprocess.run(['cargo','check','--offline','--locked','--manifest-path',str(project/'Cargo.toml')],
                      env=environment,text=True,capture_output=True)
assert result.returncode==101,(result.returncode,result.stdout,result.stderr)
assert 'E0277' in result.stderr and 'From<Option<std::string::String>>' in result.stderr
assert 'whose_name_is' in result.stderr
print(result.stderr,end='')
print('Original Option input refusal: expected E0277, reference dependencies verified')
