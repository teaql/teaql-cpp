#include <teaql/runtime.hpp>
using Engine = teaql::runtime::inmemory_engine::InMemoryQueryEngine;
using Formatter = teaql::runtime::log_formatter::LogFormatter;
using Saved = teaql::runtime::entity_save::AuditedSaveExt<int>;
using Purpose = teaql::runtime::generated_support::PurposedSelectQuery;
