#pragma once
#include <teaql/runtime/sqlite.hpp>
#include <array>
#include <iostream>

namespace example {
struct Models { std::array<const teaql::EntityDescriptor*, 3> all; };
inline Models& models(teaql::GlobalContext& global) {
    using namespace teaql;
    auto& order = global.make<EntityDescriptor>("Order");
    order.table("example_orders");
    order.property("id", DataType::U64).id();
    order.property("version", DataType::I64).version();
    order.property("name", DataType::Text);
    auto& line = global.make<EntityDescriptor>("OrderLine");
    line.table("example_orderline");
    line.property("id", DataType::U64).id();
    line.property("order_id", DataType::U64);
    line.property("name", DataType::Text);
    line.property("product_id", DataType::U64);
    auto& product = global.make<EntityDescriptor>("Product");
    product.table("example_product");
    product.property("id", DataType::U64).id();
    product.property("name", DataType::Text);
    auto& result = global.make<Models>();
    result.all = {&order, &line, &product};
    return result;
}
inline const teaql::Value& field(const teaql::Record& row, std::string_view name) {
    auto it = row.find(name);
    if (it == row.end()) throw std::runtime_error("Missing result field");
    return it->second;
}
inline const teaql::String& text(const teaql::Record& row, std::string_view name) { return std::get<teaql::String>(field(row, name).data); }
inline void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
inline void print_logs(const teaql::SqliteSession& session) {
    for (const auto& log : session.sql_logs()) {
        std::cout << "SQL " << log.sql << " | count=" << log.count;
        if (!log.audit_reason.empty()) std::cout << " | audit=" << log.audit_reason;
        if (!log.purpose.empty()) std::cout << " | purpose=" << log.purpose;
        std::cout << '\n';
    }
}
}
