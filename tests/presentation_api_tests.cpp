#include <teaql/core.hpp>
#include <teaql/runtime/context.hpp>
#include <array>
#include <iostream>
#include <list>
#include "presentation_binding_fixture.hpp"

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct OwnedEntity : EntityDefaults<OwnedEntity> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Projection";
    MutationValues values;

    OwnedEntity(std::allocator_arg_t, Allocator a, std::string_view text) : values(a.resource()) {
        values.fields.emplace("payload", Value(a.resource(), text));
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
        throw std::logic_error("Unused materializer");
    }

    MutationValues into_values() && {
        return std::move(values);
    }
};

template <class T>
concept BorrowedProjection =
    requires(Memory owner, const T& entity) { WebResponse::from_entity(owner, entity); };
static_assert(!BorrowedProjection<OwnedEntity>);

static void consuming_projection_contracts() {
    GlobalContext global;
    Ref<WebResponse> retained;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        String text(8192, 'e', owner);
        std::array entities{OwnedEntity(std::allocator_arg, Allocator(owner), text),
                            OwnedEntity(std::allocator_arg, Allocator(owner), text)};
        auto response = WebResponse::from_entities(global.memory_resource(), std::move(entities));
        check(response.data.size() == 2 && response.record_count == 2 &&
                  entities[0].values.fields.empty(),
              "arbitrary owning array projection consumes non-copyable entities");
        retained = user->make_global<WebResponse>(std::move(response));
        std::array borrowed{OwnedEntity(std::allocator_arg, Allocator(owner), text)};
        try {
            (void)WebResponse::from_entities(owner, borrowed);
            throw std::runtime_error("Borrowed non-copyable entity projection was accepted");
        } catch (const std::invalid_argument&) {
        }
        check(!borrowed[0].values.fields.empty(),
              "rejected borrowed projection preserves source values");
        std::pmr::list<Record> records(owner);
        Record record(owner);
        record.emplace("payload", Value(owner, text));
        records.emplace_back(std::move(record));
        auto projected = WebResponse::from_records(global.memory_resource(), std::move(records));
        check(
            projected.data.size() == 1 && projected.record_count == 1,
            "non-contiguous record ranges preserve complete projection without copying Record objects");
    }
    check(global.live_user_bytes() == 0,
          "actual source User destruction reclaims projection inputs");
    auto next = global.make_user_context();
    const auto& response = next->get(next->rebind_global(retained));
    const auto& object = std::get<Json::Object>(response.data[0].data);
    const auto& text = std::get<String>(object.find("payload")->second.data);
    check(response.memory_resource() == global.memory_resource() && text.size() == 8192 &&
              text.get_allocator().resource() == global.memory_resource(),
          "retained response owns all recursive JSON payloads in Global");
}

static void model_retention_contracts() {
    GlobalContext global;
    Ref<XlsWorkbook> retained;
    Ref<String> output;
    Ref<WebAction> action;
    {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        Json data(owner, String(8192, 'v', owner));
        XlsBlock block(std::allocator_arg, Allocator(owner), "page", 2, 3, data);
        block.span(2, 1).add_property("long property key requiring allocation", data);
        XlsBlock style(std::allocator_arg, Allocator(owner), "style", 0, 0, Json(owner));
        style.add_property("backgroundColor", Json(owner, "#ffeecc"));
        block.style(style);
        XlsPage page(std::allocator_arg, Allocator(owner), "page");
        page.add_block(block);
        auto& workbook = user->make<XlsWorkbook>();
        workbook.add_page(page).add_page(page);
        auto& web_action = user->make<WebAction>();
        web_action.set_warning_message("raw warning").set_request_url("/URL");
        auto before = user->allocation_stats().issued_objects;
        String text(owner);
        core::DebugWriter writer(text, {.pretty = true});
        core::write_debug(writer, workbook);
        check(text.find("style_refer_block") != String::npos && text.find("8192") == String::npos &&
                  text.find(String(8192, 'v', owner)) != String::npos,
              "raw structural diagnostics include complete nested styles, JSON and long text");
        check(user->allocation_stats().issued_objects == before,
              "presentation formatting buffers consume no object identities");
        retained = user->make_global<XlsWorkbook>(workbook);
        output = user->make_global<String>(text);
        action = user->make_global<WebAction>(web_action);
    }
    check(global.live_user_bytes() == 0,
          "actual source User destruction returns all presentation blocks");
    auto next = global.make_user_context();
    const auto& workbook = next->get(next->rebind_global(retained));
    const auto& block = workbook.pages[0].blocks[0];
    auto owner = global.memory_resource();
    check(workbook.memory_resource() == owner && block.memory_resource() == owner &&
              block.style_refer_block->memory_resource() == owner &&
              std::get<String>(block.value.data).get_allocator().resource() == owner &&
              block.properties.begin()->first.get_allocator().resource() == owner,
          "all retained pages, blocks, boxed styles, map keys and JSON strings use Global");
    check(debug_string(next->memory_resource(), workbook, {.pretty = true}) ==
              next->get(next->rebind_global(output)),
          "complete recursive model diagnostics survive source User deletion");
    const auto& web_action = next->get(next->rebind_global(action));
    auto wire = web_action.to_json_value(next->memory_resource());
    check(std::get<Json::Object>(wire.data).contains("requestURL") &&
              !std::get<Json::Object>(wire.data).contains("requestUrl"),
          "requestURL wire spelling remains distinct from raw diagnostic field name");
}

// Exact original Xls navigation test inputs and assertions.
static void original_navigation_contract() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    XlsBlockBuildContext context(std::allocator_arg, Allocator(owner), "orders", 2, 3);
    auto header = context.to_block(Json(owner, "Order No"));
    header.add_property("bold", Json(owner, true)).span(2, 1);
    auto next = context.next().to_block(Json(owner, "Amount"));
    auto line = context.next_line().to_block(Json(owner, "SO-1"));
    auto reset = context.new_line().to_block(Json(owner, "reset-left"));
    check(header.page == "orders" && header.left == 2 && header.top == 3 && header.right == 3 &&
              header.bottom == 3 && header.width() == 2 && header.height() == 1 &&
              header.contains(3, 3) && !header.contains(4, 3),
          "original header region and containment assertions");
    check(next.left == 3 && next.top == 3 && line.left == 2 && line.top == 4 && reset.left == 0 &&
              reset.top == 4 && std::get<bool>(header.properties.find("bold")->second.data),
          "original next/next-line/new-line and property assertions");
}

static void binding_dispatch_and_retention_contracts() {
    GlobalContext global;
    Ref<test::BindingEntity> retained;
    {
        auto user = global.make_user_context();
        retained = user->make_global<test::BindingEntity>(1u);
        auto& entity = user->get(retained);
        auto style = WebStyle::with_font_color(user->memory_resource(),
                                               String(8192, 's', user->memory_resource()));
        style.bind_entity(entity);
        check(entity.calls.size() == 1 && entity.calls.front() == STYLE_KEY &&
                  entity.data.dynamic("redirectedStyle") && !entity.data.dynamic(STYLE_KEY),
              "style binding dispatches the entity's dynamic override");
        auto action = WebAction::view_web_action(user->memory_resource());
        action.bind_entity(entity);
        check(entity.calls.size() == 1 && entity.data.dynamic(ACTION_LIST_KEY),
              "action binding preserves the source's direct base mutation");
        test::BindingEntity ignored(std::allocator_arg, Allocator(user->memory_resource()), 2u);
        style.bind_entity(ignored);
        check(ignored.calls.size() == 1 && ignored.data.dynamic_values.empty(),
              "an entity may suppress style binding through its override");
    }
    check(global.live_user_bytes() == 0,
          "binding retains no source User payload after actual destruction");
    auto next = global.make_user_context();
    const auto& entity = next->get(next->rebind_global(retained));
    const auto& json = std::get<Json>(entity.data.dynamic("redirectedStyle")->data);
    const auto& color =
        std::get<String>(std::get<Json::Object>(json.data).find("color")->second.data);
    check(entity.memory_resource() == global.memory_resource() && color.size() == 8192 &&
              color.get_allocator().resource() == global.memory_resource() &&
              entity.calls.front().get_allocator().resource() == global.memory_resource(),
          "binding stores complete JSON and override metadata in the entity owner");
}

struct SourceOrder : EntityDefaults<SourceOrder> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    Record fields;

    SourceOrder(std::allocator_arg_t,
                Allocator a,
                std::uint64_t id,
                std::int64_t version,
                std::string_view name)
        : fields(a.resource()) {
        fields.emplace("id", Value(a.resource(), id));
        fields.emplace("version", Value(a.resource(), version));
        fields.emplace("name", Value(a.resource(), name));
    }

    SourceOrder(std::allocator_arg_t, Allocator a, const SourceOrder& other)
        : fields(clone_record(a.resource(), other.fields)) {}

    SourceOrder(SourceOrder&&) noexcept = default;
    SourceOrder(const SourceOrder&) = delete;

    Memory memory_resource() const {
        return fields.get_allocator().resource();
    }

    static EntityDescriptor entity_descriptor(Memory owner) {
        return EntityDescriptor(std::allocator_arg, Allocator(owner), ENTITY_NAME);
    }

    static Result<SourceOrder, EntityError> from_compact_row(Memory, CompactRow) {
        throw std::logic_error("Unused original presentation materializer");
    }

    MutationValues into_values() && {
        return MutationValues(memory_resource(), std::move(fields));
    }
};

static const Json& json_field(const Json& value, std::string_view name) {
    return std::get<Json::Object>(value.data).find(name)->second;
}

// Exact original web model tests, using handwritten entity mappings.
static void original_web_contracts() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto success = WebResponse::success(owner);
    check(success.result_code == 0 && success.status == "YES" && !success.message &&
              success.version == WEB_RESPONSE_VERSION && success.data.empty() &&
              success.record_count == 0,
          "original success response assertions");
    auto fail = WebResponse::fail(owner, "Internal Error");
    check(fail.result_code == 1 && fail.status == "NO" && fail.message == "Internal Error" &&
              fail.data.empty() && fail.record_count == 0,
          "original failed response assertions");
    auto empty = WebResponse::empty_list(owner, "No items found");
    check(empty.result_code == 0 && !empty.status && empty.message == "No items found" &&
              empty.data.empty() && empty.record_count == 0,
          "original empty response assertions");
    auto wire = success.to_json_value(owner);
    check(!std::get<Json::Object>(wire.data).contains("message") &&
              std::get<Json::Array>(json_field(wire, "data").data).empty() &&
              std::get<Json::Object>(json_field(wire, "facets").data).empty(),
          "original success wire omission and empty collections");
    BaseEntityData base(std::allocator_arg, Allocator(owner));
    auto style = WebStyle::with_background_color(owner, "#ffeecc");
    style.font_color("#111111").bind_base(base);
    WebAction::view_web_action(owner).bind_base(base);
    WebAction::modify_web_action(owner, "EDIT", "/orders/1/edit").bind_base(base);
    const auto& bound = std::get<Json>(base.dynamic(STYLE_KEY)->data);
    check(std::get<String>(json_field(bound, "backgroundColor").data) == "#ffeecc" &&
              std::get<String>(json_field(bound, "color").data) == "#111111",
          "original style binding assertions");
    const auto& actions =
        std::get<Json::Array>(std::get<Json>(base.dynamic(ACTION_LIST_KEY)->data).data);
    check(actions.size() == 2 &&
              std::get<String>(json_field(actions[0], "execute").data) == "switchview" &&
              std::get<String>(json_field(actions[0], "target").data) == "detail" &&
              std::get<String>(json_field(actions[1], "name").data) == "EDIT" &&
              std::get<String>(json_field(actions[1], "requestURL").data) == "/orders/1/edit",
          "original action binding order and requestURL assertions");
    SourceOrder entity(std::allocator_arg, Allocator(owner), 7, 2, "order");
    auto single = WebResponse::from_entity(owner, entity);
    check(single.result_code == 0 && single.status == "YES" && single.record_count == 1 &&
              single.version == WEB_RESPONSE_VERSION &&
              std::get<std::uint64_t>(json_field(single.data[0], "id").data) == 7,
          "original borrowed entity response assertions");
    SmartList<SourceOrder> list(std::allocator_arg, Allocator(owner));
    list.push(entity);
    list.loaded().with_total_count(99);
    auto plain_list = detail::clone_in(owner, list);
    auto plain_response = WebResponse::from_smart_list(owner, std::move(plain_list));
    check(plain_response.record_count == 99 && plain_response.data.size() == 1,
          "original plain-list response assertions");
    Record facet_record(owner);
    facet_record.emplace("status", Value(owner, "PENDING"));
    facet_record.emplace("count", Value(owner, std::int64_t{5}));
    SmartList<CompactRow> facets(std::allocator_arg, Allocator(owner));
    facets.emplace_back(std::move(facet_record));
    facets.loaded();
    list.with_facet("status", facets);
    check(list.facets.contains("status") && list.facet("status")->len() == 1 &&
              list.facet_mut("status") && list.facets_mut().contains("status"),
          "original facet getter and mutator assertions");
    auto response = WebResponse::from_smart_list(owner, std::move(list));
    auto response_wire = response.to_json_value(owner);
    const auto& response_object = std::get<Json::Object>(response_wire.data);
    check(response_object.contains("facets"), "original response serializes facets");
    const auto& facets_object = std::get<Json::Object>(json_field(response_wire, "facets").data);
    check(facets_object.contains("status"), "original serialized facets contain status");
    const auto& status_facet = std::get<Json::Array>(facets_object.find("status")->second.data);
    check(response.record_count == 99 && response.data.size() == 1 && status_facet.size() == 1 &&
              std::get<String>(json_field(status_facet[0], "status").data) == "PENDING" &&
              json_field(status_facet[0], "count").dump(owner) == "5",
          "original response facet projection assertions");
    auto failed = WebResponse::fail(owner, "bad request").to_json_value(owner);
    check(std::get<String>(json_field(failed, "status").data) == "NO" &&
              std::get<String>(json_field(failed, "message").data) == "bad request" &&
              std::get<String>(json_field(failed, "version").data) == "1.001",
          "original failed response wire assertions");
    SmartList<SourceOrder> other(std::allocator_arg, Allocator(owner));
    other.emplace_back(8, 1, "other");
    other.loaded();
    other.with_facet("status", SmartList<CompactRow>(std::allocator_arg, Allocator(owner)));
    check(other.remove_facet("status").has_value() && !other.facet("status"),
          "original facet removal assertions");
    other.add_facet("status", SmartList<CompactRow>(std::allocator_arg, Allocator(owner)));
    auto taken = other.take_facets();
    check(taken.contains("status") && other.facets.empty(), "original facet take assertions");
}

static void original_workbook_contract() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    XlsBlock style(std::allocator_arg, Allocator(owner), "orders", 0, 0, Json(owner));
    style.add_property("backgroundColor", Json(owner, "#ffeecc"));
    XlsBlock title(std::allocator_arg, Allocator(owner), "orders", 0, 0, Json(owner, "Orders"));
    title.style(style).span(3, 1);
    XlsPage page(std::allocator_arg, Allocator(owner), "orders");
    page.add_block(title);
    XlsWorkbook workbook(std::allocator_arg, Allocator(owner));
    workbook.add_page(page);
    auto* found = workbook.page("orders");
    check(found && found->block_at(1, 0) &&
              std::get<String>(found->block_at(1, 0)->value.data) == "Orders",
          "original workbook page and block lookup assertions");
    auto wire = workbook.to_json_value(owner);
    const auto& wire_page = std::get<Json::Array>(json_field(wire, "pages").data)[0];
    const auto& wire_block = std::get<Json::Array>(json_field(wire_page, "blocks").data)[0];
    check(std::get<String>(json_field(wire_page, "name").data) == "orders" &&
              json_field(wire_block, "right").dump(owner) == "2" &&
              std::get<String>(
                  json_field(json_field(json_field(wire_block, "styleReferBlock"), "properties"),
                             "backgroundColor")
                      .data) == "#ffeecc",
          "original workbook ordered JSON and nested style assertions");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        consuming_projection_contracts();
        model_retention_contracts();
        original_navigation_contract();
        binding_dispatch_and_retention_contracts();
        original_web_contracts();
        original_workbook_contract();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
