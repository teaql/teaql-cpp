#include "../generated_inheritance_variants/include/cpp_inheritance_variants.hpp"
#include "fixture_value.hpp"
#include <iostream>

using namespace cpp_inheritance_variants;

static Json changed(Memory r, const std::optional<Value>& value) {
    Json::Object result(r);
    result.emplace(String("present", r), Json(r, bool(value)));
    result.emplace(String("value", r), value ? value->to_json_value(r) : Json(r));
    return Json(r, result);
}

static Json evaluated(Memory r, const auto& result) {
    Json::Object out(r);
    if (result.is_value()) {
        out.emplace(String("state", r), Json(r, "Value"));
        out.emplace(String("value", r), Value(r, result.get()).to_json_value(r));
    } else if (result.is_null()) {
        out.emplace(String("state", r), Json(r, "Null"));
    } else {
        out.emplace(String("state", r), Json(r, "NotLoaded"));
        out.emplace(String("node", r), Json(r, result.missing().failed_node));
        out.emplace(String("path", r), Json(r, result.missing().attempted_path));
    }
    return Json(r, out);
}

template <class Model, class Getter, class Eval, class Changed, class Setter>
static Json run(UserContext& user_context,
                const Json& input,
                Getter getter,
                Eval evaluation,
                Changed mutation,
                Setter setter) {
    auto r = user_context.memory_resource();
    auto& root = user_context.make<EntityRuntimeState>();
    auto& entity = [&]() -> Model& {
        if (text(field(input, "initial")) == "row") {
            Record row(r);
            row.emplace(String("id", r), Value(r, std::uint64_t(7)));
            row.emplace(String("name", r), Value(r, "stored"));
            row.emplace(String("version", r), Value(r, std::int64_t(3)));
            row.emplace(String("active", r), Value(r, true));
            row.emplace(String("note", r), Value(r, "note"));
            row.emplace(String("tenant_id", r), Value(r, std::uint64_t(42)));
            row.emplace(String("created_at", r), Value(r, Timestamp(111)));
            row.emplace(String("updated_at", r), Value(r, Timestamp(222)));
            row.emplace(String("amount", r), Value(r, *Decimal::parse("12.50")));
            row.emplace(String("status", r), Value(r, "open"));
            row.emplace(String("reference", r), Value(r, "ref"));
            return user_context.make<Model>(
                Model::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row))
                    .value());
        }
        return user_context.make<Model>();
    }();
    entity.attachRuntimeStateRecursive(root);
    auto view = [&](const Model& model) {
        Json::Object out(r);
        out.emplace(String("id", r), Json(r, model.id()));
        out.emplace(String("version", r), Json(r, model.version()));
        out.emplace(String("name", r), Json(r, model.name()));
        out.emplace(String("active", r), Json(r, model.active()));
        out.emplace(String("note", r), model.note() ? Json(r, *model.note()) : Json(r));
        out.emplace(String("created", r), Json(r, model.createdAt().as_millis()));
        out.emplace(String("updated", r), Json(r, model.updatedAt().as_millis()));
        out.emplace(String("tenant", r), Json(r, model.tenantId()));
        out.emplace(String("getter", r), Value(r, getter(model)).to_json_value(r));
        out.emplace(String("eval", r), evaluated(r, evaluation(model)));
        out.emplace(String("changed", r), changed(r, mutation(model)));
        out.emplace(String("loaded", r), Json(r, model.isLoaded(text(field(input, "field")))));
        auto original = model.original_values();
        const Value* old = nullptr;
        if (original) {
            auto found = original->fields.find(text(field(input, "field")));
            if (found != original->fields.end()) {
                old = &found->second;
            }
        }
        out.emplace(String("original", r), old ? old->to_json_value(r) : Json(r));
        return Json(r, out);
    };
    auto before = view(entity);
    auto& copy = user_context.make<Model>(entity);
    auto operand = value(r, field(input, "value"));
    auto op = text(field(input, "op"));
    if (op == "update") {
        setter(entity, operand);
    } else if (op == "overlay") {
        root.set(entity.entity_key(), text(field(input, "field")), operand);
    } else if (op == "self") {
        setter(entity, Value(r, getter(entity)));
    }
    Json::Object out(r);
    out.emplace(String("wall_millis", r), Json(r, Timestamp::now().as_millis()));
    out.emplace(String("before", r), before);
    out.emplace(String("after", r), view(entity));
    out.emplace(String("copy", r), view(copy));
    Json::Array pending(r);
    for (const auto& name : root.changed_field_names(entity.entity_key())) {
        pending.emplace_back(name);
    }
    out.emplace(String("pending", r), Json(r, pending));
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
#define RUN(MODEL, GETTER, EVAL, CHANGED, UPDATE, CONVERT)                                         \
    return run<MODEL>(                                                                             \
        user_context,                                                                              \
        input,                                                                                     \
        [](const auto& m) {                                                                        \
            return m.GETTER();                                                                     \
        },                                                                                         \
        [](const auto& m) {                                                                        \
            return m.EVAL();                                                                       \
        },                                                                                         \
        [](const auto& m) {                                                                        \
            return m.CHANGED();                                                                    \
        },                                                                                         \
        [](auto& m, const Value& v) {                                                              \
            m.UPDATE(CONVERT);                                                                     \
        })
    auto model = text(field(input, "model"));
    auto field_name = text(field(input, "field"));
    if (model == "invoice") {
        RUN(Invoice, amount, evalAmount, changedAmount, updateAmount, *v.try_decimal());
    }
    if (model == "special_invoice" && field_name == "amount") {
        RUN(SpecialInvoice, amount, evalAmount, changedAmount, updateAmount, *v.try_decimal());
    }
    if (model == "special_invoice") {
        RUN(SpecialInvoice,
            reference,
            evalReference,
            changedReference,
            updateReference,
            *v.try_text());
    }
    if (model == "receipt") {
        RUN(Receipt, status, evalStatus, changedStatus, updateStatus, *v.try_text());
    }
    if (model == "intermediate_record") {
        RUN(IntermediateRecord, name, evalName, changedName, updateName, *v.try_text());
    }
    RUN(Document, name, evalName, changedName, updateName, *v.try_text());
#undef RUN
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
                    throw std::runtime_error("invalid constructor-helper fixture");
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
