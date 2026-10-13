#!/usr/bin/env python3
"""Require real compiler rejection of invalid generated C++ application calls."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--cases', nargs='+', help='Run named compiler contracts; default is all')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    commands = json.loads((build / 'compile_commands.json').read_text())
    record = next(row for row in commands
                  if row['file'].endswith('/tests/generator/generated_api_tests.cpp'))
    original = record.get('arguments') or shlex.split(record['command'])
    command = []
    skip = False
    for index, part in enumerate(original):
        if skip:
            skip = False
            continue
        if part == '-o':
            skip = True
            continue
        if part == '-c' or part == record['file']:
            continue
        command.append(part)
    command.extend(['-fsyntax-only', '-ferror-limit=5'])
    header = ROOT / 'examples/generated/include/cpp_examples.hpp'
    prelude = 'using namespace teaql; using namespace cpp_examples;'
    cases = {
        'positive': ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.orders().comment("what").purpose("why"); request.executeForList(user_context); request.executeForOne(user_context); Q.orderLines().comment("what").purpose("why").executeForStream(user_context); cpp_examples::E::order(user_context.make<Order>()).getName();', True, ''),
        'missing_purpose': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().comment("what").executeForList(user_context);', False, 'executeForList'),
        'unknown_field': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().withNmaeIs("x");', False, 'withNmaeIs'),
        'wrong_scalar': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().withNameIs(42);', False, 'withNameIs'),
        'wrong_relation': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().selectLinesWith(Q.products());', False, 'selectLinesWith'),
        'extra_runtime_argument': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().purpose("why").executeForOne(user_context, 1);', False, 'executeForOne'),
        'global_factory': ('GlobalContext global; global.queries<cpp_examples::query_factory>();', False, 'queries'),
        'context_on_entry': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines(user_context);', False, 'orderLines'),
        'temporary_entity': ('cpp_examples::E::order(Order(std::allocator_arg, Allocator(user_context.memory_resource())));', False, 'deleted'),
    }
    cases['missing_audit'] = ('user_context.make<Order>().save(user_context);', False, 'save')
    cases['sample_positive'] = ('cpp_examples::generateSampleData(user_context, SampleDataPlan::small());', True, '')
    cases['sample_wrong_plan'] = ('cpp_examples::generateSampleData(user_context, 42);', False, 'generateSampleData')
    cases['sample_extra_runtime'] = ('cpp_examples::generateSampleData(user_context, SampleDataPlan::small(), 1);', False, 'generateSampleData')
    cases['dynamic_json_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.orders().purpose("why").filterWithJson("{}").comment("what"); request.executeForList(user_context);', True, '')
    cases['general_query_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().withDeletedRows().deletedRowsOnly().pageNumber(2, 3).offset(1, 2).offsetOnly(3).top(2).withNameNotContaining("x").selectNameRaw("name").unselectName();', True, '')
    cases['aggregation_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto query = Q.productsMinimal().groupByNameAs("group").countName().sumId().sumVersion().aggregateWithFunction("id", "total", teaql::AggregateFunction::Sum).enableAggregationCacheFor(250).purpose("inspect statistics"); query.comment("aggregate fields").countAs("rows");', True, '')
    cases['facets_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().facetByOrderAsWithOptions("_orders", Q.orders().comment("facet counts").countLinesAs("_lines"), false).purpose("inspect facets").comment("outer list").executeForList(user_context);', True, '')
    cases['facets_wrong_child'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().facetByOrderAs("_orders", Q.products());', False, 'facetByOrderAs')
    cases['relationship_analysis_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto lines = Q.orderLines().withDeletedRows(); Q.orders().countLinesWith("_count", lines).statsFromLinesAs("_stats", lines).purpose("inspect members").comment("aggregate relation").executeForList(user_context); Q.orderLines().rollUpToOrderWith(Q.orders());', True, '')
    cases['relationship_analysis_wrong_child'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().countLinesWith("_count", Q.products());', False, 'countLinesWith')
    cases['aggregation_wrong_function'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().aggregateWithFunction("id", "total", "SUM");', False, 'aggregateWithFunction')
    cases['raw_segment_wrong_type'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().unsafeRawSql("SELECT 1");', False, 'unsafeRawSql')
    cases['dynamic_json_wrong_input'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().filterWithJson(42);', False, 'filterWithJson')
    cases['execution_extensions_positive'] = ('static_assert(!std::same_as<cpp_examples::DataServiceExecutor, cpp_business_ids::DataServiceExecutor>); auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.products().comment("what").purpose("why"); request.executeForCount(user_context); request.executeForExists(user_context); request.executeForFirst(user_context); request.executeForPage(user_context, 0, 10);', True, '')
    cases['count_missing_purpose'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().comment("what").executeForCount(user_context);', False, 'executeForCount')
    cases['page_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().purpose("why").executeForPage(user_context, user_context, 0, 10);', False, 'executeForPage')
    cases['candidate_positive'] = ('auto Q = user_context.queries<cpp_candidates::query_factory>(); Q.records().withStatusIsDraft().withPhaseIsNotInProgress().withTagIsNull().withTagIsUnknown(); Q.people().whoseStatusIsDone().whosePhaseIsDraft(); auto& entity = user_context.make<cpp_candidates::Record>(); entity.updateStatusToInProgress(); cpp_candidates::E::record(entity).statusIsInProgress();', True, '')
    cases['candidate_raw_id_setter'] = ('user_context.make<cpp_candidates::Record>().updateStatusId(999);', False, 'private')
    cases['candidate_raw_object_setter'] = ('auto& status = user_context.make<cpp_candidates::OrderStatus>(); user_context.make<cpp_candidates::Record>().updateStatus(status);', False, 'private')
    cases['candidate_unknown_update'] = ('user_context.make<cpp_candidates::Record>().updateStatusToUnknown();', False, 'updateStatusToUnknown')
    cases['candidate_unknown_predicate'] = ('auto Q = user_context.queries<cpp_candidates::query_factory>(); Q.records().withStatusIsUnknownCandidate();', False, 'withStatusIsUnknownCandidate')
    cases['candidate_unknown_expression'] = ('cpp_candidates::E::record(user_context.make<cpp_candidates::Record>()).statusIsUnknownCandidate();', False, 'statusIsUnknownCandidate')
    cases['candidate_human_vocabulary'] = ('auto Q = user_context.queries<cpp_candidates::query_factory>(); Q.people().withStatusIsDraft();', False, 'withStatusIsDraft')
    cases['candidate_missing_purpose'] = ('auto Q = user_context.queries<cpp_candidates::query_factory>(); Q.records().withStatusIsDraft().comment("what").executeForOne(user_context);', False, 'executeForOne')
    cases['business_id_positive'] = ('auto id = cpp_business_ids::ReceiptNumber::create(user_context.memory_resource(), "RCP-value"); auto Q = user_context.queries<cpp_business_ids::query_factory>(); Q.receipts().withReceiptNumberIs(id.value()).comment("what").purpose("why").executeForOne(user_context);', True, '')
    cases['business_id_text_setter'] = ('user_context.make<cpp_business_ids::Receipt>().updateReceiptNumber("raw text");', False, 'updateReceiptNumber')
    cases['business_id_wrong_type'] = ('auto id = cpp_business_ids::TicketNumber::create(user_context.memory_resource(), "TKT-value"); user_context.make<cpp_business_ids::Receipt>().updateReceiptNumber(id.value());', False, 'updateReceiptNumber')
    cases['business_id_text_predicate'] = ('auto Q = user_context.queries<cpp_business_ids::query_factory>(); Q.receipts().withReceiptNumberIs("raw text");', False, 'withReceiptNumberIs')
    cases['optional_scalar_positive'] = ('auto& reading = user_context.make<cpp_optional_types::Reading>(); reading.updateRecordedAt(std::nullopt); reading.updateActive(false); reading.updateText(std::optional<String>(std::in_place, "text", user_context.memory_resource())); auto Q = user_context.queries<cpp_optional_types::query_factory>(); Q.readings().withRecordedAtIs(Timestamp(0)).comment("what").purpose("why").executeForOne(user_context);', True, '')
    cases['optional_timestamp_text'] = ('user_context.make<cpp_optional_types::Reading>().updateRecordedAt("raw timestamp");', False, 'updateRecordedAt')
    cases['optional_timestamp_predicate'] = ('auto Q = user_context.queries<cpp_optional_types::query_factory>(); Q.readings().withRecordedAtIs("raw timestamp");', False, 'withRecordedAtIs')
    cases['context_root_getters'] = ('auto& note = user_context.make<cpp_checker_fixture::Note>(); note.tenantId(); note.changedTenantId(); note.evalTenantId(); note.tenant(); note.evalTenant(); auto Q = user_context.queries<cpp_checker_fixture::query_factory>(); Q.notes().withTenantIs(42).selectTenant();', True, '')
    cases['context_root_id_setter'] = ('user_context.make<cpp_checker_fixture::Note>().updateTenantId(42);', False, 'private')
    cases['context_root_object_setter'] = ('auto& tenant = user_context.make<cpp_checker_fixture::Tenant>(); user_context.make<cpp_checker_fixture::Note>().updateTenant(tenant);', False, 'private')
    cases['business_context_root_setter'] = ('user_context.make<cpp_business_ids::Receipt>().updateTenantId(42);', False, 'private')
    cases['inheritance_positive'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schoolsWithChildren().isAcademy().enhanceChild(Q.academies()).comment("what").purpose("why").executeForOne(user_context); Q.academies().isResearchAcademy();', True, '')
    cases['inheritance_unknown_subtype'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().isResearchAcademy();', False, 'isResearchAcademy')
    cases['inheritance_missing_purpose'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schoolsWithChildren().comment("what").executeForList(user_context);', False, 'executeForList')
    cases['inheritance_untyped_child'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().enhanceChild(42);', False, 'query_')
    cases['return_type_positive'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); auto adapted = Q.schoolsWithChildren().returnType<cpp_inheritance::ResearchAcademy>().purpose("why").comment("what"); adapted.executeForList(user_context); adapted.executeForOne(user_context); adapted.executeForStream(user_context); static_assert(std::same_as<decltype(adapted.newEntity(user_context)), cpp_inheritance::School&>); Q.schools().enhanceChild(Q.academies().returnType<cpp_inheritance::ResearchAcademy>());', True, '')
    cases['return_type_not_entity'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().returnType<int>();', False, 'returnType')
    cases['return_type_missing_purpose'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().returnType<cpp_inheritance::ResearchAcademy>().executeForList(user_context);', False, 'executeForList')
    cases['return_type_extra_context'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().returnType<cpp_inheritance::ResearchAcademy>().purpose("why").executeForOne(user_context, user_context);', False, 'executeForOne')
    cases['candidate_diagnostic_repairs'] = ('auto Q = user_context.queries<cpp_candidates::query_factory>(); Q.recordsMinimal().selectStatusId().selectPhase().selectStatus();', True, '')
    cases['inherited_diagnostic_repairs'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.researchAcademiesMinimal().selectName().selectCampusCode().selectBudget();', True, '')
    cases['expression_diagnostic_dynamic_repairs'] = ('auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.peopleMinimal().selectPlatformId().selectPlatformWith(Q.platformsMinimal().selectName()).selectProfileWith(Q.profilesMinimal().selectNote()).selectOrdersWith(Q.ordersMinimal().selectCode());', True, '')
    cases['expression_diagnostic_wrong_child'] = ('auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.peopleMinimal().selectPlatformWith(Q.ordersMinimal().selectCode());', False, 'selectPlatformWith')
    if args.cases:
        unknown = set(args.cases) - cases.keys()
        if unknown:
            parser.error('Unknown compiler contracts: ' + ', '.join(sorted(unknown)))
        cases = {name: cases[name] for name in args.cases}
    with tempfile.TemporaryDirectory(prefix='teaql_cpp_type_errors_') as directory:
        source = Path(directory) / 'consumer.cpp'
        for name, (body, succeeds, diagnostic) in cases.items():
            business_header = ROOT / 'tests/generated_business_ids/include/cpp_business_ids.hpp'
            optional_header = ROOT / 'tests/generated_optional_types/include/cpp_optional_types.hpp'
            checker_header = ROOT / 'tests/generated_checkers/include/cpp_checker_fixture.hpp'
            inheritance_header = ROOT / 'tests/generated_inheritance/include/cpp_inheritance.hpp'
            candidate_header = ROOT / 'tests/generated_candidates/include/cpp_candidates.hpp'
            dynamic_header = ROOT / 'tests/generated_dynamic_json/include/cpp_dynamic_json.hpp'
            headers = ([dynamic_header] if name.startswith('expression_diagnostic_') else
                       [candidate_header] if name.startswith('candidate_') else
                       [header, business_header, optional_header, checker_header, inheritance_header])
            case_prelude = 'using namespace teaql;' if name.startswith(('candidate_', 'expression_diagnostic_')) else prelude
            source.write_text(''.join('#include "' + str(path) + '"\n' for path in headers) + case_prelude + '\n'
                              'void contract(UserContext& user_context) { ' + body + ' }\n')
            result = subprocess.run([*command, str(source)], cwd=record['directory'],
                                    text=True, capture_output=True)
            if (result.returncode == 0) != succeeds:
                raise SystemExit(f'Unexpected compiler outcome for {name}:\n{result.stdout[-1000:]}{result.stderr[-7000:]}')
            if not succeeds and diagnostic not in result.stderr:
                raise SystemExit(f'Unrelated compiler failure for {name}:\n{result.stderr[-7000:]}')
            print(f'Verified {name}', flush=True)
    positives = sum(succeeds for _, succeeds, _ in cases.values())
    print(f'Verified {positives} positive and {len(cases) - positives} negative generated API compiler contracts')


if __name__ == '__main__':
    main()
