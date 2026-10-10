#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <algorithm>
#include <charconv>
#include <iostream>

using namespace teaql;

static const Json& field(const Json& input, std::string_view name) {
    return std::get<Json::Object>(input.data).at(String(name, input.memory_resource()));
}

static std::string_view text(const Json& value) {
    return std::get<String>(value.data);
}

static String millis_text(Memory r, std::int64_t millis) {
    char buffer[32];
    auto end = std::to_chars(buffer, buffer + sizeof(buffer), millis).ptr;
    return String(std::string_view(buffer, end - buffer), r);
}

static Json observe(UserContext& user, const Json& input) {
    auto r = user.memory_resource();
    Json::Object result(r);
    if (text(field(input, "kind")) == "timestamp") {
        Vector<Timestamp> values(r);
        for (const auto& value : std::get<Json::Array>(field(input, "values").data)) {
            std::int64_t millis = 0;
            auto raw = text(value);
            auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), millis);
            if (parsed.ec != std::errc{} || parsed.ptr != raw.data() + raw.size()) {
                throw std::runtime_error("invalid timestamp fixture");
            }
            values.emplace_back(millis);
        }
        Vector<Timestamp> sorted(values, r);
        std::ranges::sort(sorted);
        Json::Array order(r), comparisons(r), hashes(r);
        for (const auto& value : sorted) {
            order.emplace_back(millis_text(r, value.as_millis()));
        }
        for (const auto& a : values) {
            for (const auto& b : values) {
                auto comparison = a <=> b;
                comparisons.emplace_back(comparison < 0 ? -1 : comparison > 0 ? 1 : 0);
            }
            // Original HashInput proves a single signed scalar write. Standard
            // C++ hashing deliberately delegates to that scalar, not a wire hash.
            auto input = a.as_millis();
            if (std::hash<Timestamp>{}(a) != std::hash<std::int64_t>{}(input)) {
                throw std::runtime_error("timestamp hash did not preserve scalar input");
            }
            hashes.emplace_back(millis_text(r, input));
        }
        result.emplace("default", Json(r, millis_text(r, Timestamp{}.as_millis())));
        result.emplace("sorted", Json(r, std::move(order)));
        result.emplace("comparisons", Json(r, std::move(comparisons)));
        result.emplace("hash_inputs", Json(r, std::move(hashes)));
    } else {
        auto& descriptor = user.make<EntityDescriptor>(text(field(input, "name")));
        auto read_names = [&](const Json& value) {
            Vector<String> names(r);
            for (const auto& name : std::get<Json::Array>(value.data)) {
                names.emplace_back(text(name));
            }
            return names;
        };
        const auto& initial = field(input, "initial");
        if (!std::holds_alternative<std::monostate>(initial.data)) {
            auto names = read_names(initial);
            descriptor.mask_fields(names);
        }
        auto initial_declared = descriptor.audit_mask_fields_declared;
        auto names = read_names(field(input, "fields"));
        descriptor.mask_fields(names);
        if (std::get<bool>(field(input, "alias").data)) {
            descriptor.mask_fields(std::span<const String>(descriptor.audit_mask_fields));
        }
        Json::Array fields(r);
        for (const auto& name : descriptor.audit_mask_fields) {
            fields.emplace_back(name);
        }
        result.emplace("name", Json(r, descriptor.name));
        result.emplace("table", Json(r, descriptor.table_name));
        result.emplace("initial_declared", Json(r, initial_declared));
        result.emplace("declared", Json(r, descriptor.audit_mask_fields_declared));
        result.emplace("fields", Json(r, std::move(fields)));
        auto copy = detail::clone_in(r, descriptor);
        result.emplace("clone_equal", Json(r, copy == descriptor));
    }
    return Json(r, std::move(result));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user = global.make_user_context();
            auto input = Json::parse(user->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid JSON fixture");
            }
            std::cout << observe(*user, *input).dump(user->memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
