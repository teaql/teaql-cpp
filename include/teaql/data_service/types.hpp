#pragma once
#include <teaql/core/mutation.hpp>
#include <teaql/data_service/sql_log.hpp>
#include <chrono>

namespace teaql {
struct DataServiceCapabilities {
    bool query=false,mutation=false,transaction=false,schema=false,id_generation=false;
    bool batch_mutation=false,returning=false,small_parent_relation_probes=false;
    bool operator==(const DataServiceCapabilities&) const=default;
};
enum class DataServiceOperation { Query,Insert,Update,Delete,Recover,Batch,Schema };
struct QueryRequest {
    using allocator_type=Allocator;
    SelectQuery query;
    TraceChain trace_chain;
    std::optional<String> comment;
    bool capture_debug_query;
    bool capture_execution_metadata;
    QueryRequest(std::allocator_arg_t,Allocator a,const SelectQuery& query,bool debug,bool metadata)
        : query(std::allocator_arg,a,query),trace_chain(a.resource()),capture_debug_query(debug),capture_execution_metadata(metadata) {}
    QueryRequest(std::allocator_arg_t,Allocator a,const QueryRequest& other)
        : query(std::allocator_arg,a,other.query),trace_chain(other.trace_chain,a.resource()),comment(detail::clone_in(a.resource(),other.comment)),
          capture_debug_query(other.capture_debug_query),capture_execution_metadata(other.capture_execution_metadata) {}
    QueryRequest(std::allocator_arg_t,Allocator a,QueryRequest&& other) : QueryRequest(detail::move_to(a.resource(),std::move(other))) {}
    QueryRequest(const QueryRequest&)=delete;
    QueryRequest(QueryRequest&&) noexcept=default;
    QueryRequest& operator=(QueryRequest&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return query.memory_resource(); }
};
struct MutationRequest {
    using allocator_type=Allocator;
    using Batch=Vector<MutationRequest>;
    using Storage=std::variant<InsertCommand,UpdateCommand,DeleteCommand,RecoverCommand,Batch>;
    Storage data;
    template<class T> requires (std::same_as<std::remove_cvref_t<T>,InsertCommand> || std::same_as<std::remove_cvref_t<T>,UpdateCommand>
        || std::same_as<std::remove_cvref_t<T>,DeleteCommand> || std::same_as<std::remove_cvref_t<T>,RecoverCommand> || std::same_as<std::remove_cvref_t<T>,Batch>)
    MutationRequest(std::allocator_arg_t,Allocator a,T&& value) : data(std::make_obj_using_allocator<std::remove_cvref_t<T>>(a,std::forward<T>(value))),resource_(a.resource()) {}
    MutationRequest(std::allocator_arg_t,Allocator a,const MutationRequest& other)
        : data(std::visit([&](const auto& value)->Storage {return std::make_obj_using_allocator<std::decay_t<decltype(value)>>(a,value);},other.data)),resource_(a.resource()) {}
    MutationRequest(std::allocator_arg_t,Allocator a,MutationRequest&& other) : MutationRequest(detail::move_to(a.resource(),std::move(other))) {}
    MutationRequest(const MutationRequest&)=delete;
    MutationRequest(MutationRequest&&) noexcept=default;
    MutationRequest& operator=(MutationRequest&& other) { return detail::assign_scoped(*this,std::move(other)); }
    std::span<const TraceNode> trace_chain() const {
        return std::visit([](const auto& value)->std::span<const TraceNode> {
            if constexpr(std::same_as<std::decay_t<decltype(value)>,Batch>) return {};
            else return value.trace_chain;
        },data);
    }
    std::optional<std::string_view> comment() const {
        auto chain=trace_chain();if(chain.empty()) return {};return chain.back().comment;
    }
    Memory memory_resource() const { return resource_; }
private:
    Memory resource_;
};
struct GuardedMutationRequest {
    // The eventual executor must apply guard in the mutation statement itself.
    // A read-before-write check cannot satisfy this contract.
    using allocator_type=Allocator;
    MutationRequest mutation;
    Expr guard;
    GuardedMutationRequest(std::allocator_arg_t,Allocator a,const MutationRequest& mutation,const Expr& guard)
        : mutation(std::allocator_arg,a,mutation),guard(std::allocator_arg,a,guard) {}
    GuardedMutationRequest(std::allocator_arg_t,Allocator a,const GuardedMutationRequest& other)
        : GuardedMutationRequest(std::allocator_arg,a,other.mutation,other.guard) {}
    GuardedMutationRequest(std::allocator_arg_t,Allocator a,GuardedMutationRequest&& other) : GuardedMutationRequest(detail::move_to(a.resource(),std::move(other))) {}
    GuardedMutationRequest(const GuardedMutationRequest&)=delete;
    GuardedMutationRequest(GuardedMutationRequest&&) noexcept=default;
    GuardedMutationRequest& operator=(GuardedMutationRequest&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return mutation.memory_resource(); }
};
struct ExecutionMetadata {
    using allocator_type=Allocator;
    using SystemTime=std::chrono::system_clock::time_point;
    Vector<ExecutionMetadata> statements;
    SqlLogContext sql_log;
    String backend;
    DataServiceOperation operation;
    SystemTime started_at,ended_at;
    std::optional<std::uint64_t> affected_rows;
    std::optional<std::size_t> result_count;
    TraceChain trace_chain;
    std::optional<String> comment,backend_request_id,parameterized_query;
    Vector<Value> params;
    std::optional<String> debug_query;
    ExecutionMetadata(std::allocator_arg_t,Allocator a,DataServiceOperation operation,SystemTime started={},SystemTime ended={})
        : statements(a.resource()),sql_log(a.resource()),backend(a.resource()),operation(operation),started_at(started),ended_at(ended),trace_chain(a.resource()),params(a.resource()) {}
    ExecutionMetadata(std::allocator_arg_t,Allocator a,const ExecutionMetadata& other)
        : statements(other.statements,a.resource()),sql_log(std::allocator_arg,a,other.sql_log),backend(other.backend,a.resource()),operation(other.operation),
          started_at(other.started_at),ended_at(other.ended_at),affected_rows(other.affected_rows),result_count(other.result_count),trace_chain(other.trace_chain,a.resource()),
          comment(detail::clone_in(a.resource(),other.comment)),backend_request_id(detail::clone_in(a.resource(),other.backend_request_id)),
          parameterized_query(detail::clone_in(a.resource(),other.parameterized_query)),params(other.params,a.resource()),debug_query(detail::clone_in(a.resource(),other.debug_query)) {}
    ExecutionMetadata(std::allocator_arg_t,Allocator a,ExecutionMetadata&& other) : ExecutionMetadata(detail::move_to(a.resource(),std::move(other))) {}
    ExecutionMetadata(const ExecutionMetadata&)=delete;
    ExecutionMetadata(ExecutionMetadata&&) noexcept=default;
    ExecutionMetadata& operator=(ExecutionMetadata&& other) { return detail::assign_scoped(*this,std::move(other)); }
    static ExecutionMetadata unrecorded_query(Memory r,std::size_t count) {
        ExecutionMetadata result(std::allocator_arg,Allocator(r),DataServiceOperation::Query);result.result_count=count;return result;
    }
    Memory memory_resource() const { return backend.get_allocator().resource(); }
};
struct QueryResult {
    using allocator_type=Allocator;
    Vector<CompactRow> rows;
    ExecutionMetadata metadata;
    QueryResult(std::allocator_arg_t,Allocator a,const ExecutionMetadata& metadata) : rows(a.resource()),metadata(std::allocator_arg,a,metadata) {}
    QueryResult(std::allocator_arg_t,Allocator a,ExecutionMetadata&& metadata) : rows(a.resource()),metadata(std::allocator_arg,a,std::move(metadata)) {}
    QueryResult(std::allocator_arg_t,Allocator a,const QueryResult& other) : rows(other.rows,a.resource()),metadata(std::allocator_arg,a,other.metadata) {}
    QueryResult(std::allocator_arg_t,Allocator a,QueryResult&& other) : QueryResult(detail::move_to(a.resource(),std::move(other))) {}
    QueryResult(const QueryResult&)=delete;
    QueryResult(QueryResult&&) noexcept=default;
    QueryResult& operator=(QueryResult&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return rows.get_allocator().resource(); }
};
struct MutationResult {
    using allocator_type=Allocator;
    std::uint64_t affected_rows;
    GeneratedValues generated_values;
    std::optional<EntitySnapshot> persisted_snapshot;
    ExecutionMetadata metadata;
    MutationResult(std::allocator_arg_t,Allocator a,std::uint64_t affected,const ExecutionMetadata& metadata)
        : affected_rows(affected),generated_values(a.resource()),metadata(std::allocator_arg,a,metadata) {}
    MutationResult(std::allocator_arg_t,Allocator a,std::uint64_t affected,ExecutionMetadata&& metadata)
        : affected_rows(affected),generated_values(a.resource()),metadata(std::allocator_arg,a,std::move(metadata)) {}
    MutationResult(std::allocator_arg_t,Allocator a,const MutationResult& other)
        : affected_rows(other.affected_rows),generated_values(a.resource(),other.generated_values),persisted_snapshot(detail::clone_in(a.resource(),other.persisted_snapshot)),metadata(std::allocator_arg,a,other.metadata) {}
    MutationResult(std::allocator_arg_t,Allocator a,MutationResult&& other) : MutationResult(detail::move_to(a.resource(),std::move(other))) {}
    MutationResult(const MutationResult&)=delete;
    MutationResult(MutationResult&&) noexcept=default;
    MutationResult& operator=(MutationResult&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return metadata.memory_resource(); }
};
struct StreamChunk {
    using allocator_type=Allocator;
    Vector<CompactRow> rows;
    std::size_t chunk_index;
    bool is_last;
    StreamChunk(std::allocator_arg_t,Allocator a,std::size_t index,bool last) : rows(a.resource()),chunk_index(index),is_last(last) {}
    StreamChunk(std::allocator_arg_t,Allocator a,const StreamChunk& other) : rows(other.rows,a.resource()),chunk_index(other.chunk_index),is_last(other.is_last) {}
    StreamChunk(std::allocator_arg_t,Allocator a,StreamChunk&& other) : StreamChunk(detail::move_to(a.resource(),std::move(other))) {}
    StreamChunk(const StreamChunk&)=delete;
    StreamChunk(StreamChunk&&) noexcept=default;
    StreamChunk& operator=(StreamChunk&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return rows.get_allocator().resource(); }
};
struct SchemaRequest {
    using allocator_type=Allocator;
    String entity_name;
    SchemaRequest(std::allocator_arg_t,Allocator a,std::string_view entity) : entity_name(entity,a.resource()) {}
    SchemaRequest(std::allocator_arg_t,Allocator a,const SchemaRequest& other) : entity_name(other.entity_name,a.resource()) {}
    SchemaRequest(std::allocator_arg_t,Allocator a,SchemaRequest&& other) : SchemaRequest(detail::move_to(a.resource(),std::move(other))) {}
    SchemaRequest(const SchemaRequest&)=delete;
    SchemaRequest(SchemaRequest&&) noexcept=default;
    SchemaRequest& operator=(SchemaRequest&& other) { return detail::assign_scoped(*this,std::move(other)); }
    Memory memory_resource() const { return entity_name.get_allocator().resource(); }
};
struct SchemaResult { bool changed; };
}
