#!/usr/bin/env python3
"""Compile unchanged original entities and observe actual descriptor/privacy contracts."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
from xml.sax.saxutils import escape

from verify_generated_privacy_metadata import classpath, LAUNCHER, REFERENCE, CPP
from verify_state_baselines_5_0_6 import versions
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]


def cases():
    values = [('absent', None), ('decimal', '17'), ('leading_zero', '010'),
              ('hex', '0x10'), ('octal', '0o20'), ('binary', '0b10000'),
              ('underscores', '1_000'), ('suffix', '17usize'), ('other_suffix', '17u8'),
              ('quoted', '"17"'), ('quoted_plus', '"+17"'), ('quoted_invalid', '"bad"'),
              ('negative', '-1'), ('boolean', 'true'), ('float', '17.5'),
              ('path', 'notAnInteger'), ('parenthesized', '(17)'),
              ('overflow', '18446744073709551616'), ('max_u64', '18446744073709551615'),
              ('quoted_overflow', '"18446744073709551616"'),
              ('zero', '0'), ('whitespace', ' 17 '), ('empty', ''),
              ('quoted_hex_escape', r'"\x31\x37"'),
              ('quoted_unicode_escape', r'"\u{31}\u{37}"'),
              ('quoted_null', r'"17\0"'), ('quoted_whitespace', '" 17 "'),
              ('raw_string', 'r"17"'), ('raw_hashed_string', 'r###"+17"###'),
              ('raw_invalid_backslash', r'r"\x31\x37"'),
              ('raw_quoted_overflow', 'r#"18446744073709551616"#'),
              ('quoted_suffix', '"17"custom'), ('raw_suffix', 'r#"17"#custom'),
              ('unicode_integer_suffix', '17雪'), ('unicode_string_suffix', '"17"雪'),
              ('unicode_raw_suffix', 'r#"17"#雪'), ('combining_suffix', '17a\u0301'),
              ('float_suffix_32', '17f32'), ('float_suffix_64', '17f64'),
              ('float_suffix_16', '17f16'), ('float_suffix_128', '17f128'),
              ('float_suffix_other_control', '17f64custom'), ('exponent', '1e2'),
              ('exponent_underscore', '1e_2'), ('exponent_plus_control', '1E+2'),
              ('hex_float_suffix_control', '0x17f32'),
              ('comment_before', '/* before */ 17'), ('comment_after', '17 /* after */'),
              ('comment_string', '"17" /* after */'),
              ('nested_comment', '/* outer /* inner */ done */ 17'),
              ('source_whitespace_nel', '\u008517\u0085'),
              ('source_whitespace_direction', '\u200e17\u200f'),
              ('source_whitespace_separator', '\u202817\u2029'),
              ('literal_attribute', '#[doc="test"] 17'),
              ('literal_attribute_string', '#[doc="test"] "17"'),
              ('char_control', "'1'"), ('byte_string_control', 'b"17"'),
              ('c_string_control', 'c"17"'), ('unicode_new_letter_suffix', '17\U00011db0'),
              ('nested_limit_override', '17, audit_value_max_len=19'),
              ('nested_integer_override', '17, audit_value_max_len_int=19'),
              ('nested_overflow_override', '17, audit_value_max_len=18446744073709551616'),
              ('nested_boolean_preserves', '17, audit_value_max_len=false'),
              ('nested_unknown_value_stops', '17, other=0, audit_value_max_len=19'),
              ('nested_unknown_flag_continues', '17, other, audit_value_max_len=19'),
              ('nested_mask_override', '17, audit_mask_fields=""'),
              ('macro_parentheses_override', '17, audit_value_max_len=ignored!(), audit_value_max_len=21'),
              ('macro_braces_override', '17, audit_value_max_len=ignored!{tokens}, audit_value_max_len=21'),
              ('macro_lifetime_override', "17, audit_value_max_len=ignored!{'a}, audit_value_max_len=21"),
              ('max_u32', '4294967295'), ('above_u32', '4294967296'),
              ('expression_if_override', '17, audit_value_max_len=if flag {1} else {2}, audit_value_max_len=21'),
              ('expression_async_override', '17, audit_value_max_len=async {1}, audit_value_max_len=21'),
              ('expression_const_override', '17, audit_value_max_len=const {1}, audit_value_max_len=21'),
              ('expression_closure_override', '17, audit_value_max_len=|| 1, audit_value_max_len=21'),
              ('expression_return_override', '17, audit_value_max_len=return 1, audit_value_max_len=21'),
              ('expression_loop_override', '17, audit_value_max_len=loop {}, audit_value_max_len=21'),
              ('expression_bad_paren_stops', '17, audit_value_max_len=(17 +), audit_value_max_len=21'),
              ('expression_bad_binary_stops', '17, audit_value_max_len=17 +, audit_value_max_len=21'),
              ('expression_mask_after_if', '17, audit_value_max_len=if flag {1}, audit_mask_fields=""'),
              ('expression_bad_closure_comma', '17, audit_value_max_len=|,| 1, audit_value_max_len=21'),
              ('expression_bad_closure_type', '17, audit_value_max_len=|x:| 1, audit_value_max_len=21'),
              ('expression_async_closure', '17, audit_value_max_len=async || 1, audit_value_max_len=21'),
              ('expression_generic_method', '17, audit_value_max_len=value.method::<usize>(), audit_value_max_len=21'),
              ('expression_statement_block', '17, audit_value_max_len={let x = 1; x}, audit_value_max_len=21'),
              ('expression_bad_if_body', '17, audit_value_max_len=if flag {1 +}, audit_value_max_len=21'),
              ('expression_bad_else_body', '17, audit_value_max_len=if flag {1} else {2 +}, audit_value_max_len=21'),
              ('expression_bad_block_body', '17, audit_value_max_len={1 +}, audit_value_max_len=21'),
              ('expression_bad_let', '17, audit_value_max_len={let = 1;}, audit_value_max_len=21'),
              ('expression_prefix_range', '17, audit_value_max_len=..17, audit_value_max_len=21'),
              ('expression_qualified_path', '17, audit_value_max_len=<Thing as Trait>::VALUE, audit_value_max_len=21'),
              ('expression_generic_path', '17, audit_value_max_len=function::<usize, (u8, u8)>(1), audit_value_max_len=21'),
              ('expression_labeled_loop', "17, audit_value_max_len='label: loop {}, audit_value_max_len=21"),
              ('expression_labeled_break', "17, audit_value_max_len=break 'label 1, audit_value_max_len=21"),
              ('expression_raw_reference', '17, audit_value_max_len=&raw const value, audit_value_max_len=21'),
              ('expression_closure_return_type', '17, audit_value_max_len=|| -> usize {1}, audit_value_max_len=21'),
              ('expression_byte_literal', r'17, audit_value_max_len=b"\xff", audit_value_max_len=21'),
              ('expression_byte_macro_mask', r'17, audit_value_max_len=ignored!{b"\xff"}, audit_mask_fields=""'),
              ('expression_if_let', '17, audit_value_max_len=if let Some(x) = value {x}, audit_value_max_len=21'),
              ('expression_while', '17, audit_value_max_len=while flag {}, audit_value_max_len=21'),
              ('expression_while_let', '17, audit_value_max_len=while let Some(x) = value {}, audit_value_max_len=21'),
              ('expression_for_pattern', '17, audit_value_max_len=for (x, y) in pairs {}, audit_value_max_len=21'),
              ('expression_match_guard', '17, audit_value_max_len=match value {Some(x) if x > 0 => x, _ => 0}, audit_value_max_len=21'),
              ('expression_match_block_arms', '17, audit_value_max_len=match value {Some(x) => {x} None => {0}}, audit_value_max_len=21'),
              ('expression_struct_fields', '17, audit_value_max_len=Thing {value: if flag {1} else {2}, ..base}, audit_value_max_len=21'),
              ('expression_struct_rest_tail', '17, audit_value_max_len=Thing {..value, other:}, audit_value_max_len=21'),
              ('expression_let', '17, audit_value_max_len=let Some(x) = value, audit_value_max_len=21'),
              ('expression_nested_struct_index', '17, audit_value_max_len=if values[Thing {value: 1}] {}, audit_value_max_len=21'),
              ('expression_pattern_mask', '17, audit_value_max_len=if let Thing {value, ..} = value {}, audit_mask_fields=""'),
              ('expression_try_block', '17, audit_value_max_len=try {1}, audit_value_max_len=21')]
    result = []
    for name, value in values:
        limit = '' if value is None else ' _audit_value_max_len="' + escape(value, {'"': '&quot;'}) + '"'
        model = ('<root name="cpp_privacy_contracts" data_service="sqlite">'
                 '<platform name="string()"/>'
                 '<item platform="platform(contextRoot=true)" name="string(column=display_name)" '
                 'note="string()?" _table="business_items" _audit_mask_fields="name,platform"'
                 + limit + '/>'
                 '<child _parent="item" secret="string(column=password)" '
                 '_table="business_children" _audit_mask_fields="secret"/></root>')
        result.append((name, model))
    return result


RUST_MAIN = '''
use serde_json::json;
use teaql_core::TeaqlEntity;
use teaql_data_service::SqlIntentRedactions;
fn main() {
    let mut records=Vec::new();
    for d in [Platform::entity_descriptor(),Item::entity_descriptor(),Child::entity_descriptor()] {
        let policies=d.properties.iter().map(|p|
            json!([p.name,p.column_name,format!("{:?}",SqlIntentRedactions::field_policy(&d,&p.name))]))
            .collect::<Vec<_>>();
        records.push(json!({"entity":d.name,"table":d.table_name,"service":d.data_service,
            "masks":d.audit_mask_fields,"declared":d.audit_mask_fields_declared,
            "limit":d.audit_value_max_len.map(|n|n.to_string()),"policies":policies}));
    }
    println!("{}",json!(records));
}
'''

CPP_MAIN = '''
#include "cpp_privacy_contracts.hpp"
#include <teaql/data_service/sql_log.hpp>
#include <iostream>
using namespace teaql;
static Json observe(Memory r, EntityDescriptor d) {
    Json::Object item(r);
    item.emplace("entity", Json(r,d.name));
    item.emplace("table", Json(r,d.table_name));
    item.emplace("service", d.data_service ? Json(r,*d.data_service) : Json(r));
    item.emplace("declared", Json(r,d.audit_mask_fields_declared));
    item.emplace("limit", d.audit_value_max_len ? Json(r,std::to_string(*d.audit_value_max_len)) : Json(r));
    Json::Array masks(r), policies(r);
    for (const auto& mask : d.audit_mask_fields) { masks.emplace_back(mask); }
    for (const auto& p : d.properties) {
        auto policy=SqlIntentRedactions::field_policy(d,p.name);
        const char* name=policy==SqlParameterLogPolicy::Plain ? "Plain" :
            policy==SqlParameterLogPolicy::Masked ? "Masked" :
            policy==SqlParameterLogPolicy::Credential ? "Credential" : "Unknown";
        Json::Array row(r);
        row.emplace_back(p.name); row.emplace_back(p.column_name); row.emplace_back(name);
        policies.emplace_back(std::move(row));
    }
    item.emplace("masks",Json(r,std::move(masks)));
    item.emplace("policies",Json(r,std::move(policies)));
    return Json(r,std::move(item));
}
int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context=global.make_user_context();
    auto r=user_context->memory_resource();
    Json::Array output(r);
    output.emplace_back(observe(r,cpp_privacy_contracts::Platform::entity_descriptor(r)));
    output.emplace_back(observe(r,cpp_privacy_contracts::Item::entity_descriptor(r)));
    output.emplace_back(observe(r,cpp_privacy_contracts::Child::entity_descriptor(r)));
    std::cout << Json(r,std::move(output)).dump(r) << '\\n';
}
'''


def main(argv=None, *, scenarios=None, rust_main=RUST_MAIN, cpp_main=CPP_MAIN,
         scope=None, extra_provenance=None, expected_rejections=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cases', nargs='+')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--require-match', action='store_true',
                        help='Fail unless every selected original/native compiled observation matches')
    args = parser.parse_args(argv)
    available = cases() if scenarios is None else scenarios
    selected = [c for c in available if not args.cases or c[0] in args.cases]
    if args.cases and set(args.cases) - {c[0] for c in available}:
        parser.error('Unknown privacy contract case')
    if args.output.exists():
        parser.error('Use a new output path; existing evidence is immutable')
    commands = json.loads((ROOT / 'build_native_current/compile_commands.json').read_text())
    entry = next(c for c in commands if c['file'].endswith('/tests/generator/generated_api_tests.cpp'))
    native = list(entry.get('arguments') or shlex.split(entry['command']))
    for flag in ('-MD', '-MT', '-MF', '-o'):
        while flag in native:
            i = native.index(flag)
            del native[i:i+(1 if flag == '-MD' else 2)]
    native.remove('-c')
    native.remove(entry['file'])
    target = '/tmp/teaql_cpp_inherited_business_binding_bridge_506'
    environment = dict(os.environ, CARGO_TARGET_DIR=target,
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')
    records = []
    with tempfile.TemporaryDirectory(prefix='teaql_privacy_contracts_') as temporary:
        work = Path(temporary)
        original_cp = classpath(REFERENCE, work / 'original_cp.txt')
        cpp_cp = classpath(CPP, work / 'cpp_cp.txt')
        launcher = work / 'FrozenPrivacy.java'
        launcher.write_text(LAUNCHER)
        subprocess.run(['javac', '-cp', original_cp, str(launcher)], check=True)
        for name, model_text in selected:
            print('Auditing '+name, flush=True)
            case = work / name
            case.mkdir()
            model = case / 'model.xml'
            model.write_text(model_text)
            rust = case / 'original'
            cpp = case / 'cpp'
            subprocess.run(['java', '-cp', str(work)+':'+original_cp, 'FrozenPrivacy', str(model), str(rust)],
                           check=True, stdout=subprocess.DEVNULL)
            subprocess.run(['java', '-cp', cpp_cp, 'com.skynet.codegenerator.cpp.CppCoreGenerator',
                            str(model), str(cpp)], check=True, stdout=subprocess.DEVNULL)
            harness = case / 'harness'
            harness.mkdir()
            imports = ''
            for entity in ['platform','item','child']:
                path = rust / 'lib/src' / entity / 'entity.rs'
                imports += f'#[path="{path}"] mod {entity}_entity; pub use {entity}_entity::*;\n'
            (harness / 'main.rs').write_text(imports + rust_main)
            (harness / 'Cargo.toml').write_text(
                '[package]\nname="teaql_privacy_contract_observer"\nversion="0.1.0"\nedition="2024"\n'
                '[workspace]\n[[bin]]\nname="teaql_privacy_contract_observer"\npath="main.rs"\n'
                '[dependencies]\nserde_json="=1.0.150"\nserde="=1.0.228"\nchrono="=0.4.45"\nrust_decimal="=1.42.1"\n'
                + ''.join(f'{package}={{path="{ROOT / ".reference/teaql-rs" / package}"}}\n'
                          for package in ['teaql-core','teaql-runtime','teaql-data-service','teaql-macros']))
            (harness / 'Cargo.lock').write_bytes((ROOT / 'tests/parity/rust_checker_clock_composition_5_0_6/Cargo.lock').read_bytes())
            built = subprocess.run(['cargo','build','--offline','--manifest-path',str(harness/'Cargo.toml')],
                                   env=environment,text=True,capture_output=True)
            source = None
            if built.returncode == 0:
                source = json.loads(subprocess.check_output([target+'/debug/teaql_privacy_contract_observer'],env=environment,text=True))
                expected = versions(ROOT / '.reference/teaql-rs/Cargo.lock')
                for package,resolved in versions(harness/'Cargo.lock').items():
                    if package!='teaql_privacy_contract_observer' and not resolved<=expected.get(package,set()):
                        raise RuntimeError('Reference dependency differs: '+package)
                if not registry_identities(harness/'Cargo.lock')<=registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock'):
                    raise RuntimeError('Registry checksum differs')
            source_file = case / 'observer.cpp'
            source_file.write_text(cpp_main)
            subprocess.run([str(ROOT/'.tools/clang-format/bin/clang-format'),'-i',
                            f'--style=file:{ROOT / ".clang-format"}',str(source_file)],check=True)
            binary = case / 'observer'
            # teaql_async is a header-only target, not a missing archive.
            libraries = ['teaql_runtime','teaql_sql','teaql_data_service','teaql_core','teaql_memory','teaql_provider_sqlite']
            compiled = subprocess.run([*native,'-I'+str(cpp/'include'),str(source_file),'-o',str(binary),
                '-Wl,--start-group',*(str(ROOT/'build_native_current'/p/('lib'+p+'.a')) for p in libraries),
                '-Wl,--end-group',str(ROOT/'.tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a'),'-lpthread','-ldl'],
                text=True,capture_output=True)
            actual = None
            if compiled.returncode == 0:
                actual = json.loads(subprocess.check_output([str(binary)],env=environment,text=True))
            files = {}
            for directory in [rust,cpp,harness]:
                for p in directory.rglob('*'):
                    if p.is_file():
                        data=p.read_bytes()
                        files[str(p.relative_to(case))]={'sha256':hashlib.sha256(data).hexdigest(),'text':data.decode()}
            matching = source is not None and source == actual
            rejection = (expected_rejections or {}).get(name)
            if rejection is not None:
                matching = (built.returncode != 0 and compiled.returncode != 0
                            and re.search(rejection[0], built.stderr) is not None
                            and re.search(rejection[1], compiled.stderr) is not None)
            records.append(dict(case=name,model=model_text,source_compile=built.returncode,
                                source_errors=built.stderr,source=source,cpp_compile=compiled.returncode,
                                cpp_errors=compiled.stderr,cpp=actual,matching=matching,
                                expected_compile_rejection=rejection,
                                files=files))
    data=dict(schema=1,runtime_commit=json.loads((ROOT/'upstream.lock.json').read_text())['commit'],
              observer_script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              scope=scope or 'Complete original entity files and actual derive descriptors/field_policy; not runtime assembly or governed SQL.',
              cases=records)
    if extra_provenance is not None:
        data['extra_provenance'] = extra_provenance
    args.output.write_bytes(gzip.compress(json.dumps(data,indent=2).encode(),mtime=0))
    print(f'{sum(c["matching"] for c in records)}/{len(records)} compiled descriptor/privacy cases match')
    if args.require_match and not all(c['matching'] for c in records):
        raise SystemExit('Compiled privacy contract mismatch; full evidence retained')


if __name__ == '__main__':
    main()
