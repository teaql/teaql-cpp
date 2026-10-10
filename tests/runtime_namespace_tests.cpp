#include <teaql/core.hpp>
#include <teaql/runtime.hpp>
#include "save_preparation_fixture.hpp"
#include <type_traits>
#include <iostream>

namespace rt = teaql::runtime;
using Model = save_fixture::Model;
static_assert(std::same_as<rt::entity_save::AuditedSaveExt<Model>, teaql::AuditedSaveExt<Model>>);
static_assert(&rt::entity_save::graph_node_from_entity<Model> ==
              &teaql::graph_node_from_entity<Model>);
static_assert(std::same_as<rt::generated_support::PurposedQuery<int>, teaql::PurposedQuery<int>>);
static_assert(std::same_as<rt::generated_support::PurposedSelectQuery, teaql::PurposedSelectQuery>);
static_assert(std::same_as<rt::inmemory_engine::ExprEvaluator, teaql::ExprEvaluator>);
static_assert(std::same_as<rt::inmemory_engine::InMemoryQueryEngine, teaql::InMemoryQueryEngine>);
static_assert(std::same_as<rt::log_formatter::SqlLogEntry, teaql::SqlLogEntry>);
static_assert(std::same_as<rt::log_formatter::SqlLogOperation, teaql::SqlLogOperation>);
static_assert(std::same_as<rt::log_formatter::LogFormatter, teaql::LogFormatter>);
static_assert(std::same_as<rt::log_formatter::HumanReaderFormatter, teaql::HumanReaderFormatter>);
static_assert(std::same_as<rt::log_formatter::DebugReaderFormatter, teaql::DebugReaderFormatter>);
static_assert(std::same_as<rt::log_formatter::LogFormatterFactory, teaql::LogFormatterFactory>);
static_assert(std::same_as<rt::log_formatter::LogLevel, teaql::LogLevel>);
static_assert(std::same_as<rt::log_formatter::LogConfig, teaql::LogConfig>);
static_assert(std::same_as<rt::log_formatter::LogManager, teaql::LogManager>);

int main() {
    teaql::GlobalContext global;
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& formatter = user->make<rt::log_formatter::HumanReaderFormatter>();
    auto& row = user->make<rt::log_formatter::SqlLogEntry>();
    auto text = formatter.format_sql_log(r, {}, row);
    if (text.empty()) {
        throw std::runtime_error(
            "published formatter namespace does not dispatch its existing implementation");
    }
    std::cout << "Runtime published module namespace contracts passed\n";
    return 0;
}
