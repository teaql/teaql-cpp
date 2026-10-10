#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include "../presentation_binding_fixture.hpp"
#include <iostream>
#include <teaql/core/serde_utils.hpp>
#include <list>

struct OwnedEntity : EntityDefaults<OwnedEntity> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Projection";
    MutationValues values;

    OwnedEntity(std::allocator_arg_t, Allocator a, const Json& payload) : values(a.resource()) {
        values.fields.emplace("payload", Value(a.resource(), payload));
    }

    OwnedEntity(std::allocator_arg_t, Allocator a, OwnedEntity&& other)
        : values(std::allocator_arg, a, std::move(other.values)) {}

    OwnedEntity(OwnedEntity&&) noexcept = default;
    OwnedEntity(const OwnedEntity&) = delete;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        return EntityDescriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
    }

    static Result<OwnedEntity, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Unused presentation materializer");
    }

    MutationValues into_values() && {
        return std::move(values);
    }
};

template <class T>
static Json decoded(Memory owner, const Json& input) {
    auto result = T::from_json_value(owner, input);
    Json::Object output(owner);
    output.emplace("accepted", Json(owner, bool(result)));
    if (result) {
        const auto& model = result.value();
        auto encoded = model.to_json_value(owner);
        auto roundtrip = T::from_json_value(owner, encoded);
        output.emplace("debug", formatted_debug(owner, model));
        output.emplace("serialized", std::move(encoded));
        output.emplace("clone_equal",
                       Json(owner, model == T(std::allocator_arg, Allocator(owner), model)));
        output.emplace("roundtrip_accepted", Json(owner, bool(roundtrip)));
        output.emplace("roundtrip_equal",
                       roundtrip ? Json(owner, roundtrip.value() == model) : Json(owner));
    }
    return Json(owner, std::move(output));
}

static Json evaluate(Memory owner, const Json& input) {
    auto kind = text(field(input, "kind"));
    const auto& value = field(input, "value");
    if (kind == "owning_ranges") {
        WebResponse result(std::allocator_arg, Allocator(owner));
        auto mode = text(field(input, "mode"));
        if (mode == "move_iterator") {
            std::array entities{OwnedEntity(std::allocator_arg, Allocator(owner), value),
                                OwnedEntity(std::allocator_arg, Allocator(owner), value)};
            auto named = std::ranges::subrange(std::make_move_iterator(entities.begin()),
                                               std::make_move_iterator(entities.end()));
            result = WebResponse::from_entities(owner, named);
        } else {
            auto named = std::views::iota(0, 2) | std::views::transform([&](int) {
                             return OwnedEntity(std::allocator_arg, Allocator(owner), value);
                         });
            result = WebResponse::from_entities(owner, named);
        }
        Json::Object output(owner);
        output.emplace("json", result.to_json_value(owner));
        output.emplace("debug", formatted_debug(owner, result));
        return Json(owner, std::move(output));
    }
    if (kind == "trim") {
        auto required = serde_utils::trimmed_string::deserialize(owner, value);
        auto optional = serde_utils::trimmed_opt_string::deserialize(owner, value);
        Json::Object output(owner);
        output.emplace("required_accepted", Json(owner, bool(required)));
        output.emplace("optional_accepted", Json(owner, bool(optional)));
        output.emplace("required", required ? Json(owner, required.value()) : Json(owner));
        output.emplace("optional",
                       optional && optional.value() ? Json(owner, *optional.value()) : Json(owner));
        if (const auto* text = std::get_if<String>(&value.data)) {
            output.emplace("serialized_required",
                           serde_utils::trimmed_string::serialize(owner, *text));
            output.emplace(
                "serialized_optional",
                serde_utils::trimmed_opt_string::serialize(owner, std::string_view(*text)));
        } else if (std::holds_alternative<std::monostate>(value.data)) {
            output.emplace("serialized_optional",
                           serde_utils::trimmed_opt_string::serialize(owner, std::nullopt));
        }
        output.emplace("original", Json(owner, value));
        return Json(owner, std::move(output));
    }
    if (kind == "bindings") {
        test::BindingEntity entity(
            std::allocator_arg,
            Allocator(owner),
            static_cast<unsigned>(std::get<std::uint64_t>(field(input, "mode").data)));
        BaseEntityData base(std::allocator_arg, Allocator(owner));
        Record record(owner);
        if (std::get<bool>(field(input, "present").data)) {
            for (auto key : {STYLE_KEY, ACTION_LIST_KEY}) {
                entity.data.put_dynamic(key, value);
                base.put_dynamic(key, value);
                record.emplace(String(key, owner), Value(owner, value));
            }
        }
        auto style = WebStyle::with_font_color(owner, "private\n雪");
        style.set_class_names("class");
        auto action = WebAction::view_web_action(owner);
        Json::Array states(owner);
        for (unsigned index = 0; index < 2; ++index) {
            style.bind_entity(entity);
            style.bind_base(base);
            style.bind_record(record);
            action.bind_entity(entity);
            action.bind_base(base);
            action.bind_record(record);
            Json::Array calls(owner);
            for (const auto& key : entity.calls) {
                calls.emplace_back(key);
            }
            Json::Object state(owner);
            state.emplace("entity", record_to_json_value(owner, entity.data.to_values_map(owner)));
            state.emplace("calls", Json(owner, std::move(calls)));
            state.emplace("base", record_to_json_value(owner, base.to_values_map(owner)));
            state.emplace("record", record_to_json_value(owner, record));
            states.emplace_back(std::move(state));
        }
        Json::Object output(owner);
        output.emplace("states", Json(owner, std::move(states)));
        return Json(owner, std::move(output));
    }
    if (kind == "ranges") {
        std::array entities{OwnedEntity(std::allocator_arg, Allocator(owner), value),
                            OwnedEntity(std::allocator_arg, Allocator(owner), value)};
        auto response = WebResponse::from_entities(owner, std::move(entities));
        std::pmr::list<Record> records(owner);
        Record first(owner), second(owner);
        first.emplace("value", Value(owner, std::int64_t{1}));
        second.emplace("value", Value(owner, value));
        records.emplace_back(std::move(first));
        records.emplace_back(std::move(second));
        auto projected = WebResponse::from_records(owner, std::move(records));
        Vector<OwnedEntity> data(owner);
        data.emplace_back(value);
        data.emplace_back(value);
        SmartList<OwnedEntity> list(std::allocator_arg, Allocator(owner), std::move(data));
        list.with_total_count(99);
        auto list_response = WebResponse::from_smart_list(owner, std::move(list));
        Json::Object output(owner);
        output.emplace("entities", formatted_debug(owner, response));
        output.emplace("entities_json", response.to_json_value(owner));
        output.emplace("records", formatted_debug(owner, projected));
        output.emplace("records_json", projected.to_json_value(owner));
        output.emplace("list", formatted_debug(owner, list_response));
        output.emplace("list_json", list_response.to_json_value(owner));
        return Json(owner, std::move(output));
    }
    if (kind == "style") {
        return decoded<WebStyle>(owner, value);
    }
    if (kind == "action") {
        return decoded<WebAction>(owner, value);
    }
    if (kind == "response") {
        return decoded<WebResponse>(owner, value);
    }
    if (kind == "block") {
        return decoded<XlsBlock>(owner, value);
    }
    if (kind == "context") {
        return decoded<XlsBlockBuildContext>(owner, value);
    }
    if (kind == "page") {
        return decoded<XlsPage>(owner, value);
    }
    if (kind == "workbook") {
        return decoded<XlsWorkbook>(owner, value);
    }
    auto flags = std::get<std::uint64_t>(field(input, "flags").data);
    WebStyle style(std::allocator_arg, Allocator(owner));
    style.set_background_color("").font_color("private\n雪").set_class_names("class");
    WebAction action(std::allocator_arg, Allocator(owner));
    action.set_key("")
        .set_name("name")
        .set_level("level")
        .set_execute("execute")
        .set_target("target")
        .set_component("component")
        .set_warning_message("warning")
        .set_role_for_list("role")
        .set_request_url("/URL")
        .optional_warning_message(flags & 1 ? std::nullopt : std::optional<std::string_view>(""));
    Json::Array items(owner);
    items.emplace_back(value);
    Json::Object facets(owner);
    facets.emplace("facet", Json(owner, value));
    auto response = WebResponse::success(owner);
    response.with_data(items).with_record_count(UINT64_MAX).with_facets(facets);
    XlsBlockBuildContext context(
        std::allocator_arg, Allocator(owner), "page\n雪", flags & 1 ? -2 : 2, flags & 2 ? -3 : 3);
    auto block = context.to_block(value);
    XlsBlock style_block(std::allocator_arg, Allocator(owner), "style", 0, 0, Json(owner));
    block.region(-3, -2, 4, 5).span(2, 3).add_property("private", value).style(style_block);
    XlsPage page(std::allocator_arg, Allocator(owner), "page\n雪");
    page.add_block(block).add_block(block);
    XlsWorkbook workbook(std::allocator_arg, Allocator(owner));
    workbook.add_page(page).add_page(page);
    Json::Object output(owner);
    auto observe = [&](std::string_view name, const auto& model) {
        output.emplace(name, formatted_debug(owner, model));
    };
    observe("style", style);
    observe("action", action);
    observe("response", response);
    observe("context", context);
    observe("next", context.next());
    observe("next_line", context.next_line());
    observe("new_line", context.new_line());
    observe("block", block);
    observe("page", page);
    observe("workbook", workbook);
    output.emplace("style_json", style.to_json_value(owner));
    output.emplace("action_json", action.to_json_value(owner));
    output.emplace("response_json", response.to_json_value(owner));
    output.emplace("block_json", block.to_json_value(owner));
    output.emplace("workbook_json", workbook.to_json_value(owner));
    auto found_block = page.block_at(-3, -2);
    auto found_page = workbook.page("page\n雪");
    output.emplace("block_found", found_block ? formatted_debug(owner, *found_block) : Json(owner));
    output.emplace("page_found", found_page ? formatted_debug(owner, *found_page) : Json(owner));
    output.emplace("missing_block", Json(owner, page.block_at(99, 99) == nullptr));
    output.emplace("missing_page", Json(owner, workbook.page("missing") == nullptr));
    observe("default_style", WebStyle(std::allocator_arg, Allocator(owner)));
    observe("default_action", WebAction(std::allocator_arg, Allocator(owner)));
    observe("default_page", XlsPage(std::allocator_arg, Allocator(owner)));
    observe("default_workbook", XlsWorkbook(std::allocator_arg, Allocator(owner)));
    return Json(owner, std::move(output));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::runtime_error("Invalid presentation model fixture");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
