#pragma once
#include <teaql/runtime/detail/context_query.hpp>
#include "entity_decoder_fixture.hpp"
namespace context_query_fixture {
using namespace teaql;
struct Config {bool noop=false,start_fail=false,finish_fail=false,failure=false,bad_model=false,event=false;unsigned pending=0;std::string_view activation="normal";};
struct State {
    using allocator_type=Allocator;Memory r;Config config;Vector<String> notes,active;Vector<RuntimeOperation> operations;QueryRequest* request=nullptr;
    State(std::allocator_arg_t,Allocator a,Config config):r(a.resource()),config(config),notes(r),active(r),operations(r){}
    String path() const{String result(r);for(const auto& s:active){if(!result.empty())result+='/';result+=s;}if(result.empty())result="outside";return result;}
    void note(std::string_view prefix,std::string_view suffix={}){String value(prefix,r);value+=suffix;notes.emplace_back(std::move(value));}
};
struct Scope final:RuntimeTelemetryScope {
    using allocator_type=Allocator;State& state;String family;
    Scope(std::allocator_arg_t,Allocator a,State& state,std::string_view family):state(state),family(family,a.resource()){}
    ~Scope(){state.note("drop:",family);}
    void with_context(FunctionRef<void()> callback) const override{
        state.note("enter:",family);state.active.emplace_back(family);
        struct Leave {State& s;std::string_view family;~Leave(){s.active.pop_back();s.note("leave:",family);}} leave{state,family};
        auto behavior=state.config.activation;
        if(behavior=="before")throw std::runtime_error("telemetry before");
        if(behavior=="skip")return;
        if(behavior=="swallow"){try{callback();}catch(...){}return;}
        callback();if(behavior=="twice")callback();if(behavior=="after")throw std::runtime_error("telemetry after");
    }
    void success(const RuntimeAttributes& attrs) override{if(!attrs.empty())throw std::logic_error("unexpected success attributes");state.note("success:",family);if(state.config.finish_fail)throw std::runtime_error("finish");}
    void failure(std::string_view error) override{String value(family,state.r);value+=':';value+=error;state.note("failure:",value);if(state.config.finish_fail)throw std::runtime_error("finish");}
};
struct Telemetry final:RuntimeTelemetry {
    using allocator_type=Allocator;Memory r;State& state;
    Telemetry(std::allocator_arg_t,Allocator a,State& s):r(a.resource()),state(s){}
    Memory memory_resource() const noexcept override{return r;}
    bool is_noop() const override{return state.config.noop;}
    ContextBox<RuntimeTelemetryScope>& start(Memory target,const RuntimeOperation& operation) const override{
        String value(operation.family,r);value+=':';value+=state.path();state.note("start:",value);state.operations.emplace_back(operation);
        if(state.config.start_fail)throw std::runtime_error("start");
        return detail::resource_arena(target).make<ContextBox<RuntimeTelemetryScope>>(std::in_place_type<Scope>,state,operation.family);
    }
};
struct Mock final:QueryExecutor<String>,MutationExecutor<String> {
    using allocator_type=Allocator;Memory r;State& state;
    Mock(std::allocator_arg_t,Allocator a,State& s):r(a.resource()),state(s){}
    Memory memory_resource() const{return r;}DataServiceCapabilities capabilities() const override{return {};}
    struct Work {
        using allocator_type=Allocator;Memory r;State& state;QueryRequest query;std::optional<ExecutionObserver> observer;unsigned pending;bool started=false,dropped=false;
        Work(std::allocator_arg_t,Allocator a,State& state,QueryRequest q,std::optional<ExecutionObserver> observer)
            :r(a.resource()),state(state),query(std::allocator_arg,a,std::move(q)),observer(detail::move_in(r,std::move(observer))),pending(state.config.pending){}
        std::optional<Result<QueryResult,String>> poll(WakeSignal& wake){
            if(!started){started=true;state.request=&detail::resource_arena(state.r).make<QueryRequest>(query);state.note("request:",state.path());if(state.config.event && observer)(*observer)(ExecutionMetadata::unrecorded_query(r,99));}
            state.note("poll:",state.path());if(pending){--pending;wake.notify();return {};}
            using R=Result<QueryResult,String>;if(state.config.failure)return R::err(r,String("fetch failed",r));
            QueryResult result(std::allocator_arg,Allocator(r),ExecutionMetadata::unrecorded_query(r,1));Record values(r);values.emplace(String("id",r),Value(r,std::uint64_t(1)));values.emplace(String("fail",r),Value(r,state.config.bad_model));result.rows.emplace_back(values);return R::ok(r,std::move(result));
        }
        void finish_and_wait() noexcept{drop();}void cancel_and_wait() noexcept{drop();}
        void drop() noexcept{if(started && !dropped){dropped=true;state.note("work.drop");}}
    };
    DataServiceOperationResult<QueryResult,String>& query(Memory target,QueryRequest q) override{return query_observed(target,std::move(q),{});}
    DataServiceOperationResult<QueryResult,String>& query_observed(Memory target,QueryRequest q,std::optional<ExecutionObserver> observer) override{return make_operation<Result<QueryResult,String>,Work>(target,state,std::move(q),std::move(observer));}
    DataServiceOperationResult<MutationResult,String>& mutate(Memory,MutationRequest) override{throw std::logic_error("unexpected mutation");}
};
inline auto& service(UserContext& c,Mock& mock){auto r=c.memory_resource();ResourceBorrow<Mock> borrow(std::allocator_arg,Allocator(r),mock,detail::resource_arena(mock.memory_resource()));return c.make<detail::ContextDataService<Mock>>(c,borrow);}
}
