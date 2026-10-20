#!/usr/bin/env python3
"""Verify actual original query field-guard refusals without modifying the library."""
import gzip
import json
import os
from pathlib import Path
import subprocess
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
ROOT=Path(__file__).resolve().parents[1]
project=ROOT/'tests/parity/rust_query_field_guard_failures_5_0_6'
reference=ROOT/'.reference/teaql-rs/Cargo.lock'
for name,resolved in versions(project/'Cargo.lock').items():
    if name not in {'teaql_query_field_guard_failures_506','cpp-query-branches-core'}:
        assert resolved<=versions(reference).get(name,set()),name
assert registry_identities(project/'Cargo.lock')<=registry_identities(reference)
env=dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
records=[]
methods = dict(bool_generic='with_active', id_before='with_id_before',
               id_function='select_id_with_function', optional_bool_in='with_optional_active_in',
               password_sound='with_secret_sounding_like', string_function='select_label_with_function',
               text_contains='with_notes_containing', version_equal='with_version_is')
assert set(methods) == {case.stem for case in (project/'src/bin').glob('*.rs')}
for case in sorted((project/'src/bin').glob('*.rs')):
    result=subprocess.run(['cargo','check','--offline','--locked','--manifest-path',str(project/'Cargo.toml'),'--bin',case.stem],env=env,text=True,capture_output=True)
    assert result.returncode==101 and 'E0599' in result.stderr,(case,result.returncode,result.stderr)
    assert f'no method named `{methods[case.stem]}`' in result.stderr, result.stderr
    records.append(dict(case=case.stem,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
    print('Verified original field guard:',case.stem)
path=ROOT/'docs/parity/query_field_guard_original_failures_5_0_6.json.gz'
if not path.exists():path.write_bytes(gzip.compress(json.dumps(records).encode(),mtime=0))
else:
    retained=json.loads(gzip.decompress(path.read_bytes()))
    assert [r['case'] for r in retained]==[r['case'] for r in records]
    assert all(r['exit_code']==101 and 'E0599' in r['stderr'] for r in retained)
print(f'Original field guard refusals: {len(records)}/{len(records)}')
