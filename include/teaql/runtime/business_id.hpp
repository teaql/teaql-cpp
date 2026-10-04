#pragma once
#include <teaql/core/business_id.hpp>
#include <teaql/runtime/resources.hpp>
#include <teaql/memory/function_ref.hpp>
#include <array>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace teaql {
class UserContext;
inline constexpr std::string_view BUSINESS_ID_V1_ALPHABET="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
inline constexpr std::size_t BUSINESS_ID_V1_WIDTH=6;
class BusinessIdEncodingKey {
public:
    static Result<BusinessIdEncodingKey,BusinessIdError> create(Memory,std::uint32_t,std::array<std::uint8_t,32>);
    std::uint32_t version() const noexcept{return version_;}
    bool operator==(const BusinessIdEncodingKey&) const=default;
private:
    friend class BusinessIdPermutationV1;
    BusinessIdEncodingKey(std::uint32_t version,std::array<std::uint8_t,32> bytes):version_(version),bytes_(bytes){}
    std::uint32_t version_;
    std::array<std::uint8_t,32> bytes_;
};
// Registration is passive; the Context schema pipeline invokes this hook.
class BusinessIdSchemaContributor {
public:
    virtual ~BusinessIdSchemaContributor()=default;
    virtual Result<std::monostate,BusinessIdError> ensure_schema(Memory,const UserContext&) const=0;
};
class BusinessIdKeyProvider {
public:
    virtual ~BusinessIdKeyProvider()=default;
    virtual Result<BusinessIdEncodingKey,BusinessIdError> current_key(Memory,const UserContext&,const BusinessIdDefinition&) const=0;
};
class StaticBusinessIdKeyProvider final:public BusinessIdKeyProvider {
public:
    using allocator_type=Allocator;
    explicit StaticBusinessIdKeyProvider(BusinessIdEncodingKey key):key_(key){}
    StaticBusinessIdKeyProvider(std::allocator_arg_t,Allocator,BusinessIdEncodingKey key):key_(key){}
    StaticBusinessIdKeyProvider(std::allocator_arg_t,Allocator,const StaticBusinessIdKeyProvider& other):key_(other.key_){}
    Result<BusinessIdEncodingKey,BusinessIdError> current_key(Memory r,const UserContext&,const BusinessIdDefinition&) const override{return Result<BusinessIdEncodingKey,BusinessIdError>::ok(r,key_);}
private:
    BusinessIdEncodingKey key_;
};
class BusinessIdPermutationV1 {
public:
    static Result<String,BusinessIdError> encode(Memory,std::uint64_t,const BusinessIdScope&,const BusinessIdEncodingKey&);
};
class DailySequenceBusinessIdProfile final:public BusinessIdProfile {
public:
    using allocator_type=Allocator;
    DailySequenceBusinessIdProfile()=default;
    DailySequenceBusinessIdProfile(std::allocator_arg_t,Allocator){}
    DailySequenceBusinessIdProfile(std::allocator_arg_t,Allocator,const DailySequenceBusinessIdProfile&){}
    Result<BusinessIdPlan,BusinessIdError> plan(Memory,const BusinessIdGenerationRequest&) const override;
    Result<BusinessIdValue,BusinessIdError> format(Memory,const BusinessIdPlan&,const BusinessIdAllocation&) const override;
    Result<BusinessIdValue,BusinessIdError> validate(Memory,const BusinessIdDefinition&,std::string_view) const override;
};
class PermutedDailyBusinessIdProfile final:public BusinessIdProfile {
public:
    using allocator_type=Allocator;
    explicit PermutedDailyBusinessIdProfile(BusinessIdEncodingKey key):key_(key){}
    PermutedDailyBusinessIdProfile(std::allocator_arg_t,Allocator,BusinessIdEncodingKey key):key_(key){}
    PermutedDailyBusinessIdProfile(std::allocator_arg_t,Allocator,const PermutedDailyBusinessIdProfile& other):key_(other.key_){}
    Result<BusinessIdPlan,BusinessIdError> plan(Memory,const BusinessIdGenerationRequest&) const override;
    Result<BusinessIdValue,BusinessIdError> format(Memory,const BusinessIdPlan&,const BusinessIdAllocation&) const override;
    Result<BusinessIdValue,BusinessIdError> validate(Memory,const BusinessIdDefinition&,std::string_view) const override;
private:
    BusinessIdEncodingKey key_;
};
// Like other Context-backed providers, calls execute on the allocation owner's
// thread. The mutex retains the source's critical-section and poisoning boundary.
class InMemoryBusinessIdAllocator final:public BusinessIdAllocator {
public:
    using allocator_type=Allocator;
    InMemoryBusinessIdAllocator(std::allocator_arg_t,Allocator a):resource_(a.resource()),levels_(0,ScopeHash{},std::equal_to<>{},resource_),owner_(std::this_thread::get_id()){}
    InMemoryBusinessIdAllocator(const InMemoryBusinessIdAllocator&)=delete;
    Memory memory_resource() const noexcept{return resource_;}
    Result<BusinessIdAllocation,BusinessIdError> allocate(Memory,const BusinessIdPlan&) const override;
private:
    Memory resource_;
    struct ScopeHash {using is_transparent=void;std::size_t operator()(std::string_view value) const noexcept{return std::hash<std::string_view>{}(value);}};
    mutable std::pmr::unordered_map<String,std::uint64_t,ScopeHash,std::equal_to<>> levels_;
    mutable std::mutex mutex_;
    mutable bool poisoned_=false;
    std::thread::id owner_;
};
class BusinessIdService {
public:
    using allocator_type=Allocator;
    template<class Owner,class T> requires std::derived_from<T,BusinessIdAllocator>
    BusinessIdService(std::allocator_arg_t,Allocator a,Owner& origin,T& allocator)
        :allocator_(std::allocator_arg,a,static_cast<const BusinessIdAllocator&>(allocator),*detail::ContextResources::entry(detail::resource_arena(a.resource()),origin.memory_resource(),allocator).owner){}
    BusinessIdService(std::allocator_arg_t,Allocator a,const BusinessIdService& other)
        :allocator_(std::allocator_arg,a,other.allocator_),key_provider_(detail::clone_in(a.resource(),other.key_provider_)){}
    BusinessIdService(std::allocator_arg_t,Allocator a,BusinessIdService&& other):BusinessIdService(std::allocator_arg,a,std::as_const(other)){}
    BusinessIdService(const BusinessIdService&)=delete;
    BusinessIdService(BusinessIdService&&) noexcept=default;
    Memory memory_resource() const noexcept{return allocator_.memory_resource();}
    template<class Owner,class T> requires std::derived_from<T,BusinessIdKeyProvider>
    BusinessIdService& with_key_provider(Owner& origin,T& provider) & {
        auto entry=detail::ContextResources::entry(detail::resource_arena(memory_resource()),origin.memory_resource(),provider);
        key_provider_.emplace(std::allocator_arg,Allocator(memory_resource()),static_cast<const BusinessIdKeyProvider&>(provider),*entry.owner);return *this;
    }
    template<class Owner,class T> requires std::derived_from<T,BusinessIdKeyProvider>
    BusinessIdService&& with_key_provider(Owner& origin,T& provider) && {with_key_provider(origin,provider);return std::move(*this);}
    template<BusinessIdSlot S> Result<BusinessIdValue,BusinessIdError> ensure(Memory r,const UserContext& context,const BusinessIdDefinition& definition,std::string_view root,std::string_view aggregate,S& slot) const {
        auto current=[&]{return std::as_const(slot).current_business_id();};auto fresh=[&]{return std::as_const(slot).is_new_aggregate();};auto assign=[&](BusinessIdValue value){slot.assign_business_id(std::move(value));};
        return ensure_value(r,context,definition,root,aggregate,current,fresh,assign);
    }
private:
    Result<BusinessIdValue,BusinessIdError> ensure_value(Memory,const UserContext&,const BusinessIdDefinition&,std::string_view,std::string_view,FunctionRef<std::optional<std::string_view>()>,FunctionRef<bool()>,FunctionRef<void(BusinessIdValue)>) const;
    ResourceBorrow<const BusinessIdAllocator> allocator_;
    std::optional<ResourceBorrow<const BusinessIdKeyProvider>> key_provider_;
};
namespace detail {
class BusinessIdSchemaService {
public:
    using allocator_type=Allocator;
    template<class Owner,class T> requires std::derived_from<T,BusinessIdSchemaContributor>
    BusinessIdSchemaService(std::allocator_arg_t,Allocator a,Owner& origin,T& contributor)
        :contributor_(std::allocator_arg,a,static_cast<const BusinessIdSchemaContributor&>(contributor),*ContextResources::entry(resource_arena(a.resource()),origin.memory_resource(),contributor).owner){}
    BusinessIdSchemaService(std::allocator_arg_t,Allocator a,const BusinessIdSchemaService& other):contributor_(std::allocator_arg,a,other.contributor_){}
    BusinessIdSchemaService(const BusinessIdSchemaService&)=delete;
    BusinessIdSchemaService(BusinessIdSchemaService&&) noexcept=default;
    Memory memory_resource() const noexcept{return contributor_.memory_resource();}
    Result<std::monostate,BusinessIdError> ensure_schema(Memory r,const UserContext& context) const{return contributor_.get().ensure_schema(r,context);}
private:
    ResourceBorrow<const BusinessIdSchemaContributor> contributor_;
};
}

}
