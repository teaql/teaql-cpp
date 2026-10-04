#pragma once
#include <teaql/sql/detail/query_execution.hpp>
#include <limits>

namespace teaql::detail {
template<class E> class SqlErrorStream {
public:
    using allocator_type=Allocator;
    using Item=Result<StreamChunk,SqlExecutorError<E>>;
    SqlErrorStream(std::allocator_arg_t,Allocator a,SqlCompileError error):resource_(a.resource()),error_(std::allocator_arg,a,std::move(error)) {}
    std::optional<std::optional<Item>> poll_next(WakeSignal&) {
        if(delivered_)return std::optional<Item>{};delivered_=true;
        return std::optional<Item>(Item::err(resource_,SqlExecutorError<E>::compile(resource_,std::move(error_))));
    }
    void finish_and_wait() noexcept{}
    void cancel_and_wait() noexcept{}
private:
    Memory resource_;SqlCompileError error_;bool delivered_=false;
};
template<class E> QueryStream<SqlExecutorError<E>>& sql_error_stream(Memory r,SqlCompileError error){return make_stream<Result<StreamChunk,SqlExecutorError<E>>,SqlErrorStream<E>>(r,std::move(error));}

template<class E> class MappedSqlStream {
public:
    using allocator_type=Allocator;
    using Item=Result<StreamChunk,SqlExecutorError<E>>;
    MappedSqlStream(std::allocator_arg_t,Allocator a,QueryStream<E>& source):resource_(a.resource()),source_(&source){
        if(source.memory_resource()!=resource_){source.cancel();throw std::invalid_argument("SQL cursor belongs to a different Context");}
    }
    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        if(!source_)return std::optional<Item>{};
        forwarding_.attach(source_->wake_signal(),wake);auto next=source_->poll_next();
        using Kind=typename StreamPoll<Result<StreamChunk,E>>::Kind;
        if(next.kind==Kind::Pending)return {};
        forwarding_.reset();
        if(next.kind==Kind::End){source_=nullptr;return std::optional<Item>{};}
        if(!*next.item)return std::optional<Item>(Item::err(resource_,SqlExecutorError<E>::transport(resource_,clone_in(resource_,next.item->error()))));
        return std::optional<Item>(Item::ok(resource_,clone_in(resource_,next.item->value())));
    }
    void finish_and_wait() noexcept{stop();}
    void cancel_and_wait() noexcept{stop();}
private:
    void stop() noexcept{if(source_)source_->cancel();forwarding_.reset();source_=nullptr;}
    Memory resource_;QueryStream<E>* source_;WakeSignal::Forward forwarding_;
};

// The diagnostic belongs to the cursor. Stop/join it before the sink and count
// only delivered rows, including a final chunk, never buffered undelivered rows.
template<class E> class DiagnosticStream {
public:
    using allocator_type=Allocator;
    using Item=Result<StreamChunk,E>;
    DiagnosticStream(std::allocator_arg_t,Allocator a,QueryStream<E>& source,ExecutionMetadata metadata,ExecutionObserver observer)
        :resource_(a.resource()),source_(&source),metadata_(move_in(a.resource(),std::move(metadata))),observer_(std::allocator_arg,a,std::move(observer)) {
        if(source.memory_resource()!=resource_)throw std::invalid_argument("Diagnostic cursor belongs to a different Context");
    }
    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        if(!source_)return std::optional<Item>{};
        try {
            forwarding_.attach(source_->wake_signal(),wake);auto next=source_->poll_next();
            using Kind=typename StreamPoll<Item>::Kind;
            if(next.kind==Kind::Pending)return {};
            forwarding_.reset();
            if(next.kind==Kind::End){finish(SqlExecutionOutcome::Success);return std::optional<Item>{};}
            auto item=clone_in(resource_,*next.item);
            if(item){
                if(item.value().rows.size()>std::numeric_limits<std::size_t>::max()-delivered_)throw std::overflow_error("delivered row count overflow");
                delivered_+=item.value().rows.size();if(item.value().is_last)finish(SqlExecutionOutcome::Success);
            }else finish(SqlExecutionOutcome::Failure);
            return std::optional<Item>(std::move(item));
        }catch(...){finish(SqlExecutionOutcome::Failure);throw;}
    }
    void finish_and_wait() noexcept{finish(SqlExecutionOutcome::Success);}
    void cancel_and_wait() noexcept{finish(std::uncaught_exceptions()?SqlExecutionOutcome::Failure:SqlExecutionOutcome::Cancelled);}
private:
    void finish(SqlExecutionOutcome outcome) noexcept {
        if(source_)source_->cancel();forwarding_.reset();source_=nullptr;
        auto metadata=std::move(metadata_);metadata_.reset();
        if(!metadata)return;
        metadata->ended_at=ExecutionMetadata::SystemTime::clock::now();metadata->result_count=delivered_;metadata->sql_log.execution_outcome=outcome;
        try{observer_(std::move(*metadata));}catch(...){}
    }
    Memory resource_;QueryStream<E>* source_;std::optional<ExecutionMetadata> metadata_;ExecutionObserver observer_;std::size_t delivered_=0;WakeSignal::Forward forwarding_;
};
template<class E> QueryStream<E>& diagnostic_stream(Memory r,QueryStream<E>& source,ExecutionMetadata metadata,ExecutionObserver observer) {
    try{return make_stream<Result<StreamChunk,E>,DiagnosticStream<E>>(r,source,std::move(metadata),std::move(observer));}
    catch(...){source.cancel();throw;}
}

template<class E> QueryStream<SqlExecutorError<E>>& native_sql_stream(Memory r,const SqlDialect& dialect,StreamingSqlTransport<E>& transport,const SchemaProvider& schema,SqlExecutorState& cache,QueryRequest request,std::size_t chunk_size,std::optional<ExecutionObserver> observer) {
    std::optional<CompiledQuery> compiled;
    try {
        auto* entity=cache.entity_descriptor(schema,request.query.entity);
        if(!entity)throw SqlCompileError(std::allocator_arg,Allocator(r),SqlCompileError::Code::UnknownEntity,request.query.entity);
        compiled.emplace(cache.compile_select_cached(r,dialect,*entity,request.query));
    }catch(const SqlCompileError& error){return sql_error_stream<E>(r,clone_in(r,error));}
    std::optional<ExecutionMetadata> metadata;
    if(observer && request.capture_execution_metadata)metadata.emplace(query_diagnostic_metadata(r,dialect,*compiled,request));
    auto& source=transport.stream_sql(r,std::move(*compiled),chunk_size);
    QueryStream<SqlExecutorError<E>>* mapped;
    try{mapped=&make_stream<Result<StreamChunk,SqlExecutorError<E>>,MappedSqlStream<E>>(r,source);}catch(...){source.cancel();throw;}
    if(metadata)return diagnostic_stream(r,*mapped,std::move(*metadata),std::move(*observer));
    return *mapped;
}

template<class E> class BufferedTransactionStream {
public:
    using allocator_type=Allocator;
    using Error=SqlExecutorError<E>;
    using Item=Result<StreamChunk,Error>;
    BufferedTransactionStream(std::allocator_arg_t,Allocator a,const SqlDialect& dialect,SqlTransport<E>& transport,const SchemaProvider& schema,SqlExecutorState& cache,QueryRequest request,std::size_t size)
        :resource_(a.resource()),dialect_(dialect),transport_(transport),schema_(schema),cache_(cache),request_(std::allocator_arg,a,std::move(request)),size_(std::max<std::size_t>(1,size)) {}
    std::optional<std::optional<Item>> poll_next(WakeSignal& wake) {
        if(done_)return std::optional<Item>{};
        if(!started_){using State=SqlQueryOperation<E>;pending_=&make_operation<typename State::Output,State>(resource_,dialect_,transport_,schema_,cache_,std::move(request_),std::optional<ExecutionObserver>{},false);started_=true;}
        if(pending_){
            forwarding_.attach(pending_->wake_signal(),wake);auto* result=pending_->poll();if(!result)return {};
            forwarding_.reset();pending_=nullptr;
            if(!*result){done_=true;return std::optional<Item>(Item::err(resource_,clone_in(resource_,result->error())));}
            rows_=&result->value().rows;
        }
        if(offset_==rows_->size()){done_=true;return std::optional<Item>{};}
        auto take=std::min(size_,rows_->size()-offset_);StreamChunk chunk(std::allocator_arg,Allocator(resource_),index_++,offset_+take==rows_->size());
        for(std::size_t i=0;i<take;++i)chunk.rows.emplace_back((*rows_)[offset_+i]);offset_+=take;
        return std::optional<Item>(Item::ok(resource_,std::move(chunk)));
    }
    void finish_and_wait() noexcept{stop();}
    void cancel_and_wait() noexcept{stop();}
private:
    void stop() noexcept{if(pending_)pending_->cancel();forwarding_.reset();pending_=nullptr;rows_=nullptr;done_=true;}
    Memory resource_;const SqlDialect& dialect_;SqlTransport<E>& transport_;const SchemaProvider& schema_;SqlExecutorState& cache_;QueryRequest request_;
    std::size_t size_,offset_=0,index_=0;bool started_=false,done_=false;const Vector<CompactRow>* rows_=nullptr;
    DataServiceOperationResult<QueryResult,Error>* pending_=nullptr;WakeSignal::Forward forwarding_;
};
template<class E> QueryStream<SqlExecutorError<E>>& buffered_transaction_stream(Memory r,const SqlDialect& dialect,SqlTransport<E>& transport,const SchemaProvider& schema,SqlExecutorState& cache,QueryRequest request,std::size_t chunk_size,std::optional<ExecutionObserver> observer) {
    std::optional<ExecutionMetadata> metadata;
    if(observer && request.capture_execution_metadata){
        try{
            auto* entity=cache.entity_descriptor(schema,request.query.entity);
            if(!entity)throw SqlCompileError(std::allocator_arg,Allocator(r),SqlCompileError::Code::UnknownEntity,request.query.entity);
            auto compiled=cache.compile_select_cached(r,dialect,*entity,request.query);metadata.emplace(query_diagnostic_metadata(r,dialect,compiled,request));
        }catch(const SqlCompileError& error){return sql_error_stream<E>(r,clone_in(r,error));}
        request.capture_execution_metadata=false;
    }
    auto& source=make_stream<Result<StreamChunk,SqlExecutorError<E>>,BufferedTransactionStream<E>>(r,dialect,transport,schema,cache,std::move(request),chunk_size);
    if(metadata)return diagnostic_stream(r,source,std::move(*metadata),std::move(*observer));
    return source;
}

template<class E,class T,bool Enabled=std::derived_from<T,StreamingSqlTransport<E>>> class SqlStreamExecutorSupport {
public:
    SqlStreamExecutorSupport(const SqlDialect&,T&,const SchemaProvider&,SqlExecutorState&) {}
};
template<class E,class T> class SqlStreamExecutorSupport<E,T,true> : public StreamQueryExecutor<SqlExecutorError<E>> {
public:
    SqlStreamExecutorSupport(const SqlDialect& dialect,T& transport,const SchemaProvider& schema,SqlExecutorState& cache):dialect_(dialect),transport_(transport),schema_(schema),cache_(cache) {}
    QueryStream<SqlExecutorError<E>>& query_stream(Memory r,QueryRequest request,std::size_t size) override{return native_sql_stream<E>(r,dialect_,transport_,schema_,cache_,std::move(request),size,{});}
    QueryStream<SqlExecutorError<E>>& query_stream_observed(Memory r,QueryRequest request,std::size_t size,ExecutionObserver observer) override{return native_sql_stream<E>(r,dialect_,transport_,schema_,cache_,std::move(request),size,std::move(observer));}
private:
    const SqlDialect& dialect_;T& transport_;const SchemaProvider& schema_;SqlExecutorState& cache_;
};
}
