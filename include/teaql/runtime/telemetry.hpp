#pragma once
#include <teaql/runtime/resources.hpp>
#include <teaql/async/operation.hpp>
#include <teaql/memory/context_box.hpp>
#include <teaql/memory/function_ref.hpp>
#include <teaql/memory/scoped.hpp>
#include <bit>
namespace teaql {
struct RuntimeAttributeValue {
    using allocator_type=Allocator;using Data=std::variant<String,std::int64_t,double,bool>;
    Data data;Memory resource_;
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,std::string_view v):data(String(v,a.resource())),resource_(a.resource()){}
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,const char* v):RuntimeAttributeValue(std::allocator_arg,a,std::string_view(v)){}
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,std::int64_t v):data(v),resource_(a.resource()){}
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,std::uint64_t v):data(std::bit_cast<std::int64_t>(v)),resource_(a.resource()){}
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,double v):data(v),resource_(a.resource()){}
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,bool v):data(v),resource_(a.resource()){}
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,const RuntimeAttributeValue& other):data(std::visit([&](const auto& v)->Data{using V=std::decay_t<decltype(v)>;if constexpr(std::same_as<V,String>)return String(v,a.resource());else return v;},other.data)),resource_(a.resource()){}
    RuntimeAttributeValue(std::allocator_arg_t,Allocator a,RuntimeAttributeValue&& other):RuntimeAttributeValue(detail::move_to(a.resource(),std::move(other))){}
    RuntimeAttributeValue(const RuntimeAttributeValue&)=delete;RuntimeAttributeValue(RuntimeAttributeValue&&) noexcept=default;
    RuntimeAttributeValue& operator=(RuntimeAttributeValue&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource() const noexcept{return resource_;}
    bool operator==(const RuntimeAttributeValue& other) const{return data==other.data;}
};
using RuntimeAttributes=std::pmr::map<String,RuntimeAttributeValue,std::less<>>;
using RuntimeContextCarrier=std::pmr::map<String,String,std::less<>>;
struct RuntimeOperation {
    using allocator_type=Allocator;String family,name;RuntimeAttributes attributes;
    RuntimeOperation(std::allocator_arg_t,Allocator,std::string_view family,std::string_view name);
    RuntimeOperation(std::allocator_arg_t,Allocator a,const RuntimeOperation& other):family(other.family,a.resource()),name(other.name,a.resource()),attributes(other.attributes,a.resource()){}
    RuntimeOperation(std::allocator_arg_t,Allocator a,RuntimeOperation&& other):RuntimeOperation(detail::move_to(a.resource(),std::move(other))){}
    RuntimeOperation(const RuntimeOperation&)=delete;RuntimeOperation(RuntimeOperation&&) noexcept=default;
    RuntimeOperation& operator=(RuntimeOperation&& other){return detail::assign_scoped(*this,std::move(other));}
    RuntimeOperation& attribute(std::string_view,const RuntimeAttributeValue&);
    template<class T> RuntimeOperation& attribute(std::string_view key,T&& value){return attribute(key,static_cast<const RuntimeAttributeValue&>(std::make_obj_using_allocator<RuntimeAttributeValue>(Allocator(memory_resource()),std::forward<T>(value))));}
    Memory memory_resource() const noexcept{return family.get_allocator().resource();}
    bool operator==(const RuntimeOperation&) const=default;
};
std::string_view runtime_error_category(std::string_view) noexcept;
class RuntimeTelemetryScope {
public:
    virtual ~RuntimeTelemetryScope()=default;
    virtual void with_context(FunctionRef<void()> callback) const {callback();}
    virtual void success(const RuntimeAttributes&)=0;
    virtual void failure(std::string_view error_type)=0;
};
class RuntimeTelemetryPropagationContext {
public:
    virtual ~RuntimeTelemetryPropagationContext()=default;
    virtual void with_context(FunctionRef<void()> callback) const {callback();}
};
class RuntimeTelemetry {
public:
    virtual ~RuntimeTelemetry()=default;
    virtual Memory memory_resource() const noexcept=0;
    // Inputs are call-duration borrows; retention requires an explicit clone.
    // Return a fresh box allocated in the supplied Context arena.
    virtual ContextBox<RuntimeTelemetryScope>& start(Memory,const RuntimeOperation&) const=0;
    virtual bool is_noop() const {return false;}
    virtual ContextBox<RuntimeTelemetryPropagationContext>& extract_context(Memory,const RuntimeContextCarrier&) const;
    virtual void flush() const {}
    virtual void shutdown() const {}
};
class NoopRuntimeTelemetry final:public RuntimeTelemetry {
public:
    using allocator_type=Allocator;
    NoopRuntimeTelemetry(std::allocator_arg_t,Allocator a):resource_(a.resource()){}
    Memory memory_resource() const noexcept override{return resource_;}
    ContextBox<RuntimeTelemetryScope>& start(Memory,const RuntimeOperation&) const override;
    bool is_noop() const override{return true;}
private:Memory resource_;
};
namespace detail {
// The callback result has two states: never polled, and polled (possibly Pending).
// A provider throwing after a completed poll cannot replace its result/error.
template<class Delegate,class Poll> auto telemetry_poll(Delegate* delegate,Poll&& poll){
    using R=decltype(poll());if(!delegate)return poll();
    std::optional<R> result;bool invoked=false;std::exception_ptr failure;
    auto callback=[&]{invoked=true;result.emplace(poll());};
    try{delegate->with_context(callback);}catch(...){failure=std::current_exception();}
    if(result)return std::move(*result);
    if(failure && invoked)std::rethrow_exception(failure);
    return poll();
}
template<class T,class Scope> struct TelemetryRun {
    using allocator_type=Allocator;
    Memory resource;Scope& scope;AsyncOperation<T>& future;WakeSignal::Forward forward;
    TelemetryRun(std::allocator_arg_t,Allocator a,Scope& scope,AsyncOperation<T>& future):resource(a.resource()),scope(scope),future(future){
        auto& destination=async_arena(resource);if(!resource_arena(scope.memory_resource()).can_lend_to(destination) || future.memory_resource()!=resource)throw std::invalid_argument("Telemetry operation belongs to another Context");
    }
    std::optional<T> poll(WakeSignal& wake){forward.attach(future.wake_signal(),wake);return scope.poll_with_context([&]()->std::optional<T>{if(auto* value=future.poll())return clone_in(resource,*value);return {};});}
    void finish_and_wait() noexcept {forward.reset();}
    void cancel_and_wait() noexcept {future.cancel();forward.reset();}
};
}
class FailOpenRuntimeTelemetryScope final {
public:
    using allocator_type=Allocator;
    FailOpenRuntimeTelemetryScope(std::allocator_arg_t,Allocator,ContextBox<RuntimeTelemetryScope>*);
    FailOpenRuntimeTelemetryScope(const FailOpenRuntimeTelemetryScope&)=delete;
    ~FailOpenRuntimeTelemetryScope(){close();}
    void close() noexcept{if(polling_)std::terminate();if(auto* owner=std::exchange(delegate_,nullptr))owner->reset();}
    void success(const RuntimeAttributes&);
    void failure(std::string_view error_type);
    Memory memory_resource() const noexcept{return resource_;}
    template<class Poll> auto poll_with_context(Poll&& poll) const {
        // A business exception escaping the lock's lifetime poisons the source
        // scope. Telemetry exceptions caught inside that lifetime do not.
        if(polling_)throw std::logic_error("Recursive telemetry scope activation");
        polling_=true;struct Guard{bool& value;~Guard(){value=false;}} guard{polling_};
        try{return detail::telemetry_poll(poisoned_?nullptr:delegate_?delegate_->get():nullptr,std::forward<Poll>(poll));}
        catch(...){poisoned_=true;throw;}
    }
    template<class T> AsyncOperation<T>& run(Memory r,AsyncOperation<T>& future) const{return make_operation<T,detail::TelemetryRun<T,const FailOpenRuntimeTelemetryScope>>(r,*this,future);}
private:
    Memory resource_;ContextBox<RuntimeTelemetryScope>* delegate_;mutable bool poisoned_=false,polling_=false;
};
class FailOpenRuntimeTelemetryPropagationContext final {
public:
    using allocator_type=Allocator;
    FailOpenRuntimeTelemetryPropagationContext(std::allocator_arg_t,Allocator,ContextBox<RuntimeTelemetryPropagationContext>*);
    FailOpenRuntimeTelemetryPropagationContext(const FailOpenRuntimeTelemetryPropagationContext&)=delete;
    ~FailOpenRuntimeTelemetryPropagationContext(){close();}
    void close() noexcept{if(auto* owner=std::exchange(delegate_,nullptr))owner->reset();}
    Memory memory_resource() const noexcept{return resource_;}
    template<class Poll> auto poll_with_context(Poll&& poll) const {return detail::telemetry_poll(delegate_?delegate_->get():nullptr,std::forward<Poll>(poll));}
    template<class T> AsyncOperation<T>& run(Memory r,AsyncOperation<T>& future) const{return make_operation<T,detail::TelemetryRun<T,const FailOpenRuntimeTelemetryPropagationContext>>(r,*this,future);}
private:Memory resource_;ContextBox<RuntimeTelemetryPropagationContext>* delegate_;
};
FailOpenRuntimeTelemetryScope& start_runtime_operation(Memory,const RuntimeTelemetry&,const RuntimeOperation&);
FailOpenRuntimeTelemetryPropagationContext& extract_runtime_context(Memory,const RuntimeTelemetry&,const RuntimeContextCarrier&);
}
