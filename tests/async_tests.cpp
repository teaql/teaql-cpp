#include <teaql/async/stream.hpp>
#include <teaql/runtime/context.hpp>
#include <atomic>
#include <array>
#include <iostream>
#include <latch>
#include <semaphore>
#include <thread>
using namespace teaql;
static void check(bool condition,const char* message) {if(!condition) throw std::runtime_error(message);}
struct Counters {int polls=0,finished=0,cancelled=0,destroyed=0;};
struct Pulse {
    Counters* counts;bool fail=false;
    std::optional<int> poll(WakeSignal& signal) {
        ++counts->polls;if(fail) throw std::runtime_error("provider poll failure");
        if(counts->polls==1) {signal.notify();return {};}
        return 42;
    }
    void finish_and_wait() noexcept {++counts->finished;}
    void cancel_and_wait() noexcept {++counts->cancelled;}
};
struct Chunks {
    Counters* counts;int index=0;bool pending=true;
    std::optional<std::optional<int>> poll_next(WakeSignal& wake) {
        ++counts->polls;
        if(pending) {pending=false;wake.notify();return {};}
        if(index==2) return std::optional<int>{};
        pending=true;return std::optional<int>{++index};
    }
    void finish_and_wait() noexcept {++counts->finished;}
    void cancel_and_wait() noexcept {++counts->cancelled;}
};
struct ClosingProbe {
    UserContext& context;Counters& counts;bool& rejected;
    std::optional<int> poll(WakeSignal&) {return {};}
    void finish_and_wait() noexcept {}
    void cancel_and_wait() noexcept {
        ++counts.cancelled;context.close(); // Reentrant close must not free us.
        try{make_operation<int,Pulse>(context,&counts,false);}catch(const std::logic_error&){rejected=true;}
        auto& diagnostic=context.make<String>("quiescence can retain its final diagnostic");
        if(diagnostic.empty()) std::terminate();
    }
};
struct Payload {
    using allocator_type=Allocator;
    String text;bool* destroyed;
    Payload(std::allocator_arg_t,Allocator a,bool& destroyed) : text(10000,'x',a.resource()),destroyed(&destroyed) {}
    ~Payload() {*destroyed=true;}
};
struct Worker {
    UserContext& context;Counters& counts;bool& payload_destroyed;Payload* payload=nullptr;
    std::thread thread;
    std::latch started{1};std::binary_semaphore release{0};std::atomic<bool> done=false;
    bool read_before_destroy=false,released=false;
    Worker(UserContext& context,Counters& counts,bool& destroyed) : context(context),counts(counts),payload_destroyed(destroyed) {}
    ~Worker() {if(thread.joinable()) std::terminate();++counts.destroyed;}
    std::optional<int> poll(WakeSignal& wake) {
        ++counts.polls;
        if(!payload) {
            // Allocated after AsyncOperation's cleanup registration: ordinary
            // LIFO destruction would release this before joining the worker.
            payload=&context.make<Payload>(payload_destroyed);
            thread=std::thread([this,&wake] {
                started.count_down();release.acquire();
                read_before_destroy=!payload_destroyed && payload->text.size()==10000 && payload->text.front()=='x';
                done.store(true,std::memory_order_release);wake.notify();
            });
            started.wait();return {};
        }
        if(!done.load(std::memory_order_acquire)) return {};
        return 99;
    }
    void allow_completion() {if(!released) {released=true;release.release();}}
    void join() noexcept {if(thread.joinable()) {allow_completion();thread.join();}}
    void finish_and_wait() noexcept {join();++counts.finished;}
    void cancel_and_wait() noexcept {
        join();++counts.cancelled;
        // Cancellation diagnostics remain allocatable during quiescence.
        auto& message=context.make<String>("cancelled after all native host-memory access stopped");
        if(message.empty() || (payload && !read_before_destroy)) std::terminate();
    }
};
static void operations() {
    GlobalContext global;auto user=global.make_user_context();Counters counts;
    auto& task=make_operation<int,Pulse>(*user,&counts,false);
    check(counts.polls==0,"operations are lazy");
    check(task.wait()==42 && task.status()==AsyncStatus::Ready,"notify during poll cannot lose the wakeup");
    check(*task.poll()==42 && counts.polls==2 && counts.finished==1,"ready operation is memoized and joined once");
    task.cancel();check(counts.cancelled==0,"cancelling ready work cannot invent cancellation");
    Counters abandoned;auto& unused=make_operation<int,Pulse>(*user,&abandoned,false);unused.cancel();unused.cancel();
    bool cancelled=false;try{unused.poll();}catch(const OperationCancelled&){cancelled=true;}
    check(cancelled && abandoned.polls==0 && abandoned.cancelled==1,"unpolled cancellation is explicit and idempotent");
    Counters failures;auto& failure=make_operation<int,Pulse>(*user,&failures,true);
    for(int i=0;i<2;++i) {bool caught=false;try{failure.poll();}catch(const std::runtime_error&){caught=true;}check(caught,"poll failure rethrows original exception");}
    check(failures.polls==1 && failures.cancelled==1 && failure.status()==AsyncStatus::Failed,"failed operation quiesces once");
    user->close();check(counts.cancelled==0 && abandoned.cancelled==1 && failures.cancelled==1,"Context close does not repeat terminal hooks");
}
static void native_work() {
    GlobalContext global;auto user=global.make_user_context(128);Counters counts;bool destroyed=false;
    auto& worker=user->make_resource<Worker>(*user,counts,destroyed);auto& task=user->make<AsyncOperation<int>>(worker);
    check(!task.poll() && !destroyed,"native work suspends with its host payload live");
    user->close();check(counts.cancelled==1 && counts.finished==0 && counts.destroyed==1 && destroyed,"Context close joins native work before destroying late allocations");
    check(user->allocation_stats().live_blocks==0,"all arena blocks returned after native completion");
    auto other=global.make_user_context();Counters success;bool success_destroyed=false;
    auto& ready_worker=other->make_resource<Worker>(*other,success,success_destroyed);auto& ready_task=other->make<AsyncOperation<int>>(ready_worker);
    check(!ready_task.poll(),"native operation initially pending");ready_worker.allow_completion();
    check(ready_task.wait()==99 && ready_worker.read_before_destroy && success.finished==1,"native wake completion joins before exposing the result");
    other->close();check(success.cancelled==0 && success_destroyed,"successful work stays successful on Context close");
}
static void streams() {
    GlobalContext global;auto user=global.make_user_context();Counters counts;
    auto& stream=make_stream<int,Chunks>(*user,&counts,0,true);
    auto* first=stream.wait_next();auto* second=stream.wait_next();
    check(first && second && *first==1 && *second==2,"stream emits ordered Context-owned chunks");
    check(!stream.wait_next() && !stream.wait_next() && *first==1 && counts.finished==1,"EOF is stable and does not invalidate earlier chunks");
    Counters early;auto& cancelled=make_stream<int,Chunks>(*user,&early,0,true);
    check(cancelled.poll_next().kind==StreamPoll<int>::Kind::Pending,"stream pending differs from EOF");cancelled.cancel();
    bool caught=false;try{cancelled.wait_next();}catch(const OperationCancelled&){caught=true;}
    check(caught && early.cancelled==1,"explicit stream cancellation");user->close();check(counts.cancelled==0 && early.cancelled==1,"stream terminal cleanup is idempotent");
}
struct FailingResource : std::pmr::memory_resource {
    bool fail=false;
    void* do_allocate(std::size_t n,std::size_t a) override {if(fail) throw std::bad_alloc{};return std::pmr::new_delete_resource()->allocate(n,a);}
    void do_deallocate(void* p,std::size_t n,std::size_t a) override {std::pmr::new_delete_resource()->deallocate(p,n,a);}
    bool do_is_equal(const std::pmr::memory_resource& r) const noexcept override {return this==&r;}
};
using Large=std::array<int,65536>;
struct AllocFailureState {
    int& cancelled;
    AllocFailureState(FailingResource& r,int& cancelled) : cancelled(cancelled) {r.fail=true;}
    std::optional<Large> poll(WakeSignal&) {return {};}
    void finish_and_wait() noexcept {}
    void cancel_and_wait() noexcept {++cancelled;}
};
static void failed_registration() {
    FailingResource upstream;GlobalContext global(&upstream,128);auto user=global.make_user_context(128);int cancelled=0;bool caught=false;
    try{make_resource_operation<Large,AllocFailureState>(*user,upstream,cancelled);}catch(const std::bad_alloc&){caught=true;}
    check(caught && cancelled==1,"failed operation allocation quiesces already-created state");upstream.fail=false;user->close();
}
static void closing_registration() {
    GlobalContext global;auto user=global.make_user_context();Counters counts;bool rejected=false;
    make_operation<int,ClosingProbe>(*user,*user,counts,rejected);user->close();
    check(rejected && counts.cancelled==1 && counts.polls==0 && user->allocation_stats().live_blocks==0,"quiescence rejects new work and safely tolerates reentrant close");
}
int main(){try{auto* previous=std::pmr::set_default_resource(std::pmr::null_memory_resource());operations();native_work();streams();failed_registration();closing_registration();std::pmr::set_default_resource(previous);std::cout<<"Async Context contracts passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
