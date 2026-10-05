#include <teaql/core/list.hpp>
#include <teaql/core/detail/number_text.hpp>
#include <charconv>

namespace teaql::detail {
String list_id_key(Memory r, const Value& value) {
    return std::visit(
        [r](const auto& item) -> String {
            using T = std::decay_t<decltype(item)>;
            String result(r);
            auto integer = [&](auto n) {
                char buffer[32];
                auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), n);
                if (error != std::errc{})
                    throw std::runtime_error("ID formatting failed");
                result.append(buffer, end);
            };
            if constexpr (std::same_as<T, std::monostate> || std::same_as<T, TypedNull>)
                result = "null";
            else if constexpr (std::same_as<T, bool>)
                result = item ? "b:true" : "b:false";
            else if constexpr (std::same_as<T, std::int64_t>) {
                result = "i:";
                integer(item);
            } else if constexpr (std::same_as<T, std::uint64_t>) {
                result = "u:";
                integer(item);
            } else if constexpr (std::same_as<T, double>) {
                result = "f:";
                append_float(result, item, false);
            } else if constexpr (std::same_as<T, String>) {
                result = "t:";
                result += item;
            } else if constexpr (std::same_as<T, Decimal>) {
                result = "decimal:";
                result += item.to_string(r);
            } else if constexpr (std::same_as<T, Json>) {
                result = "j:";
                result += item.dump(r);
            } else if constexpr (std::same_as<T, Date>) {
                result = "date:";
                result += item.to_string(r);
            } else if constexpr (std::same_as<T, Timestamp>) {
                result = "ts:";
                integer(item.as_millis());
            } else if constexpr (std::same_as<T, Value::Object>)
                result = "object";
            else if constexpr (std::same_as<T, Value::List>)
                result = "list";
            return result;
        },
        value.data);
}
}
