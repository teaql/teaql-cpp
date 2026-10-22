#include "memory_data_service.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
using namespace teaql;

static int complexity(GlobalContext& global) {
    struct Item {
        std::size_t key, id;
    };

    for (std::size_t size : {0, 1, 2, 17, 1024, 8193}) {
        for (unsigned pattern = 0; pattern < 3; ++pattern) {
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            Vector<Item> values(r);
            for (std::size_t i = 0; i < size; ++i) {
                values.push_back({pattern == 0   ? i
                                  : pattern == 1 ? size - i
                                                 : (i * 37 + 11) % 13,
                                  i});
            }
            std::size_t comparisons = 0;
            original_memory::stable_sort_in_context(values, [&](const Item& a, const Item& b) {
                ++comparisons;
                return a.key < b.key;
            });
            const auto levels = size < 2 ? 0 : std::bit_width(size - 1);
            if (values.size() != size || values.get_allocator().resource() != r ||
                comparisons > size * levels) {
                return 1;
            }
            Vector<bool> seen(r);
            seen.resize(size);
            for (std::size_t i = 0; i < size; ++i) {
                const auto& item = values[i];
                if (item.id >= size || seen[item.id]) {
                    return 2;
                }
                seen[item.id] = true;
                if (i && (values[i - 1].key > item.key ||
                          (values[i - 1].key == item.key && values[i - 1].id > item.id))) {
                    return 3;
                }
            }
            std::cout << "size=" << size << " pattern=" << pattern << " comparisons=" << comparisons
                      << " bound=" << size * levels << '\n';
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    if (argc == 2 && std::string_view(argv[1]) == "--complexity-test") {
        return complexity(global);
    }
    for (std::size_t size : {0, 1, 2, 17, 257}) {
        for (unsigned pattern = 0; pattern < 3; ++pattern) {
            for (unsigned mode = 0; mode < 4; ++mode) {
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto& metadata = user_context->make<InMemoryMetadataStore>();
                metadata.register_entity(
                    EntityDescriptor(std::allocator_arg, Allocator(r), "Item"));
                auto& service = user_context->make<original_memory::MemoryDataService>(metadata);
                Vector<CompactRow> input(r);
                for (std::size_t i = 0; i < size; ++i) {
                    auto key = pattern == 0 ? i : pattern == 1 ? size - i : (i * 37 + 11) % 13;
                    Record fields(r);
                    fields.emplace("id", Value(r, std::uint64_t(i)));
                    fields.emplace("key", Value(r, std::int64_t(key)));
                    fields.emplace("secondary", Value(r, std::int64_t(i % 3)));
                    auto number = debug_string(r, key);
                    String name("item_", r);
                    name.append(4 - number.size(), '0');
                    name.append(number);
                    fields.emplace("name", Value(r, std::move(name)));
                    input.emplace_back(std::move(fields));
                }
                service.seed("Item", input);
                SelectQuery query(std::allocator_arg, Allocator(r), "Item");
                if (mode == 0) {
                    query.order_asc("key");
                } else if (mode == 1) {
                    query.order_desc("key");
                } else if (mode == 2) {
                    query.order_asc("key").order_desc("secondary");
                } else {
                    query.order_expr_asc(Expr::gbk(Expr::column(r, "name")));
                }
                auto result = service.fetch_all(query);
                if (!result) {
                    std::cerr << result.error().to_string(r) << '\n';
                    return 1;
                }
                Json::Array rows(r);
                for (const auto& row : result.value()) {
                    auto fields = detail::model_json(r);
                    for (const auto& [key, value] : row.into_map(r)) {
                        detail::model_put(fields, key, detail::debug_value(r, value));
                    }
                    rows.emplace_back(std::move(fields));
                }
                auto out = detail::model_json(r);
                detail::model_put(out,
                                  "case",
                                  String("sort_", r) + debug_string(r, size) + "_" +
                                      debug_string(r, pattern) + "_" + debug_string(r, mode));
                detail::model_put(out, "rows", Json(r, std::move(rows)));
                std::cout << out.dump(r) << '\n';
            }
        }
    }
}
