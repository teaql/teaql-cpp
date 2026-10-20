#include <iostream>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/error_debug.hpp>
#include "../support/move_only_error_fixture.hpp"
#include "debug_modes.hpp"

using namespace teaql;

void write_debug(DebugWriter& writer, const MoveOnlyError& error) {
    writer.tuple("OwnedError", [&](auto& out) {
        out.text(error.message);
    });
}

static const Json& field(const Json& input, std::string_view key) {
    return std::get<Json::Object>(input.data).find(key)->second;
}

static std::string_view text(const Json& input, std::string_view key) {
    return std::get<String>(field(input, key).data);
}

static bool flag(const Json& input, std::string_view key) {
    const auto& object = std::get<Json::Object>(input.data);
    auto found = object.find(key);
    return found != object.end() && std::get<bool>(found->second.data);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto destination = global.memory_resource();
    for (std::string line; std::getline(std::cin, line);) {
        Json::Object output(destination);
        RuntimeError* retained = nullptr;
        unsigned destroyed = 0;
        bool executor = false;
        {
            auto user_context = global.make_user_context();
            auto owner = user_context->memory_resource();
            auto parsed = Json::parse(owner, line);
            if (!parsed) {
                throw std::runtime_error("Invalid error input");
            }
            executor = text(*parsed, "kind") == "executor";
            if (executor) {
                auto error = DataServiceError<MoveOnlyError>::executor(
                    owner,
                    MoveOnlyError(
                        std::allocator_arg, Allocator(owner), text(*parsed, "text"), destroyed));
                output.emplace("display", Json(destination, error.to_string(destination)));
                output.emplace("debug", formatted_debug(destination, error));
                output.emplace("source_none", Json(destination, error.source() == nullptr));
            } else {
                auto error = RuntimeError::event(owner, text(*parsed, "text"));
                if (text(*parsed, "kind") == "request") {
                    auto request_error = std::make_obj_using_allocator<RequestIntentError>(
                        Allocator(owner),
                        flag(*parsed, "mutation") ? RequestKind::Mutation : RequestKind::Query,
                        text(*parsed, "field"));
                    error = RuntimeError::request_intent(owner, request_error);
                } else if (text(*parsed, "kind") == "dynamic") {
                    DynamicFieldError dynamic(owner,
                                              flag(*parsed, "alternative")
                                                  ? "CUSTOM_DYNAMIC_CODE"
                                                  : "DYNAMIC_FIELD_TYPE_MISMATCH",
                                              text(*parsed, "text"));
                    error = RuntimeError::dynamic_field(owner, dynamic);
                }
                auto depth = std::get<std::uint64_t>(field(*parsed, "depth").data);
                for (std::uint64_t index = 0; index < depth; ++index) {
                    error = RuntimeError::audit_after_commit(owner, error);
                }
                retained = &global.make<RuntimeError>(error);
            }
        }
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("User memory retained");
        }
        if (executor) {
            output.emplace("destroyed_once", Json(destination, destroyed == 1));
        } else {
            const RuntimeError* nested = retained;
            for (;;) {
                if (nested->memory_resource() != destination) {
                    throw std::runtime_error("Wrong error owner");
                }
                if (nested->kind() != RuntimeError::Kind::AuditAfterCommit) {
                    break;
                }
                nested = &nested->audit_after_commit_error();
            }
            if (nested->kind() == RuntimeError::Kind::RequestIntent &&
                nested->request_intent_error().memory_resource() != destination) {
                throw std::runtime_error("Request error was not copied");
            }
            if (nested->kind() == RuntimeError::Kind::DynamicField &&
                nested->dynamic_field_error().memory_resource() != destination) {
                throw std::runtime_error("Dynamic error was not copied");
            }
            output.emplace("display", Json(destination, retained->to_string(destination)));
            output.emplace("debug", formatted_debug(destination, *retained));
            output.emplace("source_none", Json(destination, retained->source() == nullptr));
            auto copied =
                std::make_obj_using_allocator<RuntimeError>(Allocator(destination), *retained);
            auto wrapped = DataServiceError<MoveOnlyError>::runtime(destination, std::move(copied));
            output.emplace("wrapped_display", Json(destination, wrapped.to_string(destination)));
            output.emplace("wrapped_debug", formatted_debug(destination, wrapped));
            output.emplace("wrapped_source_none", Json(destination, wrapped.source() == nullptr));
        }
        std::cout << Json(destination, std::move(output)).dump(destination) << '\n';
    }
}
