#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static Json observe_layout(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Vector<String> names(r);
    Vector<Value> values(r);
    for (const auto& name : std::get<Json::Array>(field(input, "names").data)) {
        names.emplace_back(text(name));
        values.emplace_back(nullptr);
    }
    auto& initial = user_context.make<CompactRowLayout>(std::move(names));
    auto& row = user_context.make<CompactRow>(initial, std::move(values));
    auto& cache = user_context.make<RelationShapeCache>();
    for (const auto& relation : std::get<Json::Array>(field(input, "relations").data)) {
        row.mark_relation_loaded(text(relation), cache);
    }
    const auto* base = row.shared_layout();
    const auto* current = base;
    Json::Array stages(r);
    for (const auto& schema : std::get<Json::Array>(field(input, "schemas").data)) {
        Vector<DynamicPropertyDefinitions::Field> fields(r);
        for (const auto& entry : std::get<Json::Array>(schema.data)) {
            const auto& pair = std::get<Json::Array>(entry.data);
            fields.emplace_back(text(pair[0]),
                                text(pair[1]) == "text" ? DataType::Text : DataType::I64);
        }
        auto result = DynamicPropertyDefinitions::from_fields(r, fields);
        if (!result) {
            throw std::runtime_error(std::string(result.error()));
        }
        auto& definitions =
            user_context.make<DynamicPropertyDefinitions>(std::move(result.value()));
        const auto& next = current->with_dynamic_property_definitions(definitions);
        Json::Object stage(r);
        stage.emplace(String("reused", r), Json(r, &next == current));
        stage.emplace(String("equals_base", r), Json(r, next == *base));
        stage.emplace(String("idempotent", r),
                      Json(r, &next.with_dynamic_property_definitions(definitions) == &next));
        stage.emplace(String("equal_copy", r),
                      Json(r, next == user_context.make<CompactRowLayout>(next)));
        Json::Array columns(r);
        for (const auto& name : next.names()) {
            columns.emplace_back(name);
        }
        stage.emplace(String("names", r), Json(r, columns));
        Vector<Value> nulls(r);
        for (std::size_t index = 0; index < next.names().size(); ++index) {
            nulls.emplace_back(nullptr);
        }
        auto& next_row = user_context.make<CompactRow>(next, std::move(nulls));
        Json::Array relations(r);
        for (const auto& name : std::get<Json::Array>(field(input, "relations").data)) {
            relations.emplace_back(next_row.is_loaded_relation(text(name)));
        }
        stage.emplace(String("relations", r), Json(r, relations));
        stages.emplace_back(stage);
        current = &next;
    }
    return Json(r, stages);
}

class PresentationFixture : public EntityDefaults<PresentationFixture> {
public:
    using allocator_type = Allocator;

    PresentationFixture(std::allocator_arg_t,
                        Allocator a,
                        bool borrowed,
                        const Json& payload,
                        std::size_t& consumed)
        : resource_(a.resource()), borrowed_(borrowed), payload_(a.resource(), payload),
          consumed_(consumed) {}

    Memory memory_resource() const {
        return resource_;
    }

    std::optional<Json> borrowed_json(Memory r, EntityJsonTraversal& traversal) const {
        if (!borrowed_) {
            return std::nullopt;
        }
        const bool expand = traversal.enter("Fixture", reinterpret_cast<std::uintptr_t>(this));
        traversal.leave();
        if (!expand) {
            throw std::runtime_error("fresh JSON traversal unexpectedly rejected the root");
        }
        return Json(r, payload_);
    }

    MutationValues into_values() && {
        ++consumed_;
        MutationValues out(resource_);
        out.fields.emplace(String("id", resource_), Value(resource_, std::uint64_t{7}));
        return out;
    }

    Record into_json_values() && {
        consumed_ += 10;
        Record out(resource_);
        out.emplace(String("id", resource_), Value(resource_, std::uint64_t{7}));
        out.emplace(String("_is_new", resource_), Value(resource_, true));
        out.emplace(String("_readonly", resource_), Value(resource_, false));
        return out;
    }

private:
    Memory resource_;
    bool borrowed_;
    Json payload_;
    std::size_t& consumed_;
};

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto kind = text(field(input, "kind"));
    if (kind == "layout") {
        return observe_layout(user_context, input);
    }
    if (kind == "presentation") {
        std::size_t consumed = 0;
        auto& model = user_context.make<PresentationFixture>(
            std::get<bool>(field(input, "borrowed").data), field(input, "payload"), consumed);
        auto rendered = std::move(model).into_json(r);
        Json::Object out(r);
        out.emplace(String("json", r), std::move(rendered));
        out.emplace(String("consumed", r), Json(r, consumed));
        return Json(r, out);
    }
    auto& traversal = user_context.make<EntityJsonTraversal>();
    Json::Array output(r);
    for (const auto& operation : std::get<Json::Array>(field(input, "operations").data)) {
        const auto& parts = std::get<Json::Array>(operation.data);
        if (text(parts[0]) == "leave") {
            traversal.leave();
            output.emplace_back();
        } else {
            output.emplace_back(
                traversal.enter(text(parts[1]), std::get<std::uint64_t>(parts[2].data)));
        }
    }
    return Json(r, output);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid shape/presentation fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
