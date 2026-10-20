#include <teaql/core/eval.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <array>
#include <iostream>

using namespace teaql::eval;

static const FieldLayout& layout(GlobalContext& global, std::string_view entity) {
    auto r = global.memory_resource();
    Vector<String> names(r), aliases(r);
    for (std::size_t i = 0; i < 130; ++i) {
        names.emplace_back(i == 0 ? "id" : i == 1 ? "version" : "field_" + std::to_string(i));
        aliases.emplace_back("sql_" + std::to_string(i));
    }
    Vector<FieldLayout::Index> indexes(r);
    Vector<FieldLayout::Mapping> mappings(r);
    Vector<std::string_view> expected(r);
    for (std::size_t i = 0; i < names.size(); ++i) {
        indexes.emplace_back(names[i], i);
        mappings.emplace_back(names[i], names[i], aliases[i]);
        expected.push_back(names[i]);
    }
    const std::array<std::string_view, 1> relations{"lines"};
    return global.make<FieldLayout>(
        std::move(FieldLayout::from_generated(
                      r, entity, "eval-v1", indexes, mappings, relations, expected))
            .value());
}

static Json diagnostics(Memory r, const LoadState& state) {
    auto out = detail::model_json(r);
    Json::Array actual(r), expected(r);
    const DebugOptions options[] = {
        {}, {true}, {false, DebugNumbers::LowerHex}, {true, DebugNumbers::UpperHex}};
    bool complete = true;
    for (const auto& option : options) {
        String text(r);
        DebugWriter writer(text, option);
        teaql::eval::write_debug(writer, state);
        actual.emplace_back(text);
        if (const auto* snapshot = state.indexed_snapshot()) {
            String nested(r);
            DebugWriter wrapper(nested, option);
            wrapper.tuple("Indexed", [&](auto& inner) {
                teaql::write_debug(inner, snapshot->get());
            });
            complete = complete && text == nested;
            expected.emplace_back(nested);
        }
    }
    detail::model_put(out, "actual", std::move(actual));
    detail::model_put(out, "expected_snapshot_wrapper", std::move(expected));
    detail::model_put(out, "complete", complete);
    return out;
}

static Json summary(Memory r, const LoadState& state) {
    auto out = detail::model_json(r);
    detail::model_put(out, "kind", state.variant_name());
    Json::Array loaded(r);
    for (auto name : {"id",
                      "version",
                      "field_2",
                      "sql_2",
                      "field_64",
                      "field_65",
                      "field_129",
                      "lines",
                      "#memo",
                      "unknown"}) {
        loaded.emplace_back(state.is_loaded(name));
    }
    detail::model_put(out, "loaded", std::move(loaded));
    detail::model_put(out, "clone_equal", state == detail::clone_in(r, state));
    bool wire_rejected = false;
    if (state.kind() == LoadState::Kind::Indexed) {
        try {
            (void)state.to_json(r);
        } catch (const std::logic_error& error) {
            wire_rejected = std::string_view(error.what()) ==
                            "the enum variant LoadState::Indexed cannot be serialized";
        }
    }
    detail::model_put(out, "indexed_wire_rejected", wire_rejected);
    return out;
}

static Json observe(GlobalContext& global, UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const auto& metadata = layout(global, "EvalRow");
    const auto& foreign = layout(global, "OtherEvalRow");
    auto& pool = user_context.make<SnapshotPool>();
    Vector<std::string_view> fields(r);
    for (const auto& name : std::get<Json::Array>(field(input, "fields").data)) {
        fields.push_back(text(name));
    }
    auto kind = text(field(input, "kind"));
    auto state = LoadState::not_loaded(r);
    if (kind == "FullyLoaded") {
        state = LoadState::fully_loaded(r);
    } else if (kind == "Partial" || kind == "Indexed") {
        state = LoadState::partial(r, fields);
        if (kind == "Indexed") {
            state = std::move(state.into_indexed(pool, metadata)).value();
        }
    } else if (kind == "PartialCompact") {
        state = LoadState::partial_compact(r, fields);
    } else if (kind == "SharedColumns") {
        Vector<String> names(r);
        for (auto name : fields) {
            names.emplace_back(name);
        }
        auto& columns = user_context.make<ColumnLayout>(std::move(names));
        state = LoadState::shared_columns(r, columns);
    }
    auto out = detail::model_json(r);
    auto common = detail::model_json(r);
    detail::model_put(common, "initial", summary(r, state));
    auto initial_diagnostics = diagnostics(r, state);
    detail::model_put(common, "initial_wrapper_complete", field(initial_diagnostics, "complete"));
    auto original = detail::clone_in(r, state);
    Json::Array steps(r);
    for (const auto& action : std::get<Json::Array>(field(input, "actions").data)) {
        auto name = text(field(action, "field"));
        auto result = text(field(action, "action")) == "load" ? state.mark_loaded(name, &pool)
                                                              : state.mark_unloaded(name, &pool);
        auto step = detail::model_json(r);
        detail::model_put(step, "error", result ? Json(r) : Json(r, result.error()));
        detail::model_put(step, "state", summary(r, state));
        steps.emplace_back(std::move(step));
    }
    detail::model_put(common, "steps", std::move(steps));
    detail::model_put(common, "original", summary(r, original));
    detail::model_put(
        common, "original_unchanged", summary(r, original) == field(common, "initial"));
    auto converted = state.into_indexed(pool, metadata);
    auto other = state.into_indexed(pool, foreign);
    detail::model_put(common, "converted", summary(r, converted.value()));
    detail::model_put(
        common, "foreign", other ? summary(r, other.value()) : Json(r, other.error()));
    auto final_diagnostics = diagnostics(r, converted.value());
    detail::model_put(common, "final_wrapper_complete", field(final_diagnostics, "complete"));
    detail::model_put(out, "common", std::move(common));
    detail::model_put(out, "initial_diagnostics", std::move(initial_diagnostics));
    detail::model_put(out, "final_diagnostics", std::move(final_diagnostics));
    return out;
}

static void lifetime() {
    GlobalContext global;
    const auto& metadata = layout(global, "EvalRow");
    Ref<LoadState> retained;
    std::optional<LoadState> expired;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& pool = user_context->make<SnapshotPool>();
        const std::array<std::string_view, 4> fields{"id", "field_129", "lines", "#memo"};
        auto state = LoadState::partial(r, fields).into_indexed(pool, metadata);
        expired.emplace(detail::clone_in(r, state.value()));
        retained = user_context->make_global<LoadState>(state.value());
    }
    const auto reclaimed_bytes = global.live_user_bytes();
    auto next_user_context = global.make_user_context();
    bool rejected = false;
    try {
        (void)diagnostics(next_user_context->memory_resource(), *expired);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    if (!rejected || reclaimed_bytes != 0 || !copy.is_loaded("field_129") ||
        !std::get<bool>(
            field(diagnostics(next_user_context->memory_resource(), copy), "complete").data)) {
        throw std::runtime_error(
            "Indexed diagnostics must validate expired borrows and explicit Global retention");
    }
    std::cout << "Reclaimed source User bytes: " << reclaimed_bytes << '\n';
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            lifetime();
        } else {
            std::string line;
            while (std::getline(std::cin, line)) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto input = Json::parse(r, line);
                std::cout << observe(global, *user_context, *input).dump(r) << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
