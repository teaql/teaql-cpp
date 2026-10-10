#include <teaql/runtime/entity_key.hpp>
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <bit>
#include <charconv>
#include <teaql/core/detail/debug.hpp>

namespace teaql {
EntityKey::EntityKey(std::allocator_arg_t, Allocator a, std::string_view name, const Value& value)
    : entity(name, a.resource()), id(a.resource(), value), kind_(Kind::Null),
      identity_(a.resource()) {
    auto r = a.resource();
    if (value.is_null()) {
        return;
    }
    std::visit(
        [&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, bool>) {
                kind_ = Kind::Bool;
                identity_ = Value(r, v);
            } else if constexpr (std::same_as<T, std::int64_t>) {
                if (v < 0) {
                    kind_ = Kind::I64;
                    identity_ = Value(r, v);
                } else {
                    kind_ = Kind::U64;
                    identity_ = Value(r, std::uint64_t(v));
                }
            } else if constexpr (std::same_as<T, std::uint64_t>) {
                kind_ = Kind::U64;
                identity_ = Value(r, v);
            } else if constexpr (std::same_as<T, double>) {
                kind_ = Kind::F64;
                identity_ = Value(r, std::bit_cast<std::uint64_t>(v));
            } else if constexpr (std::same_as<T, Decimal>) {
                kind_ = Kind::Decimal;
                identity_ = Value(r, v);
            } else if constexpr (std::same_as<T, String>) {
                kind_ = Kind::Text;
                identity_ = Value(r, v);
            } else if constexpr (std::same_as<T, Date>) {
                kind_ = Kind::Date;
                identity_ = Value(r, v);
            } else if constexpr (std::same_as<T, Timestamp>) {
                kind_ = Kind::Timestamp;
                identity_ = Value(r, v);
            } else if constexpr (std::same_as<T, Json>) {
                kind_ = Kind::Other;
                String text("json:", r);
                text += v.dump(r);
                identity_ = Value(r, text);
            } else if constexpr (std::same_as<T, Value::Object>) {
                kind_ = Kind::Other;
                identity_ = Value(r, "object");
            } else if constexpr (std::same_as<T, Value::List>) {
                kind_ = Kind::Other;
                identity_ = Value(r, "list");
            }
        },
        value.data);
}

EntityKey::EntityKey(std::allocator_arg_t, Allocator a, const EntityKey& other)
    : entity(other.entity, a.resource()), id(a.resource(), other.id), kind_(other.kind_),
      identity_(a.resource(), other.identity_) {}

std::strong_ordering EntityKey::operator<=>(const EntityKey& other) const {
    if (auto order = entity <=> other.entity; order != 0) {
        return order;
    }
    if (auto order = kind_ <=> other.kind_; order != 0) {
        return order;
    }
    switch (kind_) {
    case Kind::Null:
        return std::strong_ordering::equal;
    case Kind::Bool:
        return std::get<bool>(identity_.data) <=> std::get<bool>(other.identity_.data);
    case Kind::I64:
        return std::get<std::int64_t>(identity_.data) <=>
               std::get<std::int64_t>(other.identity_.data);
    case Kind::U64:
    case Kind::F64:
        return std::get<std::uint64_t>(identity_.data) <=>
               std::get<std::uint64_t>(other.identity_.data);
    case Kind::Decimal:
        return std::get<Decimal>(identity_.data) <=> std::get<Decimal>(other.identity_.data);
    case Kind::Text:
    case Kind::Other:
        return std::get<String>(identity_.data) <=> std::get<String>(other.identity_.data);
    case Kind::Date:
        return std::get<Date>(identity_.data).epoch_days() <=>
               std::get<Date>(other.identity_.data).epoch_days();
    case Kind::Timestamp:
        return std::get<Timestamp>(identity_.data).as_millis() <=>
               std::get<Timestamp>(other.identity_.data).as_millis();
    }
    std::terminate();
}
}

namespace teaql {
void write_debug(DebugWriter& writer, const EntityKey& key) {
    writer.structure("EntityKey", [&](auto& fields) {
        fields.field("entity", [&](auto& out) {
            out.text(key.entity);
        });
        fields.field("id", [&](auto& out) {
            write_debug(out, key.id);
        });
        fields.field("id_key", [&](auto& out) {
            if (key.kind_ == EntityKey::Kind::Null) {
                out.raw("Null");
                return;
            }
            constexpr std::string_view names[]{"Null",
                                               "Bool",
                                               "I64",
                                               "U64",
                                               "F64",
                                               "Decimal",
                                               "Text",
                                               "Date",
                                               "Timestamp",
                                               "Other"};
            out.tuple(names[static_cast<unsigned>(key.kind_)], [&](auto& inner) {
                std::visit(
                    [&](const auto& value) {
                        using T = std::decay_t<decltype(value)>;
                        if constexpr (std::same_as<T, Timestamp>) {
                            inner.integer(value.as_millis());
                        } else if constexpr (std::same_as<T, bool> || std::integral<T> ||
                                             std::same_as<T, String> || std::same_as<T, Decimal> ||
                                             std::same_as<T, Date>) {
                            write_debug(inner, value);
                        } else {
                            throw std::logic_error("Invalid cached entity identity diagnostic");
                        }
                    },
                    key.identity_.data);
            });
        });
    });
}

String EntityKey::debug(Memory r) const {
    return debug_string(r, *this);
}
}
