#include <teaql/core/web.hpp>
#include <teaql/core/xls.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;

static void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

struct NoDefaultAllocator {
    Memory previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());

    ~NoDefaultAllocator() {
        std::pmr::set_default_resource(previous);
    }
};

static const Json& field(const Json& value, std::string_view name) {
    return std::get<Json::Object>(value.data).find(name)->second;
}

static void web_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    WebStyle* retained_style = nullptr;
    WebAction* retained_action = nullptr;
    WebResponse* retained_response = nullptr;
    Record* retained_record = nullptr;
    {
        auto style = WebStyle::with_background_color(
            r, "a background value long enough for allocated storage");
        style.font_color("#123456").set_class_names("compact");
        auto& base = global.make<BaseEntityData>();
        style.bind_base(base);
        check(std::holds_alternative<Json>(base.dynamic(STYLE_KEY)->data),
              "style binding stores Value::Json");
        check(std::get<Json>(base.dynamic(STYLE_KEY)->data).memory_resource() ==
                  global.memory_resource(),
              "binding chooses entity domain");
        auto action = WebAction::delete_web_action_with_warning(
            r, "/some/long/request/path", "long warning message requiring allocation");
        check(action.level == "modify" && action.name == "web.action.delete" &&
                  action.target == "modify",
              "delete-with-warning retains Rust modify action semantics");
        auto before = action.to_json_value(r);
        action.optional_warning_message({});
        check(!std::get<Json::Object>(action.to_json_value(r).data).contains("warningMessage"),
              "optional None clears warning");
        action.set_warning_message("long warning message requiring allocation");
        check(action.to_json_value(r) == before, "action fluent fields preserve text");
        action.bind_base(base);
        action.bind_base(base);
        auto& actions = std::get<Json>(base.dynamic_values.find(ACTION_LIST_KEY)->second.data);
        check(std::get<Json::Array>(actions.data).size() == 2,
              "existing JSON array appends actions");
        auto& record = source->make<Record>();
        Value::List legacy(r);
        legacy.emplace_back("old action");
        record.emplace(String(ACTION_LIST_KEY, r), Value(r, legacy));
        action.bind_record(record);
        const auto& items = std::get<Json::Array>(std::get<Json>(record.begin()->second.data).data);
        check(items.size() == 2 && std::holds_alternative<Json::Array>(items[0].data),
              "Value::List is projected as one prior action, not flattened");
        auto& null_record = source->make<Record>();
        null_record.emplace(String(ACTION_LIST_KEY, r), Value(r));
        action.bind_record(null_record);
        check(std::holds_alternative<std::monostate>(
                  std::get<Json::Array>(std::get<Json>(null_record.begin()->second.data).data)[0]
                      .data),
              "existing null remains first action");
        auto response = WebResponse::empty_list(r, "no results");
        check(!response.status && response.result_code == 0 && response.record_count == 0,
              "empty_list response distinct from success");
        auto failure = WebResponse::fail(r, "failed");
        check(failure.result_code == 1 && failure.status == "NO", "failure response defaults");
        response.push_json(
            *Json::parse(r, R"({"nested":["long response payload requiring storage"]})"));
        response.with_record_count(999);
        response.with_data(response.data);
        check(response.record_count == 1 && response.data.size() == 1,
              "aliased with_data retains payload and resets count");
        response.with_record_count(999).push_json(response.data[0]);
        check(response.record_count == 2, "push_json uses actual data length");
        response.facets.emplace("names", Json(r, "long facet payload requiring allocation"));
        response.with_facets(response.facets);
        check(response.facets.size() == 1, "aliased facets replacement");
        retained_style = &global.make<WebStyle>(style);
        retained_action = &global.make<WebAction>(action);
        retained_response = &global.make<WebResponse>(response);
        retained_record = &global.make<Record>(record);
        response.with_facets_option({});
        check(response.facets.empty(), "None facets clears old facets");
        auto roundtrip = WebResponse::from_json_value(r, retained_response->to_json_value(r));
        check(roundtrip && roundtrip.value() == *retained_response,
              "response JSON preserves count, status and facets");
    }
    source->close();
    auto g = global.memory_resource();
    check(retained_style->background_color->get_allocator().resource() == g &&
              retained_action->warning_message->get_allocator().resource() == g,
          "optional strings retained in explicit global domain");
    check(retained_response->data[0].memory_resource() == g &&
              retained_response->facets.begin()->second.memory_resource() == g,
          "nested response domain after source close");
    check(retained_response->to_json_value(g).dump(g).find("long response payload") != String::npos,
          "response data survives source close");
    check(std::get<Json>(retained_record->begin()->second.data).memory_resource() == g,
          "bound metadata survives deep copy");
}

static void xls_contracts() {
    NoDefaultAllocator guard;
    GlobalContext global;
    auto source = global.make_user_context();
    auto r = source->memory_resource();
    XlsWorkbook* retained = nullptr;
    XlsBlock* assigned = nullptr;
    {
        auto& context =
            source->make<XlsBlockBuildContext>("long page name requiring allocation", 3, 2);
        check(context.next().x == 4 && context.next().y == 2, "context next column");
        check(context.new_line().x == 0 && context.new_line().start_x == 3 &&
                  context.next_line().x == 3,
              "new_line and next_line have different column origins");
        auto& negative = source->make<XlsBlockBuildContext>("page", -5, -8);
        check(negative.x == 0 && negative.y == 0 && negative.start_x == 0,
              "constructor clamps negative coordinates");
        auto block = context.to_block(Json(r, "a long block value requiring allocation"));
        block.span(2, 3);
        check(block.width() == 2 && block.height() == 3 && block.contains(4, 4) &&
                  !block.contains(5, 4),
              "block dimensions and inclusive coordinates");
        auto& style = source->make<XlsBlock>("styles", 0, 0, Json(r));
        style.set_property("font", Json(r, "a long font configuration requiring allocation"));
        block.style(style).add_property(
            "payload", *Json::parse(r, R"({"nested":["long property requiring allocation"]})"));
        auto& page = source->make<XlsPage>(context.page);
        page.add_block(block);
        page.add_block(block);
        check(page.block_at(3, 2) == &page.blocks[0] && !page.block_at(0, 0),
              "overlapping blocks choose first match");
        auto& workbook = source->make<XlsWorkbook>();
        workbook.add_page(page);
        workbook.add_page(page);
        check(workbook.page(context.page) == &workbook.pages[0] && !workbook.page("absent"),
              "duplicate pages choose first match");
        retained = &global.make<XlsWorkbook>(workbook);
        assigned = &global.make<XlsBlock>("destination", 0, 0, Json(global.memory_resource()));
        *assigned = std::move(block);
        check(block.style_refer_block &&
                  assigned->style_refer_block.get() != block.style_refer_block.get(),
              "cross-domain move deeply copies recursive style");
        auto before = assigned->to_json_value(global.memory_resource());
        assigned->style(*assigned);
        check(assigned->style_refer_block->to_json_value(global.memory_resource()) == before,
              "self-style snapshots finite prior block");
        block.region(5, 5, 2, 1);
        check(block.width() == -2 && !block.contains(4, 3), "region preserves reversed bounds");
        block.region(0, 0, 0, 0).span(INT32_MIN, 0);
        check(block.right == INT32_MIN && block.bottom == -1,
              "span uses saturating subtraction only");
        bool overflow = false;
        try {
            block.region(INT32_MAX, 0, INT32_MAX, 0).span(2, 1);
        } catch (const std::overflow_error&) {
            overflow = true;
        }
        check(overflow, "coordinate overflow throws instead of C++ signed UB");
        context.x = INT32_MAX;
        overflow = false;
        try {
            (void)context.next();
        } catch (const std::overflow_error&) {
            overflow = true;
        }
        check(overflow, "navigation overflow rejects");
        // Null-valued style omits value when serialized; Rust also rejects that serialization on decode.
        auto no_value = style.to_json_value(r);
        check(!XlsBlock::from_json_value(r, no_value),
              "omitted null value does not silently become a decode default");
    }
    source->close();
    auto g = global.memory_resource();
    const auto& block = retained->pages[0].blocks[0];
    check(block.memory_resource() == g && block.style_refer_block->memory_resource() == g,
          "recursive block style survives source close");
    check(field(block.to_json_value(g), "properties").dump(g).find("long property") != String::npos,
          "nested XLS properties retained");
    check(retained->pages[0].name.get_allocator().resource() == g &&
              assigned->value.memory_resource() == g,
          "workbook nested allocators");
}

struct FailingResource : std::pmr::memory_resource {
    bool fail = false;
    std::size_t bytes = 0;

    void* do_allocate(std::size_t n, std::size_t alignment) override {
        if (fail)
            throw std::bad_alloc();
        auto p = std::pmr::new_delete_resource()->allocate(n, alignment);
        bytes += n;
        return p;
    }

    void do_deallocate(void* p, std::size_t n, std::size_t alignment) override {
        bytes -= n;
        std::pmr::new_delete_resource()->deallocate(p, n, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void allocation_failures() {
    NoDefaultAllocator guard;
    FailingResource upstream;
    {
        GlobalContext source;
        auto r = source.memory_resource();
        GlobalContext destination(&upstream, 128);
        auto d = destination.memory_resource();
        auto& large = source.make<String>(128 * 1024, 'x');
        auto& style = source.make<XlsBlock>("style", 0, 0, Json(r, large));
        auto& block = source.make<XlsBlock>("source", 1, 2, Json(r, 7));
        block.style(style);
        auto& target = destination.make<XlsBlock>("destination", 0, 0, Json(d, 11));
        upstream.fail = true;
        bool failed = false;
        try {
            target = std::move(block);
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        check(failed && target.page == "destination" &&
                  std::get<std::uint64_t>(target.value.data) == 11 && block.style_refer_block,
              "failed recursive clone preserves source and target");
        upstream.fail = false;
        auto& response = source.make<WebResponse>();
        response.push_json(Json(r, large));
        auto& kept = destination.make<WebResponse>();
        kept.with_record_count(17);
        upstream.fail = true;
        failed = false;
        try {
            kept = std::move(response);
        } catch (const std::bad_alloc&) {
            failed = true;
        }
        check(failed && kept.record_count == 17 && kept.data.empty() && response.data.size() == 1,
              "failed response clone preserves original state");
        upstream.fail = false;
    }
    check(upstream.bytes == 0, "failed presentation copies reclaim every arena block at close");
}

int main() {
    try {
        web_contracts();
        xls_contracts();
        allocation_failures();
        std::cout << "Presentation model contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
