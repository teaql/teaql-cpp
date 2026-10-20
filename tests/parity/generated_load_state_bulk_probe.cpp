#include "../generated_typed_member_aliases/include/cpp_member_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <array>
#include <iostream>
#include <thread>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static std::string payload(std::size_t index) {
    return "row-" + std::to_string(index) + "-" + std::string(64, 'x');
}

static const ColumnLayout& shape(UserContext& user_context) {
    return user_context.make<ColumnLayout>(
        std::initializer_list<std::string_view>{"id", "version", "type_field"});
}

template <Entity T>
static T decode(UserContext& user_context,
                const ColumnLayout& columns,
                std::uint64_t id,
                const std::optional<std::string>& text_value) {
    auto r = user_context.memory_resource();
    Vector<Value> values(r);
    values.emplace_back(id);
    values.emplace_back(std::int64_t{1});
    if (text_value) {
        values.emplace_back(*text_value);
    } else {
        values.emplace_back(nullptr);
    }
    auto result = T::from_compact_row(
        r, CompactRow(std::allocator_arg, Allocator(r), columns, std::move(values)));
    check(result.is_ok(), "Actual generated typed decoder failed");
    return std::move(result).value();
}

template <Entity T>
static Json bulk(UserContext& user_context, std::size_t count, bool independent) {
    check(count == 1 || count == 100 || count == 10000, "Unexpected source batch size");
    auto r = user_context.memory_resource();
    const auto& columns = shape(user_context);
    Vector<T> rows(r);
    rows.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        rows.emplace_back(
            decode<T>(user_context,
                      independent ? shape(user_context) : columns,
                      index + 1,
                      index == 0 ? std::nullopt : std::optional<std::string>(payload(index))));
    }
    const auto first = rows.front().loaded_state_snapshot();
    check(first.has_value(), "Generated typed row did not install indexed state");
    bool shared = true, no_overflow = true, no_names = true, private_values = true;
    bool loaded = true, omitted = true, clean = true, layout_shared = true;
    Json::Array values(r);
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& row = rows[index];
        const auto snapshot = row.loaded_state_snapshot();
        check(snapshot.has_value(), "A typed result lost its snapshot");
        shared = shared && snapshot->identity() == first->identity();
        layout_shared = layout_shared && &snapshot->get().layout() == &T::field_layout(r);
        no_overflow = no_overflow && snapshot->get().overflow() == nullptr;
        no_names = no_names && snapshot->get().selected_names() == nullptr;
        loaded = loaded && row.is_field_loaded("type") && row.is_field_loaded("type_field") &&
                 row.is_field_loaded("legacy_type");
        omitted = omitted && !row.is_field_loaded("name") && !row.is_field_loaded("platform_id");
        clean = clean && !row.dirty_fields() && !row.is_new();
        private_values = private_values && row.id() == index + 1 &&
                         (index == 0 ? !row.type()
                                     : row.type() && std::string_view(*row.type()) ==
                                                         std::string_view(payload(index)));
        auto value = detail::model_json(r);
        detail::model_put(value, "id", row.id());
        detail::model_put(value, "value", row.type() ? Json(r, *row.type()) : Json(r));
        values.emplace_back(std::move(value));
    }
    check(shared && layout_shared && no_overflow && no_names && loaded && omitted && clean &&
              private_values,
          "Typed batches must share geometry without sharing values, presence or defaults");
    std::optional<String> changed(std::in_place, "changed-last-row-payload", r);
    rows.back().updateType(changed);
    bool siblings_unchanged = true;
    for (std::size_t index = 0; index + 1 < rows.size(); ++index) {
        siblings_unchanged =
            siblings_unchanged &&
            (index == 0 ? !rows[index].type()
                        : rows[index].type() && std::string_view(*rows[index].type()) ==
                                                    std::string_view(payload(index)));
    }
    const bool value_keeps_snapshot =
        rows.back().loaded_state_snapshot()->identity() == first->identity();
    const bool updated_value = rows.back().type() && *rows.back().type() == *changed;
    FieldSet narrow(r);
    narrow.emplace("id");
    rows.back().set_checker_loaded_fields(std::move(narrow));
    const bool detached = rows.back().loaded_state_snapshot()->identity() != first->identity();
    const bool narrowed = rows.back().is_field_loaded("id") &&
                          !rows.back().is_field_loaded("version") &&
                          !rows.back().is_field_loaded("type_field");
    const bool sibling_projection =
        count == 1 || (rows.front().loaded_state_snapshot()->identity() == first->identity() &&
                       rows.front().is_field_loaded("type_field"));
    check(siblings_unchanged && value_keeps_snapshot && updated_value && detached && narrowed &&
              sibling_projection,
          "Value mutation and Checker narrowing must preserve independent sibling views");
    auto out = detail::model_json(r);
    detail::model_put(out, "rows", values);
    detail::model_put(out, "shared_snapshot", shared);
    detail::model_put(out, "shared_layout", layout_shared);
    detail::model_put(out, "overflow_absent", no_overflow);
    detail::model_put(out, "selected_names_absent", no_names);
    detail::model_put(out, "all_aliases_loaded", loaded);
    detail::model_put(out, "omitted_unloaded", omitted);
    detail::model_put(out, "clean", clean);
    detail::model_put(out, "private_values", private_values);
    detail::model_put(out, "siblings_unchanged", siblings_unchanged);
    detail::model_put(out, "value_keeps_snapshot", value_keeps_snapshot);
    detail::model_put(out, "updated_value", updated_value);
    detail::model_put(out, "checker_detaches", detached);
    detail::model_put(out, "checker_narrows", narrowed);
    detail::model_put(out, "sibling_projection", sibling_projection);
    return out;
}

template <Entity T>
static void cold_contexts_and_lifetimes() {
    GlobalContext global;
    Ref<T> retained;
    std::optional<EntityBorrow<T>> expired;
    const FieldLayout* layout = nullptr;
    std::uint64_t old_snapshot = 0;
    const char* original_text = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        layout = &T::field_layout(r);
        Vector<T> rows(r);
        rows.reserve(16);
        for (std::size_t index = 0; index < 16; ++index) {
            rows.emplace_back(
                decode<T>(*user_context, shape(*user_context), index + 1, std::nullopt));
            check(&T::field_layout(r) == layout && rows.back().id() == index + 1 &&
                      rows.back().is_field_loaded("type_field") && !rows.back().type() &&
                      !rows.back().is_field_loaded("name") && !rows.back().dirty_fields(),
                  "Cold independent shapes reuse installed metadata without sharing row values");
            check(rows.front().loaded_state_snapshot()->identity() ==
                      rows.back().loaded_state_snapshot()->identity(),
                  "Independent generated decoders must reuse one scoped snapshot");
        }
        old_snapshot = rows.front().loaded_state_snapshot()->identity();
        bool foreign_rejected = false;
        std::thread worker([&] {
            try {
                (void)T::field_layout(r);
            } catch (const std::logic_error& error) {
                foreign_rejected =
                    std::string_view(error.what()) == "Context accessed from a non-owner thread";
            }
        });
        worker.join();
        check(foreign_rejected, "Owner-thread checks must precede metadata/cache access");
        {
            auto value = decode<T>(*user_context, shape(*user_context), 42, std::string(2048, 'r'));
            auto id = user_context->make_ref<T>(std::move(value));
            original_text = user_context->get(id).type()->data();
            expired.emplace(user_context->get(id));
            retained = user_context->make_global<T>(user_context->get(id));
        }
    }
    bool rejected = false;
    try {
        (void)expired->get();
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected && global.live_user_bytes() == 0,
          "Surviving typed borrows must not retain User arena blocks");
    auto next_user_context = global.make_user_context();
    auto r = next_user_context->memory_resource();
    check(&T::field_layout(r) == layout,
          "User recreation must retain the application-owned layout");
    const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    check(
        copy.type() && copy.type()->size() == 2048 && copy.type()->front() == 'r' &&
            copy.type()->data() != original_text &&
            copy.type()->get_allocator().resource() == global.memory_resource() &&
            copy.is_field_loaded("type_field"),
        "Explicit Global copies retain independent heap strings and indexed state after actual deletion");
    auto independent = decode<T>(*next_user_context, shape(*next_user_context), 1, std::nullopt);
    check(independent.loaded_state_snapshot()->identity() != old_snapshot &&
              &independent.loaded_state_snapshot()->get().layout() == layout,
          "User pools stay isolated while borrowing the same Global immutable layout");
}

static ContextLimits test_limits() {
    ContextLimits limits;
    limits.user.max_bytes = 512 * 1024 * 1024;
    limits.max_live_user_bytes = limits.user.max_bytes;
    return limits;
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            for (auto count : {1u, 100u, 10000u}) {
                for (bool independent : {false, true}) {
                    GlobalContext global(std::pmr::new_delete_resource(), 4096, test_limits());
                    auto user_context = global.make_user_context();
                    (void)bulk<cpp_member_aliases::Record>(*user_context, count, independent);
                    (void)bulk<cpp_member_aliases::ChildRecord>(*user_context, count, independent);
                }
            }
            cold_contexts_and_lifetimes<cpp_member_aliases::Record>();
            cold_contexts_and_lifetimes<cpp_member_aliases::ChildRecord>();
        } else {
            std::string line;
            while (std::getline(std::cin, line)) {
                GlobalContext global(std::pmr::new_delete_resource(), 4096, test_limits());
                auto user_context = global.make_user_context();
                auto r = user_context->memory_resource();
                auto input = Json::parse(r, line);
                const auto count = std::get<std::uint64_t>(field(*input, "count").data);
                const auto independent = std::get<bool>(field(*input, "independent").data);
                auto output =
                    text(field(*input, "model")) == "record"
                        ? bulk<cpp_member_aliases::Record>(*user_context, count, independent)
                        : bulk<cpp_member_aliases::ChildRecord>(*user_context, count, independent);
                std::cout << output.dump(r) << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
