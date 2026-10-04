#pragma once
#include <teaql/core/result.hpp>
#include <teaql/core/time.hpp>
#include <concepts>
#include <cstdint>
#include <optional>
#include <string_view>

namespace teaql {
inline constexpr std::string_view DEFAULT_BUSINESS_ID_PROFILE="daily-permuted-v1";
inline constexpr std::string_view LEGACY_DAILY_SEQUENCE_PROFILE="daily-sequence";
inline constexpr std::uint8_t DEFAULT_BUSINESS_ID_DIGITS=6;
inline constexpr std::uint64_t BUSINESS_ID_V1_DOMAIN_SIZE=2176782336ULL;
enum class BusinessIdErrorCode { InvalidDefinition, InvalidFormat, Immutable, Exhausted, Allocation, KeyNotFound, Encoding };
std::string_view business_id_error_code_name(BusinessIdErrorCode);
struct BusinessIdError {
    using allocator_type=Allocator;
    BusinessIdErrorCode code;
    String message;
    BusinessIdError(std::allocator_arg_t, Allocator, BusinessIdErrorCode code, std::string_view message);
    BusinessIdError(std::allocator_arg_t, Allocator, const BusinessIdError&);
    BusinessIdError(std::allocator_arg_t, Allocator a, BusinessIdError&& other) : BusinessIdError(detail::move_to(a.resource(),std::move(other))) {}
    BusinessIdError(const BusinessIdError&)=delete;
    BusinessIdError(BusinessIdError&&) noexcept=default;
    BusinessIdError& operator=(BusinessIdError&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return message.get_allocator().resource(); }
    String to_string(Memory destination) const;
    bool operator==(const BusinessIdError& other) const { return code==other.code && message==other.message; }
};
struct BusinessIdDefinition {
    using allocator_type=Allocator;
    String field_name,profile,prefix,namespace_name;
    bool split_by_date=true;
    std::uint8_t preserve_digits=DEFAULT_BUSINESS_ID_DIGITS;
    String separator;
    BusinessIdDefinition(std::allocator_arg_t, Allocator, std::string_view field, std::string_view prefix, std::string_view namespace_name);
    BusinessIdDefinition(std::allocator_arg_t, Allocator, const BusinessIdDefinition&);
    BusinessIdDefinition(std::allocator_arg_t, Allocator a, BusinessIdDefinition&& other) : BusinessIdDefinition(detail::move_to(a.resource(),std::move(other))) {}
    BusinessIdDefinition(const BusinessIdDefinition&)=delete;
    BusinessIdDefinition(BusinessIdDefinition&&) noexcept=default;
    BusinessIdDefinition& operator=(BusinessIdDefinition&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return field_name.get_allocator().resource(); }
    static BusinessIdDefinition daily_sequence(Memory, std::string_view field, std::string_view prefix, std::string_view namespace_name);
    static BusinessIdDefinition legacy_daily_sequence(Memory, std::string_view field, std::string_view prefix, std::string_view namespace_name);
    Result<std::uint64_t,BusinessIdError> max_sequence() const;
    bool operator==(const BusinessIdDefinition&) const;
};
struct BusinessIdScope {
    using allocator_type=Allocator;
    String domain_root_key,aggregate_type,namespace_name,period_key;
    BusinessIdScope(std::allocator_arg_t, Allocator, std::string_view root, std::string_view aggregate, std::string_view namespace_name, std::string_view period);
    BusinessIdScope(std::allocator_arg_t, Allocator, const BusinessIdScope&);
    BusinessIdScope(std::allocator_arg_t, Allocator a, BusinessIdScope&& other) : BusinessIdScope(detail::move_to(a.resource(),std::move(other))) {}
    BusinessIdScope(const BusinessIdScope&)=delete;
    BusinessIdScope(BusinessIdScope&&) noexcept=default;
    BusinessIdScope& operator=(BusinessIdScope&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return domain_root_key.get_allocator().resource(); }
    String canonical_key(Memory destination) const;
    bool operator==(const BusinessIdScope&) const;
};
struct BusinessIdGenerationRequest {
    using allocator_type=Allocator;
    BusinessIdDefinition definition;
    String domain_root_key,aggregate_type;
    Date business_date;
    BusinessIdGenerationRequest(std::allocator_arg_t, Allocator, const BusinessIdDefinition&, std::string_view root, std::string_view aggregate, Date);
    BusinessIdGenerationRequest(std::allocator_arg_t, Allocator, const BusinessIdGenerationRequest&);
    BusinessIdGenerationRequest(std::allocator_arg_t, Allocator a, BusinessIdGenerationRequest&& other) : BusinessIdGenerationRequest(detail::move_to(a.resource(),std::move(other))) {}
    BusinessIdGenerationRequest(const BusinessIdGenerationRequest&)=delete;
    BusinessIdGenerationRequest(BusinessIdGenerationRequest&&) noexcept=default;
    BusinessIdGenerationRequest& operator=(BusinessIdGenerationRequest&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return domain_root_key.get_allocator().resource(); }
    bool operator==(const BusinessIdGenerationRequest& other) const {
        return definition==other.definition && domain_root_key==other.domain_root_key && aggregate_type==other.aggregate_type && business_date==other.business_date;
    }
};
struct BusinessIdPlan {
    using allocator_type=Allocator;
    BusinessIdScope scope;
    String prefix;
    std::optional<String> date_text;
    std::uint8_t preserve_digits;
    String separator;
    std::uint64_t initial_sequence,max_sequence;
    BusinessIdPlan(std::allocator_arg_t, Allocator, const BusinessIdScope&, std::string_view prefix, std::optional<std::string_view> date_text, std::uint8_t digits, std::string_view separator, std::uint64_t initial, std::uint64_t maximum);
    BusinessIdPlan(std::allocator_arg_t, Allocator, const BusinessIdPlan&);
    BusinessIdPlan(std::allocator_arg_t, Allocator a, BusinessIdPlan&& other) : BusinessIdPlan(detail::move_to(a.resource(),std::move(other))) {}
    BusinessIdPlan(const BusinessIdPlan&)=delete;
    BusinessIdPlan(BusinessIdPlan&&) noexcept=default;
    BusinessIdPlan& operator=(BusinessIdPlan&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return scope.memory_resource(); }
    Result<std::monostate,BusinessIdError> validate_allocation_range() const;
    bool operator==(const BusinessIdPlan&) const;
};
struct BusinessIdAllocation {
    using allocator_type=Allocator;
    BusinessIdScope scope;
    std::uint64_t sequence;
    BusinessIdAllocation(std::allocator_arg_t, Allocator, const BusinessIdScope&, std::uint64_t sequence);
    BusinessIdAllocation(std::allocator_arg_t, Allocator, const BusinessIdAllocation&);
    BusinessIdAllocation(std::allocator_arg_t, Allocator a, BusinessIdAllocation&& other) : BusinessIdAllocation(detail::move_to(a.resource(),std::move(other))) {}
    BusinessIdAllocation(const BusinessIdAllocation&)=delete;
    BusinessIdAllocation(BusinessIdAllocation&&) noexcept=default;
    BusinessIdAllocation& operator=(BusinessIdAllocation&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return scope.memory_resource(); }
    bool operator==(const BusinessIdAllocation& other) const { return scope==other.scope && sequence==other.sequence; }
};
struct BusinessIdValue {
    using allocator_type=Allocator;
    String value;
    BusinessIdValue(std::allocator_arg_t, Allocator, std::string_view value);
    BusinessIdValue(std::allocator_arg_t, Allocator, const BusinessIdValue&);
    BusinessIdValue(std::allocator_arg_t, Allocator a, BusinessIdValue&& other) : BusinessIdValue(detail::move_to(a.resource(),std::move(other))) {}
    BusinessIdValue(const BusinessIdValue&)=delete;
    BusinessIdValue(BusinessIdValue&&) noexcept=default;
    BusinessIdValue& operator=(BusinessIdValue&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return value.get_allocator().resource(); }
    bool operator==(const BusinessIdValue& other) const { return value==other.value; }
};
// Implementations have Rust Send+Sync's shared-service responsibility: calls may
// come from different UserContexts. Result allocation uses the caller's domain.
class BusinessIdAllocator {
public:
    virtual ~BusinessIdAllocator()=default;
    virtual Result<BusinessIdAllocation,BusinessIdError> allocate(Memory destination,const BusinessIdPlan&) const=0;
};
class BusinessIdProfile {
public:
    virtual ~BusinessIdProfile()=default;
    virtual Result<BusinessIdPlan,BusinessIdError> plan(Memory destination,const BusinessIdGenerationRequest&) const=0;
    virtual Result<BusinessIdValue,BusinessIdError> format(Memory destination,const BusinessIdPlan&,const BusinessIdAllocation&) const=0;
    virtual Result<BusinessIdValue,BusinessIdError> validate(Memory destination,const BusinessIdDefinition&,std::string_view value) const=0;
};
template<class T> concept BusinessIdSlot = requires(T& slot,const T& value,BusinessIdValue id) {
    { value.current_business_id() } -> std::same_as<std::optional<std::string_view>>;
    { value.is_new_aggregate() } -> std::same_as<bool>;
    { slot.assign_business_id(std::move(id)) } -> std::same_as<void>;
};
}
