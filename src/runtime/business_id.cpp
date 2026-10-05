#include <teaql/runtime/business_id.hpp>
#include <teaql/core/detail/sha256.hpp>
#include <teaql/core/detail/text.hpp>
#include <algorithm>
#include <bit>
#include <charconv>
#include <limits>
#include <span>

namespace teaql {
namespace {
using detail::Sha256;

std::array<std::uint8_t, 32> hmac(const std::array<std::uint8_t, 32>& key,
                                  std::string_view tweak,
                                  std::array<std::uint8_t, 3> suffix) {
    std::array<std::uint8_t, 64> pad{};
    pad.fill(0x36);
    for (unsigned i = 0; i < 32; ++i) {
        pad[i] ^= key[i];
    }
    Sha256 inner;
    inner.update(pad);
    inner.update({reinterpret_cast<const std::uint8_t*>(tweak.data()), tweak.size()});
    inner.update(suffix);
    auto digest = inner.finish();
    pad.fill(0x5c);
    for (unsigned i = 0; i < 32; ++i) {
        pad[i] ^= key[i];
    }
    Sha256 outer;
    outer.update(pad);
    outer.update(digest);
    return outer.finish();
}

template <class T>
Result<T, BusinessIdError> fail(Memory r, BusinessIdErrorCode code, std::string_view message) {
    return Result<T, BusinessIdError>::err(
        r, BusinessIdError(std::allocator_arg, Allocator(r), code, message));
}

bool blank(std::string_view text) {
    detail::trim(text);
    return text.empty();
}

void append_number(String& out, std::uint64_t value, unsigned width = 0) {
    char buffer[32];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    auto size = std::size_t(result.ptr - buffer);
    if (width > size) {
        out.append(width - size, '0');
    }
    out.append(buffer, result.ptr);
}

String date_text(Memory r, Date date) {
    String out(r);
    auto year = date.year();
    if (year < 0) {
        out += '-';
        append_number(out, std::uint64_t(-std::int64_t(year)), 4);
    } else {
        if (year > 9999) {
            out += '+';
        }
        append_number(out, std::uint64_t(year), 4);
    }
    append_number(out, date.month(), 2);
    append_number(out, date.day(), 2);
    return out;
}

Vector<std::string_view> split(Memory r, std::string_view text, std::string_view separator) {
    Vector<std::string_view> parts(r);
    if (separator.empty()) {
        parts.emplace_back();
        for (std::size_t i = 0; i < text.size();) {
            auto start = i++;
            while (i < text.size() && (static_cast<unsigned char>(text[i]) & 0xc0) == 0x80) {
                ++i;
            }
            parts.push_back(text.substr(start, i - start));
        }
        parts.emplace_back();
        return parts;
    }
    std::size_t start = 0;
    for (;;) {
        auto end = text.find(separator, start);
        if (end == text.npos) {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, end - start));
        start = end + separator.size();
    }
    return parts;
}

bool digits(std::string_view text) {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return c >= '0' && c <= '9';
    });
}

bool compact_date(std::string_view text) {
    auto parse = [&](std::size_t max, bool trim_space) -> std::optional<unsigned> {
        if (trim_space) {
            detail::trim_start(text);
        }
        unsigned value = 0;
        std::size_t count = 0;
        while (count < max && !text.empty() && text.front() >= '0' && text.front() <= '9') {
            auto digit = unsigned(text.front() - '0');
            if (value > (unsigned(INT32_MAX) - digit) / 10) {
                return {};
            }
            value = value * 10 + digit;
            text.remove_prefix(1);
            ++count;
        }
        return count ? std::optional<unsigned>(value) : std::nullopt;
    };
    // Chrono consumes all digits after a sign, but at most four without one.
    // Whitespace is accepted before each numeric field, never after its sign.
    detail::trim_start(text);
    bool signed_year = !text.empty() && (text.front() == '+' || text.front() == '-');
    bool negative = !text.empty() && text.front() == '-';
    if (signed_year) {
        text.remove_prefix(1);
    }
    auto year = parse(signed_year ? std::numeric_limits<std::size_t>::max() : 4, false);
    auto month = parse(2, true);
    auto day = parse(2, true);
    return year && month && day && text.empty() &&
           Date::from_ymd(negative ? -std::int32_t(*year) : std::int32_t(*year), *month, *day)
               .has_value();
}

String invalid_message(Memory r, std::string_view prefix, std::string_view value) {
    String out(prefix, r);
    out += value;
    return out;
}

Result<BusinessIdValue, BusinessIdError> id_value(Memory r, std::string_view value) {
    return Result<BusinessIdValue, BusinessIdError>::ok(
        r, BusinessIdValue(std::allocator_arg, Allocator(r), value));
}
}

Result<BusinessIdEncodingKey, BusinessIdError>
BusinessIdEncodingKey::create(Memory r, std::uint32_t version, std::array<std::uint8_t, 32> bytes) {
    if (!version) {
        return fail<BusinessIdEncodingKey>(
            r, BusinessIdErrorCode::InvalidDefinition, "business ID key version must be positive");
    }
    return Result<BusinessIdEncodingKey, BusinessIdError>::ok(
        r, BusinessIdEncodingKey(version, bytes));
}

Result<String, BusinessIdError> BusinessIdPermutationV1::encode(Memory r,
                                                                std::uint64_t sequence,
                                                                const BusinessIdScope& scope,
                                                                const BusinessIdEncodingKey& key) {
    if (sequence >= BUSINESS_ID_V1_DOMAIN_SIZE) {
        return fail<String>(
            r, BusinessIdErrorCode::Exhausted, "business ID V1 sequence must be in 0..2176782335");
    }
    std::array<std::string_view, 4> fields{
        scope.domain_root_key, scope.aggregate_type, scope.namespace_name, scope.period_key};
    for (auto field : fields) {
        if (blank(field)) {
            return fail<String>(r,
                                BusinessIdErrorCode::InvalidDefinition,
                                "business ID V1 scope fields must not be blank");
        }
    }
    String tweak("teaql-business-id-fp-v1", r);
    tweak += '\0';
    tweak += '\1';
    auto append32 = [&](std::uint32_t n) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            tweak += char(n >> shift);
        }
    };
    append32(key.version_);
    for (auto field : fields) {
        if (field.size() > UINT32_MAX) {
            return fail<String>(r,
                                BusinessIdErrorCode::InvalidDefinition,
                                "business ID V1 scope field is too long");
        }
        append32(std::uint32_t(field.size()));
        tweak += field;
    }
    auto candidate = std::uint32_t(sequence);
    do {
        auto left = std::uint16_t(candidate >> 16), right = std::uint16_t(candidate);
        for (std::uint8_t round = 0; round < 8; ++round) {
            auto digest =
                hmac(key.bytes_, tweak, {round, std::uint8_t(right >> 8), std::uint8_t(right)});
            auto next = std::uint16_t(left ^ (std::uint16_t(digest[0]) << 8) ^ digest[1]);
            left = right;
            right = next;
        }
        candidate = (std::uint32_t(left) << 16) | right;
    } while (candidate >= BUSINESS_ID_V1_DOMAIN_SIZE);
    String out(BUSINESS_ID_V1_WIDTH, '0', r);
    for (std::size_t i = out.size(); i > 0;) {
        out[--i] = BUSINESS_ID_V1_ALPHABET[candidate % 36];
        candidate /= 36;
    }
    return Result<String, BusinessIdError>::ok(r, std::move(out));
}

Result<BusinessIdPlan, BusinessIdError>
DailySequenceBusinessIdProfile::plan(Memory r, const BusinessIdGenerationRequest& request) const {
    const auto& d = request.definition;
    if (d.profile != LEGACY_DAILY_SEQUENCE_PROFILE) {
        return fail<BusinessIdPlan>(
            r,
            BusinessIdErrorCode::InvalidDefinition,
            invalid_message(r, "unsupported business ID profile ", d.profile));
    }
    if (blank(d.prefix) || blank(d.namespace_name) || d.separator.empty()) {
        return fail<BusinessIdPlan>(r,
                                    BusinessIdErrorCode::InvalidDefinition,
                                    "prefix, namespace and separator must not be blank");
    }
    auto maximum = d.max_sequence();
    if (!maximum) {
        return Result<BusinessIdPlan, BusinessIdError>::err(r,
                                                            detail::clone_in(r, maximum.error()));
    }
    std::optional<String> date;
    if (d.split_by_date) {
        date = date_text(r, request.business_date);
    }
    BusinessIdScope scope(std::allocator_arg,
                          Allocator(r),
                          request.domain_root_key,
                          request.aggregate_type,
                          d.namespace_name,
                          date ? std::string_view(*date) : std::string_view{});
    return Result<BusinessIdPlan, BusinessIdError>::ok(
        r,
        BusinessIdPlan(std::allocator_arg,
                       Allocator(r),
                       scope,
                       d.prefix,
                       date ? std::optional<std::string_view>(*date) : std::nullopt,
                       d.preserve_digits,
                       d.separator,
                       1,
                       maximum.value()));
}

Result<BusinessIdValue, BusinessIdError> DailySequenceBusinessIdProfile::format(
    Memory r, const BusinessIdPlan& plan, const BusinessIdAllocation& allocation) const {
    if (allocation.scope != plan.scope || allocation.sequence > plan.max_sequence) {
        return fail<BusinessIdValue>(
            r,
            BusinessIdErrorCode::Exhausted,
            "business ID allocation does not belong to the plan or exceeds its range");
    }
    String out(plan.prefix, r);
    out += plan.separator;
    if (plan.date_text) {
        out += *plan.date_text;
        out += plan.separator;
    }
    append_number(out, allocation.sequence, plan.preserve_digits);
    return id_value(r, out);
}

Result<BusinessIdValue, BusinessIdError> DailySequenceBusinessIdProfile::validate(
    Memory r, const BusinessIdDefinition& d, std::string_view value) const {
    if (d.profile != LEGACY_DAILY_SEQUENCE_PROFILE || blank(d.prefix) || blank(d.namespace_name) ||
        d.separator.empty()) {
        return fail<BusinessIdValue>(r,
                                     BusinessIdErrorCode::InvalidDefinition,
                                     "invalid daily-sequence Business ID definition");
    }
    auto parts = split(r, value, d.separator);
    bool valid = parts.size() == (d.split_by_date ? 3u : 2u) && parts[0] == d.prefix;
    if (valid && d.split_by_date) {
        valid = parts[1].size() == 8 && digits(parts[1]) && compact_date(parts[1]);
    }
    if (valid) {
        valid = parts.back().size() == d.preserve_digits && digits(parts.back());
    }
    if (!valid) {
        return fail<BusinessIdValue>(
            r,
            BusinessIdErrorCode::InvalidFormat,
            invalid_message(r, "invalid daily-sequence Business ID: ", value));
    }
    return id_value(r, value);
}

Result<BusinessIdPlan, BusinessIdError>
PermutedDailyBusinessIdProfile::plan(Memory r, const BusinessIdGenerationRequest& request) const {
    const auto& d = request.definition;
    if (d.profile != DEFAULT_BUSINESS_ID_PROFILE || !d.split_by_date ||
        d.preserve_digits != BUSINESS_ID_V1_WIDTH) {
        return fail<BusinessIdPlan>(
            r,
            BusinessIdErrorCode::InvalidDefinition,
            "daily-permuted-v1 requires split_by_date=true and preserve_digits=6");
    }
    if (blank(d.prefix) || blank(d.namespace_name) || d.separator.empty()) {
        return fail<BusinessIdPlan>(r,
                                    BusinessIdErrorCode::InvalidDefinition,
                                    "prefix, namespace and separator must not be blank");
    }
    auto date = date_text(r, request.business_date);
    BusinessIdScope scope(std::allocator_arg,
                          Allocator(r),
                          request.domain_root_key,
                          request.aggregate_type,
                          d.namespace_name,
                          date);
    return Result<BusinessIdPlan, BusinessIdError>::ok(
        r,
        BusinessIdPlan(std::allocator_arg,
                       Allocator(r),
                       scope,
                       d.prefix,
                       std::string_view(date),
                       d.preserve_digits,
                       d.separator,
                       0,
                       BUSINESS_ID_V1_DOMAIN_SIZE - 1));
}

Result<BusinessIdValue, BusinessIdError> PermutedDailyBusinessIdProfile::format(
    Memory r, const BusinessIdPlan& plan, const BusinessIdAllocation& allocation) const {
    if (allocation.scope != plan.scope || allocation.sequence > plan.max_sequence) {
        return fail<BusinessIdValue>(
            r,
            BusinessIdErrorCode::Exhausted,
            "business ID allocation does not belong to the V1 plan or exceeds its range");
    }
    auto code = BusinessIdPermutationV1::encode(r, allocation.sequence, plan.scope, key_);
    if (!code) {
        return Result<BusinessIdValue, BusinessIdError>::err(r, detail::clone_in(r, code.error()));
    }
    if (!plan.date_text) {
        return fail<BusinessIdValue>(r,
                                     BusinessIdErrorCode::InvalidDefinition,
                                     "daily-permuted-v1 plan is missing its period key");
    }
    String out(plan.prefix, r);
    out += plan.separator;
    out += *plan.date_text;
    out += plan.separator;
    out += code.value();
    return id_value(r, out);
}

Result<BusinessIdValue, BusinessIdError> PermutedDailyBusinessIdProfile::validate(
    Memory r, const BusinessIdDefinition& d, std::string_view value) const {
    auto parts = split(r, value, d.separator);
    bool valid = d.profile == DEFAULT_BUSINESS_ID_PROFILE && parts.size() == 3 &&
                 parts[0] == d.prefix && compact_date(parts[1]) &&
                 parts[2].size() == BUSINESS_ID_V1_WIDTH;
    if (valid) {
        valid = std::all_of(parts[2].begin(), parts[2].end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z');
        });
    }
    if (!valid) {
        return fail<BusinessIdValue>(
            r,
            BusinessIdErrorCode::InvalidFormat,
            invalid_message(r, "invalid daily-permuted-v1 Business ID: ", value));
    }
    return id_value(r, value);
}
}
