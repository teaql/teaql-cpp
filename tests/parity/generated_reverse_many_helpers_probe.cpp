#include "../generated_relations/include/cpp_relations.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_relations;

static bool boolean(const Json& input, std::string_view name) {
    return std::get<bool>(field(input, name).data);
}

static Record row(Memory r, std::uint64_t id, std::optional<std::string_view> name) {
    Record fields(r);
    fields.emplace(String("id", r), Value(r, id));
    fields.emplace(String("version", r), Value(r, std::int64_t(3)));
    if (name) {
        fields.emplace(String("name", r), Value(r, *name));
    }
    return fields;
}

template <class T>
static Json evaluation(Memory r, const eval::EvalResult<T>& result, auto&& encode) {
    Json::Object output(r);
    if (result.is_value()) {
        output.emplace(String("state", r), Json(r, "Value"));
        output.emplace(String("value", r), encode(result.get()));
    } else if (result.is_null()) {
        output.emplace(String("state", r), Json(r, "Null"));
    } else {
        output.emplace(String("state", r), Json(r, "NotLoaded"));
        output.emplace(String("node", r), Json(r, result.missing().failed_node));
        output.emplace(String("path", r), Json(r, result.missing().attempted_path));
    }
    return Json(r, output);
}

template <class T>
static Json list(Memory r, const SmartList<T>& values) {
    Json::Object output(r);
    Json::Array items(r);
    for (const auto& entity : values.data) {
        Json::Object item(r);
        item.emplace(String("id", r), Json(r, entity.id()));
        item.emplace(String("name", r), Json(r, entity.name()));
        item.emplace(String("eval", r), evaluation(r, entity.evalName(), [&](const auto& value) {
                         return Json(r, value);
                     }));
        items.emplace_back(item);
    }
    output.emplace(String("loaded", r), Json(r, values.is_loaded));
    output.emplace(String("items", r), Json(r, items));
    return Json(r, output);
}

template <class T>
static Json summary(Memory r, const RelationHandle<SmartList<T>>& handle, const auto& result) {
    Json::Object output(r);
    output.emplace(String("handle", r),
                   Json(r,
                        handle.is_empty()    ? "Empty"
                        : handle.is_loaded() ? "Loaded"
                                             : "NotLoaded"));
    output.emplace(String("loaded", r), Json(r, handle.is_loaded()));
    output.emplace(String("empty", r), Json(r, handle.is_empty()));
    output.emplace(String("value", r), handle.value() ? list(r, *handle.value()) : Json(r));
    output.emplace(String("eval", r), evaluation(r, result, [&](const auto& borrow) {
                       return list(r, borrow.get());
                   }));
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& owner = user_context.make<Platform>(
        Platform::from_compact_row(r,
                                   CompactRow(std::allocator_arg, Allocator(r), row(r, 9, "owner")))
            .value());
    Vector<std::string_view> names(r);
    for (auto name : {"id", "version", "name"}) {
        names.emplace_back(name);
    }
    if (boolean(input, "selected")) {
        names.emplace_back("school_list");
        names.emplace_back("device_list");
    }
    owner.load_state = eval::LoadState::partial(r, names)
                           .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                                         Platform::field_layout(r))
                           .value();
    auto& graph = user_context.make<EntityGraphBuilder>();
    auto stored_owner = text(field(input, "key")) == "owner" ? "School" : "Platform";
    std::uint64_t stored_id = text(field(input, "key")) == "id" ? 10 : 9;
    auto relation = text(field(input, "field"));
    if (text(field(input, "edge")) == "wrong") {
        SmartList<std::uint64_t> wrong(std::allocator_arg, Allocator(r));
        wrong.data.emplace_back(42);
        graph.install_relation_list(stored_owner, stored_id, relation, std::move(wrong));
    } else if (text(field(input, "edge")) == "list") {
        auto install = [&]<class T>() {
            SmartList<T> values(std::allocator_arg, Allocator(r));
            auto size = std::get<std::uint64_t>(field(input, "size").data);
            for (std::uint64_t index = 0; index < size; ++index) {
                auto name = boolean(input, "child_loaded")
                                ? std::optional<std::string_view>(text(field(input, "name")))
                                : std::nullopt;
                values.data.emplace_back(
                    T::from_compact_row(
                        r, CompactRow(std::allocator_arg, Allocator(r), row(r, 11 + index, name)))
                        .value());
            }
            values.is_loaded = boolean(input, "list_loaded");
            graph.install_relation_list(stored_owner, stored_id, relation, std::move(values));
        };
        if (relation == "school_list") {
            install.template operator()<School>();
        } else {
            install.template operator()<Device>();
        }
    }
    owner.root.freeze_graph(std::move(graph));
    try {
        if (relation == "school_list") {
            return summary(r, owner.schoolListHandle(), owner.evalSchoolList());
        }
        return summary(r, owner.deviceListHandle(), owner.evalDeviceList());
    } catch (const std::exception& error) {
        Json::Object result(r);
        result.emplace(String("error", r), Json(r, error.what()));
        return Json(r, result);
    }
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            Json retained(global.memory_resource());
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid reverse-many fixture");
                }
                auto result = observe(*user_context, *input);
                retained = detail::clone_in(global.memory_resource(), result);
            }
            user_context.reset();
            std::cout << retained.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
