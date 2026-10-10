#include <teaql/provider_sqlite/native_diagnostics.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Vector<std::uint8_t> bytes(Memory r, const Json& value) {
    Vector<std::uint8_t> result(r);
    for (const auto& byte : std::get<Json::Array>(value.data)) {
        auto number = std::get<std::uint64_t>(byte.data);
        if (number > UINT8_MAX) {
            throw std::invalid_argument("Invalid fixture byte");
        }
        result.push_back(static_cast<std::uint8_t>(number));
    }
    return result;
}

static Json json_bytes(Memory r, std::span<const std::uint8_t> bytes) {
    Json::Array array(r);
    for (auto byte : bytes) {
        array.emplace_back(static_cast<std::uint64_t>(byte));
    }
    return Json(r, std::move(array));
}

static Json evaluate(Memory r, const Json& input) {
    auto raw = bytes(r, field(input, "bytes"));
    std::string_view view(reinterpret_cast<const char*>(raw.data()), raw.size());
    Json::Object result(r);
    auto utf8 = SqliteUtf8Error::validate(view);
    if (utf8) {
        Json::Object item(r);
        item.emplace("debug", formatted_debug(r, *utf8));
        item.emplace("display", Json(r, utf8->to_string(r)));
        item.emplace("valid_up_to", Json(r, static_cast<std::uint64_t>(utf8->valid_up_to())));
        item.emplace("error_len",
                     utf8->error_len() ? Json(r, static_cast<std::uint64_t>(*utf8->error_len()))
                                       : Json(r));
        item.emplace("source_none", Json(r, utf8->source() == nullptr));
        item.emplace("clone_equal", Json(r, *utf8 == SqliteUtf8Error(*utf8)));
        result.emplace("utf8", Json(r, std::move(item)));
    } else {
        result.emplace("utf8", Json(r));
    }
    auto nul = SqliteNulError::from_bytes(r, raw);
    if (nul) {
        Json::Object item(r);
        item.emplace("debug", formatted_debug(r, *nul));
        item.emplace("display", Json(r, nul->to_string(r)));
        item.emplace("position", Json(r, static_cast<std::uint64_t>(nul->nul_position())));
        SqliteNulError copy(std::allocator_arg, Allocator(r), *nul);
        item.emplace("clone_equal", Json(r, *nul == copy));
        item.emplace("source_none", Json(r, nul->source() == nullptr));
        auto recovered = std::move(copy).into_vec();
        item.emplace("into_vec", json_bytes(r, recovered));
        result.emplace("nul", Json(r, std::move(item)));
    } else {
        result.emplace("nul", Json(r));
    }
    SqlitePath path(std::allocator_arg, Allocator(r), view);
    auto other_bytes = bytes(r, field(input, "other"));
    SqlitePath other(
        std::allocator_arg,
        Allocator(r),
        std::string_view(reinterpret_cast<const char*>(other_bytes.data()), other_bytes.size()));
    Json::Object item(r);
    item.emplace("debug", formatted_debug(r, path));
    item.emplace("lossy", Json(r, path.to_string_lossy(r)));
    item.emplace("bytes", json_bytes(r, raw));
    item.emplace("other_equal", Json(r, path == other));
    result.emplace("path", Json(r, std::move(item)));
    return Json(r, std::move(result));
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto owner = user->memory_resource();
        auto input = Json::parse(owner, line);
        if (!input) {
            throw std::invalid_argument("Invalid diagnostics fixture JSON");
        }
        std::cout << evaluate(owner, *input).dump(owner) << '\n';
    }
}
