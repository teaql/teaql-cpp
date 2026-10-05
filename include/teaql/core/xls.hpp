#pragma once
#include <teaql/core/json.hpp>
#include <teaql/core/model_decode.hpp>
#include <teaql/memory/scoped.hpp>

namespace teaql {
struct XlsBlockBuildContext;

struct XlsBlock {
    using allocator_type = Allocator;
    String page;
    std::int32_t top = 0, bottom = 0, left = 0, right = 0;
    detail::ScopedBox<XlsBlock> style_refer_block;
    Json value;
    Json::Object properties;
    XlsBlock(std::allocator_arg_t,
             Allocator,
             std::string_view page,
             std::int32_t x,
             std::int32_t y,
             const Json&);
    XlsBlock(std::allocator_arg_t, Allocator, const XlsBlock&);

    XlsBlock(std::allocator_arg_t, Allocator a, XlsBlock&& other)
        : XlsBlock(detail::move_to(a.resource(), std::move(other))) {}

    XlsBlock(const XlsBlock&) = delete;
    XlsBlock(XlsBlock&&) noexcept = default;

    XlsBlock& operator=(XlsBlock&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return page.get_allocator().resource();
    }

    static XlsBlock from_context(const XlsBlockBuildContext&, const Json&);
    XlsBlock& region(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom);
    XlsBlock& span(std::int32_t width, std::int32_t height);

    XlsBlock& set_value(const Json& input) {
        value = Json(memory_resource(), input);
        return *this;
    }

    XlsBlock& add_property(std::string_view name, const Json& input) {
        set_property(name, input);
        return *this;
    }

    void set_property(std::string_view, const Json&);
    XlsBlock& style(const XlsBlock& input);
    std::int32_t width() const;
    std::int32_t height() const;

    bool contains(std::int32_t x, std::int32_t y) const {
        return x >= left && x <= right && y >= top && y <= bottom;
    }

    Json to_json_value(Memory destination) const;
    static ModelResult<XlsBlock> from_json_value(Memory, const Json&);
    bool operator==(const XlsBlock&) const;
};

struct XlsBlockBuildContext {
    using allocator_type = Allocator;
    String page;
    std::int32_t start_x = 0, x = 0, y = 0;
    XlsBlockBuildContext(std::allocator_arg_t,
                         Allocator,
                         std::string_view page,
                         std::int32_t x = 0,
                         std::int32_t y = 0);
    XlsBlockBuildContext(std::allocator_arg_t, Allocator, const XlsBlockBuildContext&);

    XlsBlockBuildContext(std::allocator_arg_t, Allocator a, XlsBlockBuildContext&& other)
        : XlsBlockBuildContext(detail::move_to(a.resource(), std::move(other))) {}

    XlsBlockBuildContext(const XlsBlockBuildContext&) = delete;
    XlsBlockBuildContext(XlsBlockBuildContext&&) noexcept = default;

    XlsBlockBuildContext& operator=(XlsBlockBuildContext&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return page.get_allocator().resource();
    }

    static XlsBlockBuildContext for_page(Memory r, std::string_view name) {
        return {std::allocator_arg, Allocator(r), name};
    }

    XlsBlockBuildContext next() const;
    XlsBlockBuildContext new_line() const;
    XlsBlockBuildContext next_line() const;

    XlsBlock to_block(const Json& value) const {
        return XlsBlock::from_context(*this, value);
    }

    Json to_json_value(Memory destination) const;
    static ModelResult<XlsBlockBuildContext> from_json_value(Memory, const Json&);

    bool operator==(const XlsBlockBuildContext& other) const {
        return page == other.page && start_x == other.start_x && x == other.x && y == other.y;
    }
};

struct XlsPage {
    using allocator_type = Allocator;
    String name;
    Vector<XlsBlock> blocks;

    XlsPage(std::allocator_arg_t, Allocator a, std::string_view name = {})
        : name(name, a.resource()), blocks(a.resource()) {}

    XlsPage(std::allocator_arg_t, Allocator, const XlsPage&);

    XlsPage(std::allocator_arg_t, Allocator a, XlsPage&& other)
        : XlsPage(detail::move_to(a.resource(), std::move(other))) {}

    XlsPage(const XlsPage&) = delete;
    XlsPage(XlsPage&&) noexcept = default;

    XlsPage& operator=(XlsPage&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    XlsPage& add_block(const XlsBlock& value) {
        push_block(value);
        return *this;
    }

    void push_block(const XlsBlock& value) {
        blocks.emplace_back(value);
    }

    const XlsBlock* block_at(std::int32_t x, std::int32_t y) const;
    Json to_json_value(Memory destination) const;
    static ModelResult<XlsPage> from_json_value(Memory, const Json&);

    bool operator==(const XlsPage& other) const {
        return name == other.name && blocks == other.blocks;
    }
};

struct XlsWorkbook {
    using allocator_type = Allocator;
    Vector<XlsPage> pages;

    XlsWorkbook(std::allocator_arg_t, Allocator a) : pages(a.resource()) {}

    XlsWorkbook(std::allocator_arg_t, Allocator, const XlsWorkbook&);

    XlsWorkbook(std::allocator_arg_t, Allocator a, XlsWorkbook&& other)
        : XlsWorkbook(detail::move_to(a.resource(), std::move(other))) {}

    XlsWorkbook(const XlsWorkbook&) = delete;
    XlsWorkbook(XlsWorkbook&&) noexcept = default;

    XlsWorkbook& operator=(XlsWorkbook&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return pages.get_allocator().resource();
    }

    XlsWorkbook& add_page(const XlsPage& value) {
        push_page(value);
        return *this;
    }

    void push_page(const XlsPage& value) {
        pages.emplace_back(value);
    }

    const XlsPage* page(std::string_view name) const;
    Json to_json_value(Memory destination) const;
    static ModelResult<XlsWorkbook> from_json_value(Memory, const Json&);

    bool operator==(const XlsWorkbook& other) const {
        return pages == other.pages;
    }
};
}
