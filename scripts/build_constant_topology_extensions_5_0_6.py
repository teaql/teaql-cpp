#!/usr/bin/env python3
"""Build original generated topology domains with a test-only extension observer."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from verify_state_baselines_5_0_6 import versions

ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'build_constant_topology_extensions_5_0_6'
CASES=('ordered','reversed','self_reference_required')


def main():
    subprocess.run([sys.executable,str(ROOT/'scripts/verify_generator_parity.py')],check=True)
    env=dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_cpp_inherited_business_binding_bridge_506',
             CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',TEAQL_LOG_ENDPOINT='off')
    records=[]
    for name in CASES:
        case=ROOT/'tests/generator/parity/constant_topology_contracts'/name
        work=WORK/name;work.mkdir(parents=True,exist_ok=True)
        (work/'main.rs').write_bytes((ROOT/'tests/parity/constant_topology_extensions_5_0_6.rs').read_bytes())
        selection='.select_parent()' if name=='self_reference_required' else ''
        read='expression_read(|| domain::E::status(entity).get_category().get_parent().get_code().eval())' if selection else 'let _=entity; (None,"NotApplicable")'
        (work/'topology_calls.rs').write_text('fn category_request()->domain::CategoryRequest {domain::Q::categories().select_self_fields()'+selection+'}\n'+
            "fn parent_read(entity:&domain::Status)->(Option<String>,&'static str) {"+read+'}\n'+
            'fn extra_sql()->&\'static str { "" }\n')
        package=re.search(r'^name = "([^"]+)"',(case/'rust/lib/Cargo.toml').read_text(),re.M)[1]
        manifest=('[package]\nname="constant_topology_extension_observer"\nversion="0.0.0"\nedition="2024"\n'
            '[[bin]]\nname="constant_topology_extension_observer"\npath="main.rs"\n[dependencies]\n'+
            f'domain={{package="{package}",path="{case/"rust/lib"}"}}\n'+
            'serde_json="=1.0.150"\nrusqlite={version="=0.40.1",features=["bundled","column_decltype"]}\n'+
            ''.join(f'{p}={{path="{ROOT/".reference/teaql-rs"/p}"}}\n' for p in ('teaql-core','teaql-runtime','teaql-provider-sqlite'))+
            '[workspace]\n[patch.crates-io]\n'+
            ''.join(f'{p}={{path="{ROOT/".reference/teaql-rs"/p}"}}\n' for p in ('teaql-core','teaql-runtime','teaql-macros','teaql-sql','teaql-data-service','teaql-provider-sqlite')))
        (work/'Cargo.toml').write_text(manifest)
        # Seed each isolated resolution from the exact original dependency lock.
        if not (work/'Cargo.lock').exists():
            (work/'Cargo.lock').write_bytes((ROOT/'.reference/teaql-rs/Cargo.lock').read_bytes())
        with (work/'build.log').open('w') as log:
            subprocess.run(['cargo','build','--offline','--manifest-path',str(work/'Cargo.toml')],
                           env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
        expected=versions(ROOT/'.reference/teaql-rs/Cargo.lock')
        for p,resolved in versions(work/'Cargo.lock').items():
            if p not in (package,'constant_topology_extension_observer') and not resolved<=expected.get(p,set()):
                raise RuntimeError(f'{name}: reference dependency changed: {p}: {resolved}')
        destination=work/'rust_probe'
        shutil.copyfile(Path(env['CARGO_TARGET_DIR'])/'debug/constant_topology_extension_observer',destination)
        destination.chmod(0o755)
        records.append(dict(case=name,build_exit_code=0,source_versions_preserved=True,
            lock_sha256=hashlib.sha256((work/'Cargo.lock').read_bytes()).hexdigest(),
            observer_sha256=hashlib.sha256((work/'main.rs').read_bytes()).hexdigest()))
        print(name+': unchanged generated domain and extension observer built',flush=True)
    (ROOT/'docs/parity/constant_topology_extensions_source_5_0_6.json').write_text(json.dumps(dict(schema=1,cases=records),indent=2)+'\n')


if __name__=='__main__':main()
