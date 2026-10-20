#!/usr/bin/env python3
"""Compile complete original wire mapping functions and matching generated C++ APIs."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from verify_generated_privacy_metadata import classpath, LAUNCHER, REFERENCE, CPP
from verify_state_baselines_5_0_6 import versions
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]


def cases():
    for profile in ('camelCase', 'snake_case', 'PascalCase'):
        for kind in ('ordinary', 'inherited', 'constant', 'wide'):
            fields = 'account_id="string(column=ledger_account)" postal_code="string()"'
            if kind == 'wide':
                fields += ' ' + ' '.join(f'field_{i}="string()"' for i in range(140))
            extra = ('<child _parent="record" child_name="string()"/>' if kind == 'inherited' else
                     '<status _constant="true" code="string(A,B)"/>' if kind == 'constant' else '')
            yield (profile+'_'+kind,
                   '<root name="cpp_wire_contracts" data_service="sqlite" json_field_naming="'
                   +profile+'"><platform owner_name="string()"/><record platform="platform(contextRoot=true)" '
                   +fields+'/>'+extra+'</root>')


def function(text, name):
    begin = text.index('pub fn '+name+'(')
    opening = text.index('{', begin)
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == '{':
            depth += 1
        elif text[index] == '}':
            depth -= 1
            if depth == 0:
                return text[begin:index+1]
    raise RuntimeError('Incomplete original wire mapping function')


CPP_MAIN = '''
#include "cpp_wire_contracts.hpp"
#include <iostream>
using namespace teaql;
static Json encode(Memory r, const GeneratedWireFieldMappings& mappings) {
    Json::Object result(r);
    for (const auto& [entity, fields] : mappings) {
        Json::Object values(r);
        for (const auto& [canonical, wire] : fields) {
            values.emplace(String(canonical,r),Json(r,wire));
        }
        result.emplace(String(entity,r),Json(r,std::move(values)));
    }
    return Json(r,std::move(result));
}
int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto owner=global.make_user_context();
    auto& user_context=*owner;
    Json retained(global.memory_resource());
    {
        auto r=user_context.memory_resource();
        Json::Object output(r);
        output.emplace("mappings",encode(r,cpp_wire_contracts::generatedWireFieldMappings(user_context)));
        output.emplace("aliases",encode(r,cpp_wire_contracts::generatedWireFieldAliases(user_context)));
        retained=Json(global.memory_resource(),output);
    }
    owner.reset();
    if (global.live_user_bytes()!=0) { throw std::runtime_error("Wire maps retained User memory"); }
    std::cout << retained.dump(global.memory_resource()) << '\\n';
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('Use a fresh evidence output path')
    commands = json.loads((ROOT/'build_native_current/compile_commands.json').read_text())
    entry = next(c for c in commands if c['file'].endswith('/tests/generator/generated_api_tests.cpp'))
    compiler = list(entry.get('arguments') or shlex.split(entry['command']))
    for flag in ('-MD','-MT','-MF','-o'):
        while flag in compiler:
            index=compiler.index(flag)
            del compiler[index:index+(1 if flag=='-MD' else 2)]
    compiler.remove('-c'); compiler.remove(entry['file'])
    environment = dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_wire_mapping_source_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',TEAQL_LOG_ENDPOINT='off')
    records=[]
    with tempfile.TemporaryDirectory(prefix='teaql_wire_mappings_') as directory:
        work=Path(directory)
        original_cp=classpath(REFERENCE,work/'original_cp.txt')
        cpp_cp=classpath(CPP,work/'cpp_cp.txt')
        launcher=work/'FrozenPrivacy.java';launcher.write_text(LAUNCHER)
        subprocess.run(['javac','-cp',original_cp,str(launcher)],check=True)
        for name,xml in cases():
            print('Comparing '+name,flush=True)
            case=work/name;case.mkdir()
            model=case/'model.xml';model.write_text(xml)
            original=case/'original';generated=case/'cpp'
            subprocess.run(['java','-cp',str(work)+':'+original_cp,'FrozenPrivacy',str(model),str(original)],
                           check=True,stdout=subprocess.DEVNULL)
            subprocess.run(['java','-cp',cpp_cp,'com.skynet.codegenerator.cpp.CppCoreGenerator',str(model),str(generated)],
                           check=True,stdout=subprocess.DEVNULL)
            runtime=(original/'lib/src/runtime.rs').read_text()
            functions='\n\n'.join(function(runtime,n) for n in ['generated_wire_field_mappings','generated_wire_field_aliases'])
            harness=case/'harness';harness.mkdir()
            (harness/'main.rs').write_text(functions+'\nfn main() { println!("{}",serde_json::json!({"mappings":generated_wire_field_mappings(),"aliases":generated_wire_field_aliases()})); }\n')
            (harness/'Cargo.toml').write_text('[package]\nname="teaql_wire_mapping_observer"\nversion="0.1.0"\nedition="2024"\n[workspace]\n[[bin]]\nname="teaql_wire_mapping_observer"\npath="main.rs"\n[dependencies]\nserde_json="=1.0.150"\n')
            (harness/'Cargo.lock').write_bytes((ROOT/'tests/parity/rust_initialized_snapshot_cache_5_0_6/Cargo.lock').read_bytes())
            subprocess.run(['cargo','build','--offline','--manifest-path',str(harness/'Cargo.toml')],env=environment,
                           check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            expected=versions(ROOT/'.reference/teaql-rs/Cargo.lock')
            for package,resolved in versions(harness/'Cargo.lock').items():
                if package!='teaql_wire_mapping_observer' and not resolved<=expected.get(package,set()):
                    raise RuntimeError('Wire observer dependency differs: '+package)
            if not registry_identities(harness/'Cargo.lock')<=registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock'):
                raise RuntimeError('Wire observer registry checksums differ')
            source=json.loads(subprocess.check_output([environment['CARGO_TARGET_DIR']+'/debug/teaql_wire_mapping_observer'],text=True))
            cpp=case/'observer.cpp';cpp.write_text(CPP_MAIN)
            subprocess.run([str(ROOT/'.tools/clang-format/bin/clang-format'),'-i',f'--style=file:{ROOT/".clang-format"}',str(cpp)],check=True)
            binary=case/'observer'
            libraries=['teaql_runtime','teaql_sql','teaql_data_service','teaql_core','teaql_memory','teaql_provider_sqlite']
            subprocess.run([*compiler,'-I'+str(generated/'include'),str(cpp),'-o',str(binary),'-Wl,--start-group',
                            *(str(ROOT/'build_native_current'/p/('lib'+p+'.a')) for p in libraries),'-Wl,--end-group',
                            str(ROOT/'.tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a'),'-lpthread','-ldl'],check=True)
            native=json.loads(subprocess.check_output([str(binary)],env=environment,text=True))
            files={}
            for folder in [original,generated,harness]:
                for path in folder.rglob('*'):
                    if path.is_file():
                        data=path.read_bytes()
                        files[str(path.relative_to(case))]={'sha256':hashlib.sha256(data).hexdigest(),'text':data.decode()}
            records.append(dict(case=name,model=xml,source=source,cpp=native,matching=source==native,
                                original_functions=functions,native_observer=cpp.read_text(),files=files))
        invalid='<root name="cpp_wire_contracts" data_service="sqlite" json_field_naming="invalid"><platform name="string()"/></root>'
        model=work/'invalid.xml';model.write_text(invalid)
        errors=[]
        for language,cp,main in [('rust',str(work)+':'+original_cp,'FrozenPrivacy'),
                                 ('cpp',cpp_cp,'com.skynet.codegenerator.cpp.CppCoreGenerator')]:
            result=subprocess.run(['java','-cp',cp,main,str(model),str(work/('invalid_'+language))],text=True,capture_output=True)
            diagnostics=result.stdout+result.stderr
            if language=='cpp':
                assert result.returncode!=0 and 'Unsupported json_field_naming' in diagnostics
            else:
                # Frozen StringTemplate suppresses the getter failure: generation
                # succeeds, with empty selected wire strings and no diagnostic.
                assert result.returncode==0
                rendered=(work/'invalid_rust/lib/src/runtime.rs').read_text()
                assert '("id".to_owned(), "".to_owned())' in rendered
                assert '("name".to_owned(), "".to_owned())' in rendered
            artifacts={}
            output=work/('invalid_'+language)
            if output.exists():
                artifacts={str(p.relative_to(output)):p.read_text() for p in output.rglob('*') if p.is_file()}
            errors.append(dict(language=language,exit_code=result.returncode,diagnostics=diagnostics,
                               generated_artifacts=artifacts))
    result=dict(schema=1,runtime_commit=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
                scope='Complete original mapping/alias function bodies, three naming profiles and selected own-field configurations; no transport or runtime assembly claim.',
                cases=records,invalid_profile=errors)
    args.output.write_bytes(gzip.compress(json.dumps(result,indent=2).encode(),mtime=0))
    if not all(c['matching'] for c in records):
        raise SystemExit('Wire mapping differences; full evidence retained')
    print(f'{len(records)}/{len(records)} original/native wire maps match; invalid-profile empty source wires/C++ rejection retained')


if __name__=='__main__':
    main()
