#!/usr/bin/env python3
"""Verify the complete generic request review and its explicit capability ownership."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from verify_predicate_projection_source_5_0_6 import closing_brace

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'generator/src/main/resources/generator/rust-lib-core/request/index.stg'
RECORD = ROOT / 'docs/parity/general_query_source_review_after_business_id_lookup_5_0_6.json'

GROUPS = {
    'GQ01': 'new default',
    'GQ02': 'return_type',
    'GQ03': 'clone query relation_selections relation_filters child_enhancements query_options into_query from',
    'GQ04': 'search_with_text filter and_filter or_filter append_search_criteria filter_property',
    'GQ05': 'with_deleted_rows deleted_rows_only',
    'GQ06': 'match_types with_type_group',
    'GQ07': 'matching_any_of match_any_of enhance_child enhance_children_if_needed',
    'GQ08': 'raw_sql unsafe_raw_sql raw_sql_filter unsafe_raw_sql_filter create_property_as unsafe_create_property_as',
    'GQ09': 'select_self select_self_fields select_self_without_parent select_all select_children select_any',
    'GQ11': 'purpose comment',
    'G09': '_execute_for_list _execute_for_rows _execute_for_stream _execute_for_first _execute_for_one '
           '_execute_for_page _execute_for_count _execute_for_exists limit stream stream_default skip offset_only '
           'offset page_offset optimize_for_continuous_page_fetch optimize_for_continuous_page_fetch_with '
           'optimize_pagination_with_id_set optimize_pagination_with_id_set_config top_n_probe_parent_threshold '
           'top offset_size unlimited page_number page_number_default page page_default',
    'G10': 'filter_with_json merge_dynamic_json_expr merge_dynamic_json apply_dynamic_json_filter '
           'apply_dynamic_json_order_by apply_dynamic_json_single_order_by dynamic_json_self_field '
           'apply_dynamic_json_chain_filter',
    'G08': 'group_by count count_as aggregate_count aggregate_count_field aggregate_with_function '
           'aggregate_sum aggregate_avg aggregate_min aggregate_max aggregate_stddev aggregate_stddev_pop '
           'aggregate_var_samp aggregate_var_pop aggregate_bit_and aggregate_bit_or aggregate_bit_xor '
           'enable_aggregation_cache enable_aggregation_cache_for propagate_aggregation_cache',
    'G16': 'select_dynamic_fields_with',
    'G13/G17': 'save',
}


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def span(text, start, end):
    body = text[start:end]
    return dict(start_line=text[:start].count('\n') + 1, end_line=text[:end].count('\n') + 1,
                sha256=digest(body), body=body)


def report():
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    source = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql_code_gen'),
                                     'show', ledger['generator_commit'] + ':' + SOURCE]).decode()
    assert digest(source) == ledger['sources'][SOURCE]['sha256']
    start = source.index('request(objectDescriptor) ::= <<')
    end = source.index('impl\\<R: teaql_core::Entity\\> crate::PurposedQuery', start)
    owners = {name: group for group, names in GROUPS.items() for name in names.split()}
    functions = []
    for match in re.finditer(r'\b(?:pub(?:\(crate\))? )?(?:async )?fn ([^\n(]+)\(', source[start:end]):
        name = match[1].split('\\<')[0]
        assert name in owners, 'Unreviewed request responsibility: ' + name
        begin = start + match.start()
        opening = source.index('{', begin)
        functions.append(dict(name=name, responsibility=owners[name],
                              **span(source, begin, closing_brace(source, opening))))
    assert set(owners) == {row['name'] for row in functions}, 'Review omitted a required method'
    templates = []
    for name in ['qMethod', 'rustBusinessIdLookup', 'childTypeFilterMethod', 'tryEnhanceChild',
                 'selectSelfField', 'selectAllObjectRelation', 'selectAllReverseRelation']:
        match = re.search(r'^' + name + r'\([^\n]*\) ::= <<\n', source, re.M)
        assert match, name
        finish = source.index('\n>>', match.end()) + len('\n>>')
        templates.append(dict(name=name, **span(source, match.start(), finish)))
    runtime_source = 'teaql-core/src/request.rs'
    runtime = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql-rs'), 'show',
                                      ledger['runtime_commit'] + ':' + runtime_source]).decode()
    runtime_functions = []
    for name in ['field_operator_column_expr', 'remove_default_live_filter', 'remove_filter_expr']:
        match = re.search(r'pub fn ' + name + r'\(', runtime)
        assert match, name
        opening = runtime.index('{', match.end())
        runtime_functions.append(dict(name=name, **span(runtime, match.start(), closing_brace(runtime, opening))))
    model_helpers = []
    for path, names in [
        ('model/src/main/java/com/skynet/codegenerator/naming/RustObjectDescriptorNaming.java',
         ['rustHasSelectAllRelations', 'rustHasSelectAnyRelations']),
        ('model/src/main/java/com/skynet/codegenerator/meta/ObjectDescriptor.java', ['getBusinessIdFields'])]:
        text = subprocess.check_output(['git', '-C', str(ROOT / '.reference/teaql_code_gen'), 'show',
                                        ledger['generator_commit'] + ':' + path]).decode()
        for name in names:
            match = re.search(r'    public [^\n]+\b' + name + r'\(', text)
            assert match, name
            opening = text.index('{', match.end())
            model_helpers.append(dict(name=name, source=path,
                                      **span(text, match.start(), closing_brace(text, opening))))
    producer = ROOT.parent / 'teaql_code_gen_cpp'
    native_paths = ['teaql_runtime/include/teaql/runtime/entity_query.hpp',
                    'teaql_core/include/teaql/core/request.hpp',
                    'teaql_core/src/request.cpp']
    native = {path: digest((ROOT / path).read_text()) for path in native_paths}
    cpp_source = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppCoreGenerator.java'
    cpp = (producer / cpp_source).read_text()
    cpp_helpers = []
    for name in ['queries', 'fluent', 'adaptQueryMethods', 'descriptorFields', 'fields', 'assist', 'businessIdLookupName', 'ownBusinessIdFields']:
        match = re.search(r'^  private [^\n]+\b' + name + r'\(', cpp, re.M)
        assert match, name
        following = re.search(r'^  private ', cpp[match.end():], re.M)
        finish = match.end() + following.start() if following else len(cpp)
        cpp_helpers.append(dict(name=name, **span(cpp, match.start(), finish)))
    return dict(runtime_commit=ledger['runtime_commit'], generator_commit=ledger['generator_commit'],
                source=SOURCE, source_sha256=digest(source), functions=functions,
                templates=templates, runtime_source=runtime_source, runtime_functions=runtime_functions,
                model_helpers=model_helpers, native_sources=native, cpp_source=cpp_source,
                cpp_helpers=cpp_helpers,
                boundaries='Complete body/guard responsibility index; executable evidence and requirement dispositions are separate. Delegated capability ownership does not waive shared integration or phase exits.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    current = report()
    if args.record:
        assert not RECORD.exists(), 'Preserve previous source reviews'
        RECORD.write_text(json.dumps(current, indent=2) + '\n')
    else:
        assert current == json.loads(RECORD.read_text()), 'Complete general request review changed'
    print(f'Verified {len(current["functions"])} complete generic request functions, seven templates, '
          'three runtime helpers, three model guards and eight native generation helpers; semantics require separate evidence')


if __name__ == '__main__':
    main()
