#pragma once
#include <teaql/runtime/entity_query.hpp>
#include <teaql/memory/object_borrow.hpp>
#include <limits>

namespace teaql {
namespace detail {
struct GeneratedDynamicJsonDiagnostic {
    using allocator_type = Allocator;
    String text;

    GeneratedDynamicJsonDiagnostic(std::allocator_arg_t, Allocator a, std::string_view input)
        : text("Input JSON format error: ", a.resource()) {
        text += input;
    }

    GeneratedDynamicJsonDiagnostic(std::allocator_arg_t,
                                   Allocator a,
                                   const GeneratedDynamicJsonDiagnostic& other)
        : text(other.text, a.resource()) {}

    Memory memory_resource() const {
        return text.get_allocator().resource();
    }
};
}

// Thrown values borrow their diagnostic Context; they never keep it alive.
class GeneratedDynamicJsonFormatError final : public std::exception {
public:
    using allocator_type = Allocator;

    GeneratedDynamicJsonFormatError(Memory r, std::string_view input)
        : diagnostic_(
              std::allocator_arg,
              Allocator(r),
              detail::resource_arena(r).make<detail::GeneratedDynamicJsonDiagnostic>(input)) {}

    GeneratedDynamicJsonFormatError(std::allocator_arg_t,
                                    Allocator a,
                                    const GeneratedDynamicJsonFormatError& other)
        : diagnostic_(std::allocator_arg,
                      a,
                      detail::resource_arena(a.resource())
                          .make<detail::GeneratedDynamicJsonDiagnostic>(other.diagnostic_.get())) {}

    const char* what() const noexcept override {
        return "Input JSON format error";
    }

    String to_string(Memory destination) const {
        return String(diagnostic_.get().text, destination);
    }

private:
    ArenaBorrow<detail::GeneratedDynamicJsonDiagnostic> diagnostic_;
};

template <class Recipe, class Query>
void apply_generated_dynamic_json_field(Query& query, std::string_view field, const Json& value) {
    auto separator = field.find('.');
    if (separator != std::string_view::npos) {
        Recipe::apply_chain(query, field.substr(0, separator), field.substr(separator + 1), value);
    } else if (auto storage = Recipe::self_field(field)) {
        query.andFilter(
            request::dynamic_json_filter_expr(query.memory_resource(), *storage, value));
    }
}

template <class ChildRecipe, class Query>
void apply_generated_dynamic_json_relation(Query& parent,
                                           std::string_view local,
                                           std::string_view foreign,
                                           std::string_view tail,
                                           const Json& value,
                                           std::string_view relation = {}) {
    using Model = typename ChildRecipe::Model;
    auto child = parent.template dynamicChildQuery<Model>();
    apply_generated_dynamic_json_field<ChildRecipe>(child, tail, value);
    parent.filterRelation(local, foreign, child, false, relation);
}

template <class Recipe, class Query>
void apply_generated_dynamic_json(Query& query, std::string_view source) {
    auto r = query.memory_resource();
    auto json = Json::parse(r, source);
    if (!json) {
        throw GeneratedDynamicJsonFormatError(r, source);
    }
    auto* object = std::get_if<Json::Object>(&json->data);
    if (!object) {
        return;
    }
    for (const auto& [field, value] : *object) {
        if (!field.starts_with('_')) {
            apply_generated_dynamic_json_field<Recipe>(query, field, value);
        }
    }
    auto single_order = [&](const Json::Object& value) {
        auto field = value.find("field");
        if (field == value.end()) {
            return;
        }
        auto* name = std::get_if<String>(&field->second.data);
        if (!name) {
            return;
        }
        auto storage = Recipe::self_field(*name);
        if (!storage) {
            return;
        }
        auto ascending = value.find("useAsc");
        auto* flag =
            ascending == value.end() ? nullptr : std::get_if<bool>(&ascending->second.data);
        if (flag && *flag) {
            query.orderAsc(*storage);
        } else {
            query.orderDesc(*storage);
        }
    };
    if (auto found = object->find("_orderBy"); found != object->end()) {
        const auto& value = found->second;
        if (auto* name = std::get_if<String>(&value.data)) {
            if (auto storage = Recipe::self_field(*name)) {
                query.orderDesc(*storage);
            }
        } else if (auto* item = std::get_if<Json::Object>(&value.data)) {
            single_order(*item);
        } else if (auto* items = std::get_if<Json::Array>(&value.data)) {
            for (const auto& item : *items) {
                if (auto* fields = std::get_if<Json::Object>(&item.data)) {
                    single_order(*fields);
                }
            }
        }
    }
    if (auto offset = request::dynamic_json_u64_field(*object, "_start")) {
        query.offset(*offset);
    }
    if (auto limit = request::dynamic_json_u64_field(*object, "_size")) {
        query.limit(*limit);
    }
    if (auto limit = request::dynamic_json_u64_field(*object, "_pageSize")) {
        query.limit(*limit);
    }
    if (auto page = request::dynamic_json_u64_field(*object, "_page"); page && *page > 0) {
        auto size = request::dynamic_json_u64_field(*object, "_pageSize");
        if (!size && query.query().slice) {
            size = query.query().slice->limit;
        }
        auto limit = size.value_or(10);
        auto pages = *page - 1;
        auto offset = limit && pages > std::numeric_limits<std::uint64_t>::max() / limit
                          ? std::numeric_limits<std::uint64_t>::max()
                          : pages * limit;
        query.mutate_selection([&](SelectQuery& selection) {
            selection.page(offset, limit);
        });
    }
}
}
