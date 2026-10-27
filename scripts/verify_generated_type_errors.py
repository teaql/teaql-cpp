#!/usr/bin/env python3
"""Require real compiler rejection of invalid generated C++ application calls."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import re
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--cases', nargs='+', help='Run named compiler contracts; default is all')
    parser.add_argument('--jobs', type=int, default=4, help='Maximum independent compiler workers')
    parser.add_argument('--no-pch', action='store_true', help='Disable operation-local Clang header precompilation')
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
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
        'scalar_value_operand': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().withNameIs(42);', True, ''),
        'relation_selection_other_entity': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().selectLinesWith(Q.products());', True, ''),
        'extra_runtime_argument': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().purpose("why").executeForOne(user_context, 1);', False, 'executeForOne'),
        'global_factory': ('GlobalContext global; global.queries<cpp_examples::query_factory>();', False, 'queries'),
        'context_on_entry': ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines(user_context);', False, 'orderLines'),
        'temporary_entity': ('cpp_examples::E::order(Order(std::allocator_arg, Allocator(user_context.memory_resource())));', False, 'deleted'),
    }
    cases['runtime_state_borrow_lvalue'] = ('auto& value = user_context.make<Order>(); const auto& entity = value; static_assert(std::same_as<decltype(entity.runtime_state_any()), std::optional<AnyRef>>); auto borrow = entity.runtime_state_any(); auto state = detail::borrow_entity_runtime_state(entity);', True, '')
    cases['runtime_state_borrow_temporary'] = ('Order(std::allocator_arg, Allocator(user_context.memory_resource())).runtime_state_any();', False, 'deleted')
    cases['runtime_state_borrow_moved'] = ('auto& value = user_context.make<Order>(); std::move(value).runtime_state_any();', False, 'deleted')
    cases['missing_audit'] = ('user_context.make<Order>().save(user_context);', False, 'save')
    cases['runtime_configuration_positive'] = ('auto r = user_context.memory_resource(); cpp_examples::ServiceRuntimeConfig config(std::allocator_arg, Allocator(r), ":memory:"); config.databaseUrl(); cpp_examples::ServiceRuntimeConfig::fromEnv(user_context); cpp_examples::serviceRuntime(user_context,config); cpp_examples::serviceRuntimeFromEnv(user_context); auto connection = user_context.make_resource_ref<SqliteConnection>(":memory:"); cpp_examples::serviceRuntimeFromConnection(user_context,connection);', True, '')
    cases['runtime_configuration_missing_context'] = ('cpp_examples::ServiceRuntimeConfig::fromEnv();', False, 'fromEnv')
    cases['runtime_configuration_wrong_config'] = ('cpp_examples::serviceRuntime(user_context, ":memory:");', False, 'serviceRuntime')
    cases['runtime_configuration_raw_connection'] = ('auto& connection = user_context.make_resource<SqliteConnection>(":memory:"); cpp_examples::serviceRuntimeFromConnection(user_context, &connection);', False, 'serviceRuntimeFromConnection')
    cases['runtime_configuration_implicit_copy'] = ('cpp_examples::ServiceRuntimeConfig config(std::allocator_arg, Allocator(user_context.memory_resource()), ":memory:"); auto copied = config;', False, 'deleted')
    cases['runtime_configuration_extra_context'] = ('cpp_examples::serviceRuntimeFromEnv(user_context, user_context);', False, 'serviceRuntimeFromEnv')
    cases['business_startup_typed_result'] = ('auto connection = user_context.make_resource_ref<SqliteConnection>(":memory:"); auto startup = cpp_business_ids::serviceRuntimeFromConnection(user_context, connection); static_assert(std::same_as<decltype(startup), Result<Ref<cpp_business_ids::DataServiceExecutor>, cpp_business_ids::ServiceRuntimeError>>); static_assert(cpp_business_ids::BUSINESS_ID_KEY_HEX_ENV == "TEAQL_BUSINESS_ID_KEY_V1_HEX"); static_assert(cpp_business_ids::BUSINESS_ID_KEY_VERSION_ENV == "TEAQL_BUSINESS_ID_KEY_VERSION");', True, '')
    cases['business_startup_unchecked_reference'] = ('auto connection = user_context.make_resource_ref<SqliteConnection>(":memory:"); Ref<cpp_business_ids::DataServiceExecutor> executor = cpp_business_ids::serviceRuntimeFromConnection(user_context, connection);', False, 'no viable conversion')
    cases['business_startup_ordinary_constant'] = ('auto name = cpp_examples::BUSINESS_ID_KEY_HEX_ENV;', False, 'BUSINESS_ID_KEY_HEX_ENV')
    cases['standalone_registries_positive'] = ('auto& repository = cpp_examples::repositoryRegistry(user_context); repository.contains("Order"); auto& behavior = cpp_examples::behaviorRegistry(user_context); auto& checkers = cpp_examples::checkerRegistry(user_context); auto& schema = user_context.make<cpp_examples::LocalSchemaProvider>(); schema.get_entity("Order"); using Dialect = cpp_examples::DataServiceDialect; using Mutation = cpp_examples::DataServiceMutationExecutor; using Error = cpp_examples::DataServiceMutationError; using Ids = cpp_examples::DataServiceIdGenerator;', True, '')
    cases['business_allocator_alias_positive'] = ('static_assert(std::is_same_v<cpp_business_ids::DataServiceBusinessIdAllocator, teaql::SqliteBusinessIdAllocator>);', True, '')
    cases['business_allocator_alias_ordinary_domain'] = ('using Allocator = cpp_examples::DataServiceBusinessIdAllocator;', False, 'DataServiceBusinessIdAllocator')
    guide = (ROOT / 'examples/generated/ASSIST.md').read_text()
    registration = re.search(r'Sample progress:.*?through `([^`]+)`', guide).group(1)
    lookup = re.search(r'`(get\w+\([^`]+\))` returns a fresh Context-owned descriptor', guide).group(1)
    environment = re.search(r'reads the model-derived `([^`]+)`', guide).group(1)
    selection = re.search(r'std::move\(request.selection\(\)\).\w+\(\)', guide).group(0)
    cases['model_guide_runtime_positive'] = (
        'auto register_sink = [](UserContext& user_context, UserContext& owner, SampleDataLogSink& sink) { '
        + registration + '; }; auto& schema = user_context.make<cpp_examples::LocalSchemaProvider>(); '
        + 'std::string_view canonical_name = "Order"; schema.' + lookup + '; '
        + 'auto environment = cpp_examples::' + environment + '; '
        + 'auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.orders(); '
        + 'auto core = ' + selection + ';', True, '')
    cases['standalone_registries_missing_context'] = ('cpp_examples::repositoryRegistry();', False, 'repositoryRegistry')
    cases['standalone_registries_global_context'] = ('GlobalContext global; cpp_examples::behaviorRegistry(global);', False, 'behaviorRegistry')
    cases['standalone_registries_extra_context'] = ('cpp_examples::checkerRegistry(user_context,user_context);', False, 'checkerRegistry')
    cases['local_schema_unscoped'] = ('cpp_examples::LocalSchemaProvider schema;', False, 'LocalSchemaProvider')
    cases['sample_positive'] = ('cpp_examples::generateSampleData(user_context, SampleDataPlan::small());', True, '')
    cases['sample_wrong_plan'] = ('cpp_examples::generateSampleData(user_context, 42);', False, 'generateSampleData')
    cases['sample_extra_runtime'] = ('cpp_examples::generateSampleData(user_context, SampleDataPlan::small(), 1);', False, 'generateSampleData')
    cases['dynamic_json_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.orders().purpose("why").filterWithJson("{}").comment("what"); request.executeForList(user_context);', True, '')
    cases['ledger_composition_positive'] = ('auto& parent = user_context.make<cpp_examples::Order>(); auto& child = user_context.make<cpp_examples::OrderLine>(); const auto& receiver = parent; auto result = receiver.includePendingMutationsFrom(child);', True, '')
    cases['ledger_composition_invalid_source'] = ('auto& parent = user_context.make<cpp_examples::Order>(); parent.includePendingMutationsFrom(7);', False, 'includePendingMutationsFrom')
    cases['ledger_composition_extra_context'] = ('auto& parent = user_context.make<cpp_examples::Order>(); auto& child = user_context.make<cpp_examples::OrderLine>(); parent.includePendingMutationsFrom(child,user_context);', False, 'includePendingMutationsFrom')
    cases['relation_identity_positive'] = ('auto Q = user_context.queries<cpp_relation_identities::query_factory>(); auto parent = Q.entriesMinimal(); parent.selectNamespace().unselectNamespace().selectNamespaceWith(Q.owners().selectName()); Q.owners().selectClassWith(Q.entries()).selectTemplate().selectOperator(); auto& value = user_context.make<cpp_relation_identities::Entry>(); auto expression = cpp_relation_identities::E::entry(value).getNamespace(); auto legacy = cpp_relation_identities::E::entry(value).getModelNamespace();', True, '')
    cases['relation_identity_extra_context'] = ('auto Q = user_context.queries<cpp_relation_identities::query_factory>(); Q.entries().selectNamespace(user_context);', False, 'selectNamespace')
    cases['relation_identity_invalid_child'] = ('auto Q = user_context.queries<cpp_relation_identities::query_factory>(); Q.entries().selectNamespaceWith(7);', False, 'selectNamespaceWith')
    cases['relation_identity_purposed_child'] = ('auto Q = user_context.queries<cpp_relation_identities::query_factory>(); Q.entries().selectNamespaceWith(Q.owners().purpose("why"));', False, 'selectNamespaceWith')
    cases['relation_identity_extra_execution_argument'] = ('auto Q = user_context.queries<cpp_relation_identities::query_factory>(); Q.entries().selectNamespace().comment("what").purpose("why").executeForList(user_context,user_context);', False, 'executeForList')
    cases['expression_entry_existing_signatures'] = ('using ScalarFactory = RuntimeExpression<std::int64_t>(*)(eval::EvalResult<std::int64_t>,std::string_view); ScalarFactory original_scalar = &RuntimeExpression<std::int64_t>::from_result; using EntityFactory = RuntimeExpression<EntityBorrow<cpp_expression_roots::Class>>(*)(const cpp_expression_roots::Class&); EntityFactory original_entity = &entity_expression<cpp_expression_roots::Class>;', True, '')
    cases['expression_root_existing_entry_signature'] = ('using Factory = RuntimeExpression<EntityBorrow<cpp_expression_roots::Class>>(*)(const cpp_expression_roots::Class&,StaticString); Factory original = &entity_expression<cpp_expression_roots::Class>;', True, '')
    cases['expression_root_static_identity'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); auto expression = entity_expression(value,StaticString{"modelClass"},StaticString{"Class"});', True, '')
    cases['expression_root_dynamic_identity'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); String name("Class",user_context.memory_resource()); entity_expression(value,StaticString{"modelClass"},name.c_str());', False, 'entity_expression')
    cases['expression_root_local_identity'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); const char name[]="Class"; entity_expression(value,StaticString{"modelClass"},StaticString{name});', False, 'StaticString')
    cases['expression_root_temporary_entity'] = ('entity_expression(cpp_expression_roots::Class(std::allocator_arg,Allocator(user_context.memory_resource())),StaticString{"modelClass"},StaticString{"Class"});', False, 'deleted')
    cases['expression_entry_positive'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); cpp_expression_roots::E::modelClass(value).getName(); auto named = entity_expression(value, StaticString{"modelClass"}); cpp_expression_roots::E::modelOperator(user_context.make<cpp_expression_roots::Operator>()); cpp_expression_roots::E::modelTemplate(user_context.make<cpp_expression_roots::Template>()); cpp_expression_roots::E::modelNamespace(user_context.make<cpp_expression_roots::Namespace>());', True, '')
    cases['expression_entry_extra_context'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); cpp_expression_roots::E::modelClass(value,user_context);', False, 'modelClass')
    cases['expression_entry_dynamic_name'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); String name("modelClass",user_context.memory_resource()); entity_expression(value,name.c_str());', False, 'entity_expression')
    cases['expression_entry_local_name'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); const char name[]="modelClass"; entity_expression(value,StaticString{name});', False, 'StaticString')
    cases['expression_entry_temporary_entity'] = ('entity_expression(cpp_expression_roots::Class(std::allocator_arg,Allocator(user_context.memory_resource())),StaticString{"modelClass"});', False, 'deleted')
    cases['expression_entry_keyword_spelling'] = ('auto& value = user_context.make<cpp_expression_roots::Class>(); cpp_expression_roots::E::class(value);', False, 'expected unqualified-id')
    cases['scalar_diagnostic_positive'] = ('auto Q = user_context.queries<cpp_query_aliases::query_factory>(); auto query = Q.recordsMinimal().selectClass().unselectClass().selectOperator().selectTemplate(); query.selectModelClass(); auto& record = user_context.make<cpp_query_aliases::Record>(); cpp_query_aliases::E::record(record).getClass(); cpp_query_aliases::E::record(record).getOperator(); cpp_query_aliases::E::record(record).getTemplate();', True, '')
    cases['scalar_diagnostic_extra_context'] = ('auto Q = user_context.queries<cpp_query_aliases::query_factory>(); Q.recordsMinimal().selectClass(user_context);', False, 'selectClass')
    cases['scalar_diagnostic_invalid_child'] = ('auto Q = user_context.queries<cpp_query_aliases::query_factory>(); Q.recordsMinimal().selectClassWith(Q.platforms());', False, 'selectClassWith')
    cases['scalar_diagnostic_temporary_entity'] = ('cpp_query_aliases::E::record(user_context.make<cpp_query_aliases::Record>()).getClass(); cpp_query_aliases::E::record(cpp_query_aliases::Record(std::allocator_arg, Allocator(user_context.memory_resource()))).getClass();', False, 'deleted')
    cases['scalar_diagnostic_missing_purpose'] = ('auto Q = user_context.queries<cpp_query_aliases::query_factory>(); Q.recordsMinimal().selectClass().comment("what").executeForList(user_context);', False, 'executeForList')
    cases['query_alias_positive'] = ('auto Q = user_context.queries<cpp_query_aliases::query_factory>(); Q.recordsMinimal().sumModelClass().sumType().countModelNamespace().comment("inspect aliases").purpose("retain source keys").executeForRows(user_context);', True, '')
    cases['query_alias_wrong_function'] = ('auto Q = user_context.queries<cpp_query_aliases::query_factory>(); Q.records().selectModelClassWithFunction("SUM");', False, 'selectModelClassWithFunction')
    cases['general_query_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().withDeletedRows().deletedRowsOnly().pageNumber(2, 3).offset(1, 2).offsetOnly(3).top(2).withNameNotContaining("x").selectNameRaw("name").unselectName();', True, '')
    cases['request_state_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.orders(); auto options = request.queryOptions(); auto relations = request.relationSelections(); auto filters = request.relationFilters(); auto children = request.childEnhancements(); auto selection = request.selection(); auto complete = std::move(selection).into_query(); auto core = std::move(request).intoQuery();', True, '')
    cases['request_state_lvalue_consume'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.orders(); request.intoQuery();', False, 'intoQuery')
    cases['request_state_purposed_consume'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().purpose("why").intoQuery();', False, 'intoQuery')
    cases['request_state_purposed_options'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().purpose("why").queryOptions();', False, 'queryOptions')
    cases['request_state_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().queryOptions(user_context);', False, 'queryOptions')
    cases['core_query_view_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.orders().comment("inspect core"); const auto& constant = request; auto core = constant.coreQuery(); const auto& complete = request.query(); auto consumed = std::move(request).intoQuery();', True, '')
    cases['core_query_view_purposed'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().purpose("why").coreQuery();', False, 'coreQuery')
    cases['core_query_view_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().coreQuery(user_context);', False, 'coreQuery')
    cases['matching_statistics_inputs_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto other_Q = user_context.queries<cpp_dynamic_json::query_factory>(); auto child = other_Q.people().comment("child intent"); auto selected = child.selection(); auto core = std::move(child.selection()).into_query(); Q.orderLines().withOrderMatching(child).withoutOrderMatching(selected).rollUpToOrderWith(core); auto aggregate = other_Q.orders(); Q.orders().withLinesMatching(aggregate).withoutLinesMatching(selected); other_Q.people().sumAmountOfOrdersAs("_sum", core).avgAmountOfOrdersAs("_avg", selected);', True, '')
    cases['matching_statistics_inputs_purposed'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().withOrderMatching(Q.orders().purpose("why"));', False, 'withOrderMatching')
    cases['matching_statistics_inputs_invalid'] = ('auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.people().sumAmountOfOrdersAs("_sum",7);', False, 'sumAmountOfOrdersAs')
    cases['matching_statistics_inputs_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().rollUpToOrderWith(Q.orders(),user_context);', False, 'rollUpToOrderWith')
    cases['analysis_inputs_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto other_Q = user_context.queries<cpp_dynamic_json::query_factory>(); auto child = other_Q.people().comment("facet intent"); auto selected = child.selection(); auto core = std::move(child.selection()).into_query(); Q.orderLines().facetByOrderAs("_facet", child).facetByOrderAsWithOptions("_facet", selected, false).groupByOrderWith(core).groupByOrderWithDetailsFrom(child); Q.orders().countLinesWith("_count", selected).statsFromLines(child).statsFromLinesAs("_stats", core).groupByLinesWithDetails(selected);', True, '')
    cases['analysis_inputs_purposed'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().facetByOrderAs("_facet", Q.orders().purpose("why"));', False, 'facetByOrderAs')
    cases['analysis_inputs_invalid'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().statsFromLines(7);', False, 'statsFromLines')
    cases['analysis_inputs_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().groupByOrderWith(Q.orders(),user_context);', False, 'groupByOrderWith')
    cases['relation_selection_inputs_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto other_Q = user_context.queries<cpp_dynamic_json::query_factory>(); auto child = other_Q.people().selectName(); auto parent = Q.orderLines(); parent.selectProductWith(child); auto selection = child.selection(); parent.selectProductWith(selection); auto core = std::move(child.selection()).into_query(); std::move(parent).selectProductWith(core); Q.orders().selectLinesWith(child);', True, '')
    cases['relation_selection_inputs_purposed'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().selectLinesWith(Q.products().purpose("why"));', False, 'selectLinesWith')
    cases['relation_selection_inputs_invalid'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().selectLinesWith(7);', False, 'selectLinesWith')
    cases['relation_selection_inputs_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().selectLinesWith(Q.products(),user_context);', False, 'selectLinesWith')
    cases['enhancement_inputs_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto other_Q = user_context.queries<cpp_dynamic_json::query_factory>(); auto child = other_Q.people().selectName(); auto parent = Q.orders(); parent.enhanceChild(child); auto selection = child.selection(); parent.enhanceChild(selection); auto core = std::move(child.selection()).into_query(); std::move(parent).enhanceChild(core);', True, '')
    cases['enhancement_inputs_purposed_child'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto other_Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.orders().enhanceChild(other_Q.people().purpose("why"));', False, 'enhanceChild')
    cases['enhancement_inputs_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().enhanceChild(Q.products(),user_context);', False, 'enhanceChild')
    cases['type_match_ranges_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); std::array<std::string_view,2> names{"Order","Product"}; Q.orders().matchTypes(names); std::array<std::int64_t,2> numbers{1,2}; Q.products().matchTypes(numbers); Vector<Value> values(user_context.memory_resource()); values.emplace_back("Order"); Q.orders().matchTypes(std::span<const Value>(values));', True, '')
    cases['type_match_ranges_bad_elements'] = ('struct Unknown {}; std::array<Unknown,1> values{}; auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().matchTypes(values);', False, 'matchTypes')
    cases['type_match_ranges_scalar'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().matchTypes(1);', False, 'matchTypes')
    cases['scalar_membership_ranges_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); std::array<std::int64_t, 2> ids{1,2}; Q.orders().withIdIn(ids).withIdNotIn(ids); std::array<std::string_view, 2> names{"A","B"}; Q.products().withNameIn(names).withNameNotIn(names); Vector<Value> values(user_context.memory_resource()); values.emplace_back("A"); Q.products().withNameIn(values);', True, '')
    cases['scalar_membership_bad_elements'] = ('struct Unknown {}; std::array<Unknown,1> values{}; auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().withNameIn(values);', False, 'withNameIn')
    cases['scalar_comparison_values_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Value value(user_context.memory_resource()); Q.products().withNameIs(value).withNameIsNot(value).withNameGreaterThan(value).withNameBetween(std::int64_t(1), std::string_view("3")); Q.orders().withIdIs(value);', True, '')
    cases['scalar_comparison_bad_value'] = ('struct Unknown {}; auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().withNameIs(Unknown{});', False, 'withNameIs')
    cases['sound_value_positive'] = ('Value operand(user_context.memory_resource()); auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.people().whoseNameSoundsLike(operand); Q.orders().withCodeSoundingLike(operand);', True, '')
    cases['sound_optional_value'] = ('std::optional<std::string_view> operand; auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.people().whoseNameSoundsLike(operand);', False, 'whoseNameSoundsLike')
    cases['scalar_optional_value'] = ('std::optional<std::string_view> operand; auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.people().whoseNameIs(operand);', False, 'whoseNameIs')
    cases['query_branches_positive'] = ('auto Q = user_context.queries<cpp_query_branches::query_factory>(); Value input(user_context.memory_resource()); Q.entries().withOptionalLabelIs(input).withNotesIs(input).withSecretIs(input).withOptionalAmountBetween(input,input).withMomentBefore(input).withOptionalDayIsKnown().whichAreActive().whichAreNotOptionalActive().orderByVersionDesc();', True, '')
    cases['query_branches_extensions_positive'] = ('auto Q = user_context.queries<cpp_query_branches::query_factory>(); Value input(user_context.memory_resource(), std::uint64_t{7}); Q.entries().selectRatioWithFunction(AggregateFunction::Sum).selectRatioAsWithFunction("total", AggregateFunction::Sum).standardDeviationRatio().squareRootOfPopulationStandardDeviationRatioAs("population").sampleVarianceRatio().samplePopulationVarianceRatioAs("variance").filterByTenant(input).filterByTenant(std::uint64_t{7});', True, '')
    cases['query_branches_bool_generic'] = ('auto Q = user_context.queries<cpp_query_branches::query_factory>(); std::array<Value,1> values{Value(user_context.memory_resource())}; Q.entries().withActive(request::FieldOperator::Equal,values);', False, 'withActive')
    cases['query_branches_password_sound_value'] = ('auto Q = user_context.queries<cpp_query_branches::query_factory>(); Value input(user_context.memory_resource()); Q.entries().withSecretSoundingLike(input);', False, 'withSecretSoundingLike')
    cases['query_branches_text_sound_value'] = ('auto Q = user_context.queries<cpp_query_branches::query_factory>(); Value input(user_context.memory_resource()); Q.entries().withNotesSoundingLike(input);', False, 'withNotesSoundingLike')
    cases['query_branches_id_before'] = ('auto Q = user_context.queries<cpp_query_branches::query_factory>(); Q.entries().withIdBefore(1);', False, 'withIdBefore')
    cases['query_branches_label_function'] = ('auto Q = user_context.queries<cpp_query_branches::query_factory>(); Q.entries().selectLabelWithFunction(AggregateFunction::Count);', False, 'selectLabelWithFunction')
    cases['scalar_comparison_business_id_value'] = ('Value value(user_context.memory_resource()); auto Q = user_context.queries<cpp_business_ids::query_factory>(); Q.receipts().withReceiptNumberIs(value);', False, 'withReceiptNumberIs')
    cases['scalar_membership_business_id_values'] = ('Vector<Value> values(user_context.memory_resource()); auto Q = user_context.queries<cpp_business_ids::query_factory>(); Q.receipts().withReceiptNumberIn(values);', False, 'withReceiptNumberIn')
    cases['aggregation_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto query = Q.productsMinimal().groupByNameAs("group").countName().sumId().sumVersion().aggregateWithFunction("id", "total", teaql::AggregateFunction::Sum).enableAggregationCacheFor(250).purpose("inspect statistics"); query.comment("aggregate fields").countAs("rows");', True, '')
    cases['facets_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().facetByOrderAsWithOptions("_orders", Q.orders().comment("facet counts").countLinesAs("_lines"), false).purpose("inspect facets").comment("outer list").executeForList(user_context);', True, '')
    cases['facet_analysis_other_entity'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orderLines().facetByOrderAs("_orders", Q.products());', True, '')
    cases['relationship_analysis_positive'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); auto lines = Q.orderLines().withDeletedRows(); Q.orders().countLinesWith("_count", lines).statsFromLinesAs("_stats", lines).purpose("inspect members").comment("aggregate relation").executeForList(user_context); Q.orderLines().rollUpToOrderWith(Q.orders());', True, '')
    cases['relation_aggregate_other_entity'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().countLinesWith("_count", Q.products());', True, '')
    cases['aggregation_wrong_function'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().aggregateWithFunction("id", "total", "SUM");', False, 'aggregateWithFunction')
    cases['raw_segment_wrong_type'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().unsafeRawSql("SELECT 1");', False, 'unsafeRawSql')
    cases['dynamic_json_wrong_input'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.orders().filterWithJson(42);', False, 'filterWithJson')
    cases['execution_extensions_positive'] = ('static_assert(!std::same_as<cpp_examples::DataServiceExecutor, cpp_business_ids::DataServiceExecutor>); auto Q = user_context.queries<cpp_examples::query_factory>(); auto request = Q.products().comment("what").purpose("why"); request.executeForCount(user_context); request.executeForExists(user_context); request.executeForFirst(user_context); request.executeForPage(user_context, 0, 10);', True, '')
    cases['count_missing_purpose'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().comment("what").executeForCount(user_context);', False, 'executeForCount')
    cases['page_extra_context'] = ('auto Q = user_context.queries<cpp_examples::query_factory>(); Q.products().purpose("why").executeForPage(user_context, user_context, 0, 10);', False, 'executeForPage')
    execution_entries = ('List', 'Rows', 'Stream', 'First', 'One', 'Page', 'Count', 'Exists')
    execution_prefix = 'auto Q = user_context.queries<cpp_examples::query_factory>(); '
    execution_calls = []
    for entry in execution_entries:
        method = 'executeFor' + entry
        arguments = 'user_context, 0, 10' if entry == 'Page' else 'user_context'
        execution_calls.append(f'request.{method}({arguments});')
        cases['execution_matrix_' + entry.lower() + '_missing_purpose'] = (
            execution_prefix + f'Q.products().comment("what").{method}({arguments});',
            False, method)
        cases['execution_matrix_' + entry.lower() + '_extra_context'] = (
            execution_prefix +
            f'Q.products().comment("what").purpose("why").{method}({arguments}, user_context);',
            False, method)
        wrong_context_arguments = 'global, 0, 10' if entry == 'Page' else 'global'
        cases['execution_matrix_' + entry.lower() + '_global_context'] = (
            'GlobalContext global; ' + execution_prefix +
            f'Q.products().comment("what").purpose("why").{method}({wrong_context_arguments});',
            False, method)
    cases['execution_matrix_all_positive'] = (
        execution_prefix + 'auto request = Q.products().comment("what").purpose("why"); ' +
        ' '.join(execution_calls), True, '')
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
    cases['optional_timestamp_predicate'] = ('auto Q = user_context.queries<cpp_optional_types::query_factory>(); Q.readings().withRecordedAtIs("raw timestamp");', True, '')
    cases['context_root_getters'] = ('auto& note = user_context.make<cpp_checker_fixture::Note>(); note.tenantId(); note.changedTenantId(); note.evalTenantId(); note.tenant(); note.evalTenant(); auto Q = user_context.queries<cpp_checker_fixture::query_factory>(); Q.notes().withTenantIs(42).selectTenant();', True, '')
    cases['context_root_id_setter'] = ('user_context.make<cpp_checker_fixture::Note>().updateTenantId(42);', False, 'private')
    cases['context_root_object_setter'] = ('auto& tenant = user_context.make<cpp_checker_fixture::Tenant>(); user_context.make<cpp_checker_fixture::Note>().updateTenant(tenant);', False, 'private')
    cases['business_context_root_setter'] = ('user_context.make<cpp_business_ids::Receipt>().updateTenantId(42);', False, 'private')
    cases['inheritance_positive'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schoolsWithChildren().isAcademy().enhanceChild(Q.academies()).comment("what").purpose("why").executeForOne(user_context); Q.academies().isResearchAcademy();', True, '')
    cases['inherited_json_analysis_positive'] = ('auto Q = user_context.queries<cpp_inheritance_graph::query_factory>(); auto child = Q.specialistsMinimal().comment("inspect advisors"); Q.researchAcademies().filterWithJson(R"({"name":"A","advisor.name":"B"})").countAdvisor().groupByAdvisorWithDetails().facetByAdvisorAs("advisors", child);', True, '')
    cases['inheritance_unknown_subtype'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().isResearchAcademy();', False, 'isResearchAcademy')
    cases['inheritance_missing_purpose'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schoolsWithChildren().comment("what").executeForList(user_context);', False, 'executeForList')
    cases['inheritance_untyped_child'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().enhanceChild(42);', False, 'query_')
    cases['return_type_positive'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); auto adapted = Q.schoolsWithChildren().returnType<cpp_inheritance::ResearchAcademy>().purpose("why").comment("what"); adapted.executeForList(user_context); adapted.executeForOne(user_context); adapted.executeForStream(user_context); static_assert(std::same_as<decltype(adapted.newEntity(user_context)), cpp_inheritance::School&>); Q.schools().enhanceChild(Q.academies().returnType<cpp_inheritance::ResearchAcademy>());', True, '')
    cases['return_type_marker_positive'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); auto marker = Q.schools().returnType<int>(); marker.limit(2).comment("what").enhanceChild(Q.academies().returnType<std::pair<int, int>>()); marker.queryOptions(); marker.coreQuery(); marker.selection(); auto restored = std::move(marker).returnType<cpp_inheritance::School>().purpose("why"); restored.executeForCount(user_context); restored.executeForRows(user_context); static_assert(std::same_as<decltype(restored.newEntity(user_context)), cpp_inheritance::School&>);', True, '')
    for operation in ['executeForList', 'executeForRows', 'executeForStream', 'executeForFirst',
                      'executeForOne', 'executeForPage', 'executeForCount', 'executeForExists',
                      'newEntity', 'execute', 'executeForEntity', 'executeForEntityList']:
        arguments = 'user_context, 0, 1' if operation == 'executeForPage' else 'user_context'
        cases['return_type_marker_refuses_' + operation] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().returnType<int>().comment("what").purpose("why").' + operation + '(' + arguments + ');', False, operation)
    cases['return_type_missing_purpose'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().returnType<cpp_inheritance::ResearchAcademy>().executeForList(user_context);', False, 'executeForList')
    cases['return_type_extra_context'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.schools().returnType<cpp_inheritance::ResearchAcademy>().purpose("why").executeForOne(user_context, user_context);', False, 'executeForOne')
    cases['business_lookup_positive'] = ('auto Q = user_context.queries<cpp_business_ids::query_factory>(); auto receipt = cpp_business_ids::ReceiptNumber::create(user_context.memory_resource(), "match"); auto ticket = cpp_business_ids::TicketNumber::create(user_context.memory_resource(), "match"); auto request = Q.receiptByReceiptNumber(receipt.value()); Q.ticketByTicketNumber(ticket.value()); static_assert(std::same_as<decltype(request), cpp_business_ids::ReceiptRequest<false>>); request.comment("what").purpose("why").executeForList(user_context);', True, '')
    cases['business_lookup_inherited_positive'] = ('auto Q = user_context.queries<cpp_inheritance_business_ids::query_factory>(); auto number = cpp_inheritance_business_ids::DocumentNumber::create(user_context.memory_resource(), "match"); Q.documentByDocumentNumber(number.value()).returnType<cpp_inheritance_business_ids::Invoice>();', True, '')
    cases['business_lookup_raw_text'] = ('auto Q = user_context.queries<cpp_business_ids::query_factory>(); Q.receiptByReceiptNumber("raw");', False, 'receiptByReceiptNumber')
    cases['business_lookup_wrong_identifier'] = ('auto Q = user_context.queries<cpp_business_ids::query_factory>(); auto number = cpp_business_ids::TicketNumber::create(user_context.memory_resource(), "match"); Q.receiptByReceiptNumber(number.value());', False, 'receiptByReceiptNumber')
    cases['business_lookup_extra_context'] = ('auto Q = user_context.queries<cpp_business_ids::query_factory>(); auto number = cpp_business_ids::ReceiptNumber::create(user_context.memory_resource(), "match"); Q.receiptByReceiptNumber(number.value(), user_context);', False, 'receiptByReceiptNumber')
    cases['business_lookup_missing_purpose'] = ('auto Q = user_context.queries<cpp_business_ids::query_factory>(); auto number = cpp_business_ids::ReceiptNumber::create(user_context.memory_resource(), "match"); Q.receiptByReceiptNumber(number.value()).executeForList(user_context);', False, 'executeForList')
    cases['business_lookup_no_inherited_alias'] = ('auto Q = user_context.queries<cpp_inheritance_business_ids::query_factory>(); auto number = cpp_inheritance_business_ids::DocumentNumber::create(user_context.memory_resource(), "match"); Q.invoiceByDocumentNumber(number.value());', False, 'invoiceByDocumentNumber')
    cases['candidate_diagnostic_repairs'] = ('auto Q = user_context.queries<cpp_candidates::query_factory>(); Q.recordsMinimal().selectStatusId().selectPhase().selectStatus();', True, '')
    cases['inherited_diagnostic_repairs'] = ('auto Q = user_context.queries<cpp_inheritance::query_factory>(); Q.researchAcademiesMinimal().selectName().selectCampusCode().selectBudget();', True, '')
    cases['expression_diagnostic_dynamic_repairs'] = ('auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.peopleMinimal().selectPlatformId().selectPlatformWith(Q.platformsMinimal().selectName()).selectProfileWith(Q.profilesMinimal().selectNote()).selectOrdersWith(Q.ordersMinimal().selectCode());', True, '')
    cases['expression_diagnostic_other_child'] = ('auto Q = user_context.queries<cpp_dynamic_json::query_factory>(); Q.peopleMinimal().selectPlatformWith(Q.ordersMinimal().selectCode());', True, '')
    cases['checker_customization_positive'] = ('''
    struct Rules final : cpp_checker_fixture::NoteCheckerLogic {
        using NoteCheckerLogic::NoteCheckerLogic;
        void checkAndFixNote(const UserContext&, cpp_checker_fixture::Note& entity,
                             CheckObjectStatus, const ObjectLocation& location,
                             CheckResults& results) const override {
            requiredText(entity.name(), "name", location, results);
        }
    };
    struct Ordering final : cpp_checker_fixture::NoteBehavior {
        using NoteBehavior::NoteBehavior;
        RuntimeHookResult before_select(Memory r, const UserContext&,
                                        SelectQuery& query) const override {
            query.order_by.emplace_back(OrderBy::asc(r, "name"));
            return RuntimeHookResult::ok(r, {});
        }
    };
    auto& rules = user_context.make<Rules>();
    auto& checker = user_context.make<cpp_checker_fixture::NoteChecker>(rules);
    auto& adapter = user_context.make<TypedEntityChecker<
        cpp_checker_fixture::Note, cpp_checker_fixture::NoteChecker>>(
            user_context, checker);
    auto& behavior = user_context.make<Ordering>();
    auto& module = cpp_checker_fixture::moduleWithBehaviorsAndCheckers(user_context);
    module.checker(user_context, adapter);
    module.entity_with_behavior<cpp_checker_fixture::Note>(user_context, behavior);
    module.apply_to(user_context);
''', True, '')
    cases['checker_wrong_logic'] = ('''
    auto& logic = user_context.make<cpp_checker_fixture::TenantCheckerLogic>();
    user_context.make<cpp_checker_fixture::NoteChecker>(logic);
''', False, 'NoteChecker')
    cases['checker_wrong_adapter'] = ('''
    auto& checker = user_context.make<cpp_checker_fixture::TenantChecker>();
    auto& adapter = user_context.make<TypedEntityChecker<
        cpp_checker_fixture::Note, cpp_checker_fixture::TenantChecker>>(
            user_context, checker);
    user_context.make<InMemoryCheckerRegistry>().register_checker(user_context, adapter);
''', False, 'constraints not satisfied')
    cases['checker_wrong_entity'] = ('''
    auto& checker = user_context.make<cpp_checker_fixture::NoteChecker>();
    auto& tenant = user_context.make<cpp_checker_fixture::Tenant>();
    ObjectLocation location(std::allocator_arg, Allocator(user_context.memory_resource()));
    CheckResults results(user_context.memory_resource());
    checker.check_and_fix_typed(user_context, tenant, CheckObjectStatus::Create,
                                location, results);
''', False, 'Note')
    cases['checker_wrong_callback'] = ('''
    struct WrongRules final : cpp_checker_fixture::NoteCheckerLogic {
        using NoteCheckerLogic::NoteCheckerLogic;
        void checkAndFixNote(const UserContext&, cpp_checker_fixture::Tenant&,
                             CheckObjectStatus, const ObjectLocation&,
                             CheckResults&) const override {}
    };
''', False, 'override')
    cases['wire_metadata_positive'] = ('''
    auto& mappings = cpp_examples::generatedWireFieldMappings(user_context);
    auto& aliases = cpp_examples::generatedWireFieldAliases(user_context);
    mappings.find(std::string_view("Order"));
    aliases.empty();
''', True, '')
    cases['wire_metadata_global_argument'] = ('''
    GlobalContext global;
    cpp_examples::generatedWireFieldMappings(global);
''', False, 'UserContext')
    cases['wire_metadata_extra_argument'] = ('''
    cpp_examples::generatedWireFieldAliases(user_context, user_context);
''', False, 'generatedWireFieldAliases')
    cases['business_id_expression_fallback'] = ('''
    auto& entity = user_context.make<cpp_business_ids::Receipt>();
    auto value = cpp_business_ids::ReceiptNumber::create(user_context.memory_resource(), "fallback");
    cpp_business_ids::E::receipt(entity).getReceiptNumber().orIfNull(std::move(value).value());
''', True, '')
    cases['business_id_expression_default_forbidden'] = ('''
    auto& entity = user_context.make<cpp_business_ids::Receipt>();
    cpp_business_ids::E::receipt(entity).getReceiptNumber().orDefaultIfNull();
''', False, 'orDefaultIfNull')
    if args.cases:
        unknown = set(args.cases) - cases.keys()
        if unknown:
            parser.error('Unknown compiler contracts: ' + ', '.join(sorted(unknown)))
        cases = {name: cases[name] for name in args.cases}
    with tempfile.TemporaryDirectory(prefix='teaql_cpp_type_errors_') as directory:
        precompiled = {}
        use_pch = not args.no_pch and 'clang' in Path(command[0]).name
        records = []
        for name, (body, succeeds, diagnostic) in cases.items():
            business_header = ROOT / 'tests/generated_business_ids/include/cpp_business_ids.hpp'
            optional_header = ROOT / 'tests/generated_optional_types/include/cpp_optional_types.hpp'
            checker_header = ROOT / 'tests/generated_checkers/include/cpp_checker_fixture.hpp'
            inheritance_header = ROOT / 'tests/generated_inheritance/include/cpp_inheritance.hpp'
            inheritance_graph_header = ROOT / 'tests/generated_inheritance_graph/include/cpp_inheritance_graph.hpp'
            inherited_business_header = ROOT / 'tests/generated_inheritance_business_ids/include/cpp_inheritance_business_ids.hpp'
            candidate_header = ROOT / 'tests/generated_candidates/include/cpp_candidates.hpp'
            dynamic_header = ROOT / 'tests/generated_dynamic_json/include/cpp_dynamic_json.hpp'
            query_branches_header = ROOT / 'tests/generated_query_field_branches/include/cpp_query_branches.hpp'
            query_alias_header = ROOT / 'tests/generator/parity/query_alias_contracts/cpp/include/cpp_query_aliases.hpp'
            relation_identity_header = ROOT / 'tests/generator/parity/relation_identity_contracts/cpp/include/cpp_relation_identities.hpp'
            expression_roots_header = ROOT / 'tests/generator/parity/expression_root_contracts/cpp/include/cpp_expression_roots.hpp'
            headers = ([inherited_business_header] if 'cpp_inheritance_business_ids::' in body else
                       [business_header] if name.startswith('business_lookup_') else
                       [expression_roots_header] if 'cpp_expression_roots::' in body else
                       [relation_identity_header] if 'cpp_relation_identities::' in body else
                       [query_alias_header] if 'cpp_query_aliases::' in body else
                       [header, dynamic_header] if name.startswith(('enhancement_inputs_', 'relation_selection_inputs_', 'analysis_inputs_', 'matching_statistics_inputs_')) else
                       [header] if name.startswith(('wire_metadata_', 'execution_matrix_')) else
                       [inheritance_graph_header] if 'cpp_inheritance_graph::' in body else
                       [query_branches_header] if 'cpp_query_branches::' in body else
                       [checker_header] if name.startswith('checker_') else
                       [dynamic_header] if name.startswith('expression_diagnostic_') or 'cpp_dynamic_json::' in body else
                       [candidate_header] if name.startswith('candidate_') else
                       [header, business_header, optional_header, checker_header, inheritance_header])
            case_prelude = 'using namespace teaql;' if name.startswith(('business_lookup_', 'checker_', 'candidate_', 'expression_diagnostic_')) or 'cpp_dynamic_json::' in body or 'cpp_query_branches::' in body or 'cpp_query_aliases::' in body or 'cpp_relation_identities::' in body or 'cpp_expression_roots::' in body or 'cpp_inheritance_graph::' in body else prelude
            prologue = ''.join('#include "' + str(path) + '"\n' for path in headers) + case_prelude + '\n'
            records.append((name, body, succeeds, diagnostic, prologue))
            if use_pch and prologue not in precompiled:
                # Producers must finish before the independent consumers start.
                prefix = Path(directory) / ('headers_' + str(len(precompiled)))
                common = prefix.with_suffix('.hpp')
                output = prefix.with_suffix('.pch')
                common.write_text(prologue)
                producer = [part for part in command if part != '-fsyntax-only']
                prepared = subprocess.run([*producer, '-c', '-x', 'c++-header', str(common), '-o', str(output)],
                                          cwd=record['directory'], text=True, capture_output=True)
                if prepared.returncode:
                    raise SystemExit('Generated header precompilation failed:\n' + prepared.stderr[-7000:])
                precompiled[prologue] = output

        def verify(item):
            name, body, succeeds, diagnostic, prologue = item
            source = Path(directory) / (name + '.cpp')
            flags = []
            if use_pch:
                flags = ['-include-pch', str(precompiled[prologue])]
                prologue = ''
            source.write_text(prologue + 'void contract(UserContext& user_context) { ' + body + ' }\n')
            result = subprocess.run([*command, *flags, str(source)], cwd=record['directory'],
                                    text=True, capture_output=True)
            if (result.returncode == 0) != succeeds:
                raise RuntimeError(f'Unexpected compiler outcome for {name}:\n{result.stdout[-1000:]}{result.stderr[-7000:]}')
            if not succeeds and diagnostic not in result.stderr:
                raise RuntimeError(f'Unrelated compiler failure for {name}:\n{result.stderr[-7000:]}')
            print(f'Verified {name}', flush=True)

        # Every case has a distinct source; temporary headers and PCHs are immutable.
        # Exiting the executor joins all producers before temporary files are reclaimed.
        with ThreadPoolExecutor(max_workers=args.jobs) as workers:
            list(workers.map(verify, records))
    positives = sum(succeeds for _, succeeds, _ in cases.values())
    print(f'Verified {positives} positive and {len(cases) - positives} negative generated API compiler contracts')


if __name__ == '__main__':
    main()
