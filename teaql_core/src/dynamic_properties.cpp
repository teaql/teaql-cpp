#include <teaql/core/dynamic_properties.hpp>
#include <teaql/core/detail/rust_default_hasher.hpp>
#include <teaql/core/detail/text.hpp>
#include <limits>
#include <algorithm>

namespace teaql {
Result<DynamicPropertyDefinitions, String>
DynamicPropertyDefinitions::from_fields(Memory r, std::span<const Field> fields) {
    using Output = Result<DynamicPropertyDefinitions, String>;
    DynamicPropertyDefinitions result(r);
    for (const auto& [name, type] : fields) {
        auto suffix = name.empty() ? std::string_view{} : name.substr(1);
        detail::trim(suffix);
        if (name.empty() || name.front() != '_' || suffix.empty()) {
            return Output::err(r, String("dynamic property metadata requires '_' names", r));
        }
        if (!result.types_.emplace(String(name, r), type).second) {
            String error("duplicate dynamic property definition: ", r);
            error += name;
            return Output::err(r, std::move(error));
        }
    }
    result.update_fingerprint();
    return Output::ok(r, std::move(result));
}

DynamicPropertyDefinitions::DynamicPropertyDefinitions(std::allocator_arg_t,
                                                       Allocator a,
                                                       const DynamicPropertyDefinitions& other)
    : types_(other.types_, a.resource()), fingerprint_(other.fingerprint_) {}

std::optional<DataType> DynamicPropertyDefinitions::data_type(std::string_view name) const {
    auto found = types_.find(name);
    return found == types_.end() ? std::nullopt : std::optional(found->second);
}

Result<std::monostate, String>
DynamicPropertyDefinitions::validate_value(std::string_view name, const Value& value) const {
    auto r = memory_resource();
    using Output = Result<std::monostate, String>;
    auto declared = data_type(name);
    if (!declared) {
        return Output::ok(r, {});
    }
    auto matches = std::visit(
        [&](const auto& actual) {
            using T = std::decay_t<decltype(actual)>;
            if constexpr (std::same_as<T, std::monostate>) {
                return true;
            } else if constexpr (std::same_as<T, TypedNull>) {
                return actual.type == *declared;
            } else if constexpr (std::same_as<T, bool>) {
                return *declared == DataType::Bool;
            } else if constexpr (std::same_as<T, std::int64_t>) {
                return *declared == DataType::I64;
            } else if constexpr (std::same_as<T, std::uint64_t>) {
                return *declared == DataType::U64;
            } else if constexpr (std::same_as<T, double>) {
                return *declared == DataType::F64;
            } else if constexpr (std::same_as<T, Decimal>) {
                return *declared == DataType::Decimal;
            } else if constexpr (std::same_as<T, String>) {
                return *declared == DataType::Text || *declared == DataType::LargeText;
            } else if constexpr (std::same_as<T, Date>) {
                return *declared == DataType::Date;
            } else if constexpr (std::same_as<T, Timestamp>) {
                return *declared == DataType::Timestamp;
            } else if constexpr (std::same_as<T, Json>) {
                return std::visit(
                    [&](const auto& json) {
                        using J = std::decay_t<decltype(json)>;
                        if constexpr (std::same_as<J, std::monostate>) {
                            return true;
                        }
                        if (*declared == DataType::Json) {
                            return true;
                        }
                        if constexpr (std::same_as<J, bool>) {
                            return *declared == DataType::Bool;
                        } else if constexpr (std::same_as<J, std::int64_t>) {
                            return *declared == DataType::I64 || *declared == DataType::F64 ||
                                   (*declared == DataType::U64 && json >= 0);
                        } else if constexpr (std::same_as<J, std::uint64_t>) {
                            return *declared == DataType::U64 || *declared == DataType::F64 ||
                                   (*declared == DataType::I64 && json <= std::uint64_t(INT64_MAX));
                        } else if constexpr (std::same_as<J, double>) {
                            return *declared == DataType::F64;
                        } else if constexpr (std::same_as<J, String>) {
                            return *declared == DataType::Text || *declared == DataType::LargeText;
                        } else {
                            return false;
                        }
                    },
                    actual.data);
            } else {
                return *declared == DataType::Json;
            }
        },
        value.data);
    if (matches) {
        return Output::ok(r, {});
    }
    String error("invalid dynamic property type: ", r);
    error += name;
    return Output::err(r, std::move(error));
}

void DynamicPropertyDefinitions::update_fingerprint() noexcept {
    detail::RustDefaultHasher hash;
    for (const auto& [name, type] : types_) {
        hash.string(name);
        hash.u64(static_cast<std::uint64_t>(type));
    }
    fingerprint_ = hash.finish();
}

Result<const DynamicPropertyDefinitions*, String> DynamicPropertyDefinitions::reusable_merge(
    Memory r, const DynamicPropertyDefinitions& left, const DynamicPropertyDefinitions& right) {
    using Output = Result<const DynamicPropertyDefinitions*, String>;
    for (const auto& [name, type] : right.types_) {
        auto previous = left.data_type(name);
        if (previous && *previous != type) {
            return Output::err(r, String(name, r));
        }
    }
    bool right_subset =
        std::all_of(right.types_.begin(), right.types_.end(), [&](const auto& item) {
            return left.types_.contains(item.first);
        });
    if (right_subset) {
        return Output::ok(r, &left);
    }
    bool left_subset = std::all_of(left.types_.begin(), left.types_.end(), [&](const auto& item) {
        return right.types_.contains(item.first);
    });
    return Output::ok(r, left_subset ? &right : nullptr);
}

Result<DynamicPropertyDefinitions, String> DynamicPropertyDefinitions::merge_copy(
    Memory r, const DynamicPropertyDefinitions& left, const DynamicPropertyDefinitions& right) {
    using Output = Result<DynamicPropertyDefinitions, String>;
    auto reuse = reusable_merge(r, left, right);
    if (!reuse) {
        return Output::err(r, std::move(reuse).error());
    }
    if (reuse.value()) {
        return Output::ok(r, detail::clone_in(r, *reuse.value()));
    }
    auto result = detail::clone_in(r, left);
    for (const auto& [name, type] : right.types_) {
        result.types_.insert_or_assign(String(name, r), type);
    }
    result.update_fingerprint();
    return Output::ok(r, std::move(result));
}
}
