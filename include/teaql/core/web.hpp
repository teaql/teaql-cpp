#pragma once
#include <teaql/core/list.hpp>
#include <teaql/core/model_decode.hpp>

namespace teaql {
inline constexpr std::string_view STYLE_KEY = "style", ACTION_LIST_KEY = "actionList",
                                  WEB_RESPONSE_VERSION = "1.001";

struct WebStyle {
    using allocator_type = Allocator;
    std::optional<String> background_color, color, class_names;

    WebStyle(std::allocator_arg_t, Allocator a) : resource_(a.resource()) {}

    WebStyle(std::allocator_arg_t, Allocator, const WebStyle&);

    WebStyle(std::allocator_arg_t, Allocator a, WebStyle&& other)
        : WebStyle(detail::move_to(a.resource(), std::move(other))) {}

    WebStyle(const WebStyle&) = delete;
    WebStyle(WebStyle&&) noexcept = default;

    WebStyle& operator=(WebStyle&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return resource_;
    }

    static WebStyle with_background_color(Memory, std::string_view);
    static WebStyle with_font_color(Memory, std::string_view);
    static WebStyle with_class_names(Memory, std::string_view);

    WebStyle& set_background_color(std::string_view value) {
        background_color = String(value, resource_);
        return *this;
    }

    WebStyle& font_color(std::string_view value) {
        color = String(value, resource_);
        return *this;
    }

    WebStyle& set_class_names(std::string_view value) {
        class_names = String(value, resource_);
        return *this;
    }

    Json to_json_value(Memory destination) const;
    static ModelResult<WebStyle> from_json_value(Memory, const Json&);

    void bind_base(BaseEntityData& entity) const {
        bind_record(entity.dynamic_values);
    }

    template <BaseEntity T>
    void bind_entity(T& entity) const {
        bind_base(entity.base_mut());
    }

    void bind_record(Record&) const;

    bool operator==(const WebStyle& other) const {
        return background_color == other.background_color && color == other.color &&
               class_names == other.class_names;
    }

private:
    Memory resource_;
};

struct WebAction {
    using allocator_type = Allocator;
    std::optional<String> key, name, level, execute, target, component, warning_message,
        role_for_list, request_url;

    WebAction(std::allocator_arg_t, Allocator a) : resource_(a.resource()) {}

    WebAction(std::allocator_arg_t, Allocator, const WebAction&);

    WebAction(std::allocator_arg_t, Allocator a, WebAction&& other)
        : WebAction(detail::move_to(a.resource(), std::move(other))) {}

    WebAction(const WebAction&) = delete;
    WebAction(WebAction&&) noexcept = default;

    WebAction& operator=(WebAction&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return resource_;
    }

    static WebAction view_web_action(Memory);
    static WebAction view_sub_list_action(Memory,
                                          std::string_view name,
                                          std::string_view list_view_name,
                                          std::string_view role_for_list);
    static WebAction
    simple_component_action(Memory, std::string_view name, std::string_view component_name);
    static WebAction modify_web_action(Memory, std::string_view name, std::string_view url);
    static WebAction modify_web_action_with_warning(Memory,
                                                    std::string_view name,
                                                    std::string_view url,
                                                    std::optional<std::string_view> warning);
    static WebAction default_modify_web_action(Memory);
    static WebAction delete_web_action(Memory);
    static WebAction
    delete_web_action_with_warning(Memory, std::string_view url, std::string_view warning);
    static WebAction audit_web_action(Memory, std::string_view url, std::string_view warning);
    static WebAction discard_web_action(Memory, std::string_view url, std::string_view warning);
    static WebAction
    goto_action(Memory, std::string_view name, std::string_view target, std::string_view url);
    static WebAction
    switch_view_action(Memory, std::string_view view_name, std::string_view target);
    static WebAction add_new_web_action(Memory, std::string_view object_display_name);
    static WebAction batch_upload_web_action(Memory);
    static Vector<WebAction> common_web_actions(Memory);

    WebAction& set_key(std::string_view value) {
        key = String(value, resource_);
        return *this;
    }

    WebAction& set_name(std::string_view value) {
        name = String(value, resource_);
        return *this;
    }

    WebAction& set_level(std::string_view value) {
        level = String(value, resource_);
        return *this;
    }

    WebAction& set_execute(std::string_view value) {
        execute = String(value, resource_);
        return *this;
    }

    WebAction& set_target(std::string_view value) {
        target = String(value, resource_);
        return *this;
    }

    WebAction& set_component(std::string_view value) {
        component = String(value, resource_);
        return *this;
    }

    WebAction& set_warning_message(std::string_view value) {
        warning_message = String(value, resource_);
        return *this;
    }

    WebAction& set_role_for_list(std::string_view value) {
        role_for_list = String(value, resource_);
        return *this;
    }

    WebAction& set_request_url(std::string_view value) {
        request_url = String(value, resource_);
        return *this;
    }

    WebAction& optional_warning_message(std::optional<std::string_view> value) {
        if (value) {
            warning_message = String(*value, resource_);
        } else {
            warning_message.reset();
        }
        return *this;
    }

    Json to_json_value(Memory destination) const;
    static ModelResult<WebAction> from_json_value(Memory, const Json&);

    void bind_base(BaseEntityData& entity) const {
        bind_record(entity.dynamic_values);
    }

    template <BaseEntity T>
    void bind_entity(T& entity) const {
        bind_base(entity.base_mut());
    }

    void bind_record(Record&) const;
    bool operator==(const WebAction&) const;

private:
    Memory resource_;
};

struct WebResponse {
    using allocator_type = Allocator;
    Json::Array data;
    std::int32_t result_code = 0;
    std::optional<String> status, message;
    std::uint64_t record_count = 0;
    String version;
    Json::Object facets;
    WebResponse(std::allocator_arg_t, Allocator);
    WebResponse(std::allocator_arg_t, Allocator, const WebResponse&);

    WebResponse(std::allocator_arg_t, Allocator a, WebResponse&& other)
        : WebResponse(detail::move_to(a.resource(), std::move(other))) {}

    WebResponse(const WebResponse&) = delete;
    WebResponse(WebResponse&&) noexcept = default;

    WebResponse& operator=(WebResponse&& other) {
        return detail::assign_scoped(*this, std::move(other));
    }

    Memory memory_resource() const {
        return data.get_allocator().resource();
    }

    static WebResponse success(Memory r) {
        return {std::allocator_arg, Allocator(r)};
    }

    static WebResponse fail(Memory, std::string_view message);
    static WebResponse empty_list(Memory, std::string_view message);
    static WebResponse from_records(Memory, std::span<const Record>);

    template <Entity T>
    static WebResponse from_entity(Memory r, const T& entity) {
        auto copy = detail::clone_in(r, entity);
        auto values = std::move(copy).into_values();
        auto result = success(r);
        result.push_json(record_to_json_value(r, values.fields));
        return result;
    }

    template <Entity T>
    static WebResponse from_entities(Memory r, Vector<T> entities) {
        auto result = success(r);
        for (auto& entity : entities) {
            auto values = std::move(entity).into_values();
            result.push_json(record_to_json_value(r, values.fields));
        }
        return result;
    }

    template <Entity T>
    static WebResponse from_smart_list(Memory r, SmartList<T>&& list) {
        auto count = list.total_count_or_len();
        auto source_facets = list.take_facets();
        Json::Object facets(r);
        for (const auto& [key, facet] : source_facets) {
            Json items(r);
            items.data.emplace<Json::Array>(r);
            for (const auto& row : facet.data) {
                std::get<Json::Array>(items.data).emplace_back(compact_row_to_json_value(r, row));
            }
            facets.emplace(String(key, r), std::move(items));
        }
        auto result = from_entities(r, std::move(list).into_vec());
        result.with_record_count(count).with_facets(facets);
        return result;
    }

    WebResponse& with_data(const Json::Array&);

    WebResponse& with_record_count(std::uint64_t count) {
        record_count = count;
        return *this;
    }

    WebResponse& with_facets(const Json::Object&);
    WebResponse& with_facets_option(const std::optional<Json::Object>&);
    WebResponse& push_json(const Json&);
    Json to_json_value(Memory destination) const;
    static ModelResult<WebResponse> from_json_value(Memory, const Json&);
    bool operator==(const WebResponse&) const;
};
}
