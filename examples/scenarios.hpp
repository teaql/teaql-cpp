#pragma once
#include "models.hpp"
#include <tuple>

namespace example {
inline Order
make_order(const UserContext& user_context, std::int64_t version, std::string_view name) {
    Order order(std::allocator_arg, Allocator(user_context.memory_resource()));
    order.id = 1;
    order.version_value = version;
    order.name = name;
    return order;
}

// Preserve the pinned example's business calls and reasons; supply the comment
// required by the 5.0.6 request contract at the query entry.
inline const SmartList<Order>& schema_crud(UserContext& user_context) {
    auto r = user_context.memory_resource();
    setup(user_context);
    auto service = user_context.entity_data_service<Executor>("Order");
    auto& repository = value(service).get();
    value(AuditedSaveExt<Order>::save(
              std::move(make_order(user_context, 1, "draft")).audit_as("Create the example order"),
              user_context)
              .wait());
    value(AuditedSaveExt<Order>::save(std::move(make_order(user_context, 1, "submitted"))
                                          .audit_as("Submit the example order"),
                                      user_context)
              .wait());
    auto deleted = make_order(user_context, 2, "submitted");
    deleted.root.mark_as_delete(deleted.entity_key());
    value(AuditedSaveExt<Order>::save(std::move(deleted).audit_as("Delete the example order"),
                                      user_context)
              .wait());
    auto query = repository.select();
    query.comment("Inspect the soft-deleted example order");
    query.project("id").project("version").project("name").filter(
        Expr::eq(r, "id", std::uint64_t(1)));
    PurposedSelectQuery purpose(
        std::allocator_arg, Allocator(r), query, "Inspect the soft-deleted example order");
    const auto& orders = value(repository.fetch_entities<Order>(purpose).wait());
    require(orders.len() == 1 && orders[0].id == 1 && orders[0].version() == -3 &&
                orders[0].name == "submitted",
            "1 -> 2 -> -3 version transitions");
    return orders;
}

// Saving a typed root delegates traversal, planning, transactions and child
// mutations to the runtime. The query delegates nested relationship loading.
inline const SmartList<Order>& relations_graph(UserContext& user_context) {
    auto r = user_context.memory_resource();
    setup(user_context);
    auto service = user_context.entity_data_service<Executor>("Order");
    auto& repository = value(service).get();
    for (auto [id, name] : {std::pair{100u, "keyboard"}, std::pair{101u, "mouse"}}) {
        Product product(std::allocator_arg, Allocator(r));
        product.id = id;
        product.name = name;
        value(AuditedSaveExt<Product>::save(
                  std::move(product).audit_as(
                      "Seed the product before creating an order line that references it"),
                  user_context)
                  .wait());
    }
    auto order = make_order(user_context, 1, "graph-order");
    for (auto [id, name, product_id] :
         {std::tuple{10u, "first-line", 100u}, std::tuple{11u, "second-line", 101u}}) {
        OrderLine line(std::allocator_arg, Allocator(r));
        line.id = id;
        line.order_id = 1;
        line.name = name;
        line.product_id = product_id;
        order.lines.data.emplace_back(std::move(line));
    }
    order.lines.loaded();
    value(AuditedSaveExt<Order>::save(std::move(order).audit_as("Create the example order graph"),
                                      user_context)
              .wait());
    SelectQuery query(std::allocator_arg, Allocator(r), "Order");
    query.comment("Display the saved example order graph");
    SelectQuery lines(std::allocator_arg, Allocator(r), "OrderLine");
    lines.relation("product");
    query.project("name").relation_query("lines", lines).order_asc("id");
    PurposedSelectQuery purpose(
        std::allocator_arg, Allocator(r), query, "Display the saved example order");
    const auto& orders = value(repository.fetch_enhanced_entities<Order>(purpose).wait());
    require(orders.len() == 1 && orders[0].name == "graph-order", "one saved root");
    require(orders[0].lines.len() == 2, "two saved order lines");
    require(orders[0].lines[0].product && orders[0].lines[0].product->name == "keyboard",
            "first product");
    require(orders[0].lines[1].product && orders[0].lines[1].product->name == "mouse",
            "second product");
    return orders;
}

// Stable business observations for native/MIPS examples and Rust comparison;
// this is a fixture representation, not a new TEAQL entity wire protocol.
inline Json observations(Memory r, const SmartList<Order>& orders) {
    Json::Array roots(r);
    for (const auto& order : orders.data) {
        Json::Object fields(std::less<>{}, r);
        fields.emplace(String("id", r), Json(r, order.id));
        fields.emplace(String("version", r), Json(r, order.version()));
        fields.emplace(String("name", r), Json(r, order.name));
        fields.emplace(String("lines_loaded", r), Json(r, order.lines.is_loaded));
        Json::Array lines(r);
        for (const auto& line : order.lines.data) {
            Json::Object child(std::less<>{}, r);
            child.emplace(String("id", r), Json(r, line.id));
            child.emplace(String("order_id", r), Json(r, line.order_id));
            child.emplace(String("name", r), Json(r, line.name));
            child.emplace(String("product_id", r), Json(r, line.product_id));
            if (line.product) {
                Json::Object product(std::less<>{}, r);
                product.emplace(String("id", r), Json(r, line.product->id));
                product.emplace(String("name", r), Json(r, line.product->name));
                child.emplace(String("product", r), Json(r, product));
            } else {
                child.emplace(String("product", r), Json(r));
            }
            lines.push_back(Json(r, child));
        }
        fields.emplace(String("lines", r), Json(r, lines));
        roots.push_back(Json(r, fields));
    }
    return Json(r, roots);
}
}
