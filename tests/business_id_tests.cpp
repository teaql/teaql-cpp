#include <teaql/core/business_id.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>
using namespace teaql;
static void check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
struct NoDefaultAllocator {
    Memory previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());
    ~NoDefaultAllocator() { std::pmr::set_default_resource(previous); }
};
struct Slot {
    using allocator_type=Allocator;
    Memory resource;std::optional<String> id;bool is_new=true;
    Slot(std::allocator_arg_t,Allocator a) : resource(a.resource()) {}
    std::optional<std::string_view> current_business_id() const { return id ? std::optional<std::string_view>(*id) : std::nullopt; }
    bool is_new_aggregate() const { return is_new; }
    void assign_business_id(BusinessIdValue value) { id.emplace(value.value,resource); }
};
static_assert(BusinessIdSlot<Slot>);
static_assert(!BusinessIdSlot<BusinessIdValue>);
// Test doubles verify separate virtual interfaces and caller-selected output domains.
struct AllocatorProbe final : BusinessIdAllocator {
    Result<BusinessIdAllocation,BusinessIdError> allocate(Memory r,const BusinessIdPlan& plan) const override {
        return Result<BusinessIdAllocation,BusinessIdError>::ok(r,BusinessIdAllocation(std::allocator_arg,Allocator(r),plan.scope,plan.initial_sequence));
    }
};
struct ProfileProbe final : BusinessIdProfile {
    Result<BusinessIdPlan,BusinessIdError> plan(Memory r,const BusinessIdGenerationRequest& request) const override {
        BusinessIdScope scope(std::allocator_arg,Allocator(r),request.domain_root_key,request.aggregate_type,request.definition.namespace_name,"");
        BusinessIdPlan plan(std::allocator_arg,Allocator(r),scope,request.definition.prefix,std::nullopt,6,"-",0,123);
        return Result<BusinessIdPlan,BusinessIdError>::ok(r,std::move(plan));
    }
    Result<BusinessIdValue,BusinessIdError> format(Memory r,const BusinessIdPlan& plan,const BusinessIdAllocation&) const override {
        return Result<BusinessIdValue,BusinessIdError>::ok(r,BusinessIdValue(std::allocator_arg,Allocator(r),plan.prefix));
    }
    Result<BusinessIdValue,BusinessIdError> validate(Memory r,const BusinessIdDefinition&,std::string_view) const override {
        return Result<BusinessIdValue,BusinessIdError>::err(r,BusinessIdError(std::allocator_arg,Allocator(r),BusinessIdErrorCode::InvalidFormat,"test validation error requiring allocated storage"));
    }
};
static void contracts_and_lifetimes() {
    NoDefaultAllocator guard;GlobalContext global;auto user=global.make_user_context();auto r=user->memory_resource(),g=global.memory_resource();
    BusinessIdGenerationRequest* request=nullptr;BusinessIdPlan* plan=nullptr;BusinessIdAllocation* allocation=nullptr;
    Result<BusinessIdValue,BusinessIdError>* error=nullptr;Slot* slot=nullptr;
    {
        auto definition=BusinessIdDefinition::daily_sequence(r,"business_id","a long prefix requiring allocation","namespace with long allocated storage");
        check(definition.profile==DEFAULT_BUSINESS_ID_PROFILE && definition.preserve_digits==6 && definition.split_by_date && definition.separator=="-","daily_sequence selects permuted v1 defaults");
        check(definition.max_sequence().value()==BUSINESS_ID_V1_DOMAIN_SIZE-1,"permuted sequence max is domain size minus one");
        auto legacy=BusinessIdDefinition::legacy_daily_sequence(r,"business_id","P","orders");
        check(legacy.profile==LEGACY_DAILY_SEQUENCE_PROFILE && legacy.preserve_digits==8 && legacy.max_sequence().value()==99999999,"legacy defaults stay distinct");
        legacy.profile="a not-yet-registered profile";legacy.preserve_digits=19;
        check(legacy.max_sequence().value()==9999999999999999999ULL,"max_sequence does not reject unknown non-default profiles");
        definition.preserve_digits=8;auto invalid=definition.max_sequence();
        check(!invalid && invalid.error().code==BusinessIdErrorCode::InvalidDefinition,"permuted width must be exactly six");definition.preserve_digits=6;
        auto& source_request=user->make<BusinessIdGenerationRequest>(definition,"long domain root key with storage","long aggregate type requiring storage",*Date::from_ymd(2024,2,29));
        definition.prefix="changed after request creation";
        check(source_request.definition.prefix.starts_with("a long prefix"),"managed request owns an explicit snapshot instead of a dangling foreign definition");
        request=&global.make<BusinessIdGenerationRequest>(source_request);
        auto& scope=user->make<BusinessIdScope>("long domain|key%7C","long aggregate %25|","namespace with long allocated storage","20240229");
        check(scope.canonical_key(r)=="long domain%7Ckey%257C|long aggregate %2525%7C|namespace with long allocated storage|20240229","canonical key escapes original percent before separators");
        auto& source_plan=user->make<BusinessIdPlan>(scope,"long prefix requiring storage",std::string_view("date text requiring separate storage"),6,"-",0,0);
        check(bool(source_plan.validate_allocation_range()),"zero-sized inclusive sequence range is valid");
        plan=&global.make<BusinessIdPlan>(source_plan);
        auto& assigned=global.make<BusinessIdPlan>(scope,"before",std::nullopt,1,"/",1,1);assigned=std::move(source_plan);
        check(assigned==*plan && source_plan.date_text.has_value(),"cross-domain plan move clones all data and leaves source available");
        source_plan.initial_sequence=1;auto range_error=source_plan.validate_allocation_range();
        check(!range_error && range_error.error().message.starts_with("business ID allocation range"),"range validation rejects initial beyond maximum");
        auto& none=user->make<BusinessIdPlan>(scope,"P",std::nullopt,0,"",0,UINT64_MAX);
        auto& empty=user->make<BusinessIdPlan>(scope,"P",std::string_view(""),0,"",0,UINT64_MAX);
        check(!(none==empty) && bool(none.validate_allocation_range()),"None and Some(empty) dates remain distinct and range validation has no extra definition checks");
        auto& source_allocation=user->make<BusinessIdAllocation>(scope,UINT64_MAX);
        allocation=&global.make<BusinessIdAllocation>(source_allocation);
        ProfileProbe concrete_profile;AllocatorProbe concrete_allocator;
        const BusinessIdProfile& profile=concrete_profile;const BusinessIdAllocator& allocator=concrete_allocator;
        auto profiled=profile.plan(g,source_request);auto allocated=allocator.allocate(g,profiled.value());
        check(profiled.value().memory_resource()==g && allocated.value().memory_resource()==g,"profile and allocator results independently target caller domain");
        auto formatted=profile.format(r,profiled.value(),allocated.value());
        slot=&global.make<Slot>();check(!slot->current_business_id() && slot->is_new_aggregate(),"slot borrowed optional ID and new aggregate state");
        slot->assign_business_id(std::move(formatted).value());
        auto rejected=profile.validate(r,definition,"invalid");error=&global.make<Result<BusinessIdValue,BusinessIdError>>(rejected);
    }
    user->close();
    check(request->definition.memory_resource()==g && request->definition.prefix.starts_with("a long prefix"),"request definition survives source close in destination domain");
    check(plan->scope.namespace_name.get_allocator().resource()==g && plan->date_text->get_allocator().resource()==g,"plan nested scope and optional text deep copy");
    check(allocation->sequence==UINT64_MAX && allocation->scope.canonical_key(g).find("%257C")!=String::npos,"allocation retains scope independently");
    check(slot->id->get_allocator().resource()==g && slot->current_business_id()->starts_with("a long prefix"),"slot assignment explicitly retains value in slot domain");
    check(error->error().to_string(g)=="InvalidFormat: test validation error requiring allocated storage","typed errors retain exact display and destination lifetime");
}
struct FailingResource : std::pmr::memory_resource {
    bool fail=false;std::size_t bytes=0;
    void* do_allocate(std::size_t n,std::size_t align) override { if(fail) throw std::bad_alloc();auto p=std::pmr::new_delete_resource()->allocate(n,align);bytes+=n;return p; }
    void do_deallocate(void* p,std::size_t n,std::size_t align) override { bytes-=n;std::pmr::new_delete_resource()->deallocate(p,n,align); }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this==&other; }
};
static void failed_copy() {
    NoDefaultAllocator guard;FailingResource upstream;
    {
        GlobalContext source;GlobalContext destination(&upstream,128);auto r=source.memory_resource();
        auto& long_name=source.make<String>(128*1024,'x');
        auto& large=source.make<BusinessIdDefinition>(long_name,"P","N");
        auto& small=destination.make<BusinessIdDefinition>("retained","P","N");
        upstream.fail=true;bool rejected=false;
        try { small=std::move(large); } catch(const std::bad_alloc&) { rejected=true; }
        check(rejected && small.field_name=="retained" && large.field_name.size()==long_name.size(),"failed cross-domain assignment preserves both definitions");
        upstream.fail=false;
        auto key=BusinessIdScope(std::allocator_arg,Allocator(r),long_name,long_name,long_name,long_name).canonical_key(r);
        check(key.size()==4*long_name.size()+3,"large canonical key uses explicit arena growth");
    }
    check(upstream.bytes==0,"all failed-copy destination blocks reclaimed");
}
int main() { try { contracts_and_lifetimes();failed_copy();std::cout<<"Business ID core contracts passed\n"; } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; } }
