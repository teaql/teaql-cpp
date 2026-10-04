#pragma once
#include <teaql/async/operation.hpp>
#include <teaql/core/query.hpp>
#include <teaql/runtime/resources.hpp>
#include <teaql/runtime/system_time.hpp>
namespace teaql {
namespace detail {struct QueryStoreAccess;struct CursorStoreOperation;struct IdSetStoreOperation;struct ContextPaginationAccess;}
struct ContinuousPageCursor {
    using allocator_type=Allocator;
    String cursor_id,query_key,entity;SortDirection direction=SortDirection::Asc;Value boundary;std::uint64_t page_size=0,next_offset=0;SystemTime expires_at;
    ContinuousPageCursor(std::allocator_arg_t,Allocator a):cursor_id(a.resource()),query_key(a.resource()),entity(a.resource()),boundary(a.resource()){}
    ContinuousPageCursor(std::allocator_arg_t,Allocator a,const ContinuousPageCursor& other):cursor_id(other.cursor_id,a.resource()),query_key(other.query_key,a.resource()),entity(other.entity,a.resource()),direction(other.direction),boundary(a.resource(),other.boundary),page_size(other.page_size),next_offset(other.next_offset),expires_at(other.expires_at){}
    ContinuousPageCursor(std::allocator_arg_t,Allocator a,ContinuousPageCursor&& other):ContinuousPageCursor(detail::move_to(a.resource(),std::move(other))){}
    ContinuousPageCursor(const ContinuousPageCursor&)=delete;ContinuousPageCursor(ContinuousPageCursor&&)noexcept=default;
    ContinuousPageCursor& operator=(ContinuousPageCursor&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource()const noexcept{return cursor_id.get_allocator().resource();}
    bool operator==(const ContinuousPageCursor&)const=default;
};
struct RetainedIdSet {
    using allocator_type=Allocator;
    String query_key;Vector<std::uint64_t> ids;SystemTime expires_at;
    RetainedIdSet(std::allocator_arg_t,Allocator a):query_key(a.resource()),ids(a.resource()){}
    RetainedIdSet(std::allocator_arg_t,Allocator a,const RetainedIdSet& other):query_key(other.query_key,a.resource()),ids(other.ids,a.resource()),expires_at(other.expires_at){}
    RetainedIdSet(std::allocator_arg_t,Allocator a,RetainedIdSet&& other):RetainedIdSet(detail::move_to(a.resource(),std::move(other))){}
    RetainedIdSet(const RetainedIdSet&)=delete;RetainedIdSet(RetainedIdSet&&)noexcept=default;
    RetainedIdSet& operator=(RetainedIdSet&& other){return detail::assign_scoped(*this,std::move(other));}
    Memory memory_resource()const noexcept{return query_key.get_allocator().resource();}
};
template<class T> using PaginationResult=Result<T,String>;
using PaginationWriteResult=PaginationResult<std::monostate>;
class ContinuousPageCursorStore {
public:
    virtual ~ContinuousPageCursorStore()=default;
    virtual Memory memory_resource()const noexcept=0;
    virtual AsyncOperation<PaginationResult<std::optional<ContinuousPageCursor>>>& get(Memory result,std::string_view query_key,std::uint64_t target_offset)=0;
    virtual AsyncOperation<PaginationWriteResult>& put(Memory result,const ContinuousPageCursor&)=0;
    virtual AsyncOperation<PaginationWriteResult>& invalidate(Memory result,std::string_view query_key)=0;
};
class IdSetStore {
public:
    virtual ~IdSetStore()=default;
    virtual Memory memory_resource()const noexcept=0;
    virtual AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>& get(Memory result,std::string_view query_key)=0;
    virtual AsyncOperation<PaginationWriteResult>& put(Memory result,const RetainedIdSet&)=0;
    virtual AsyncOperation<PaginationWriteResult>& invalidate(Memory result,std::string_view query_key)=0;
};
class InMemoryContinuousPageCursorStore final:public ContinuousPageCursorStore {
public:
    using allocator_type=Allocator;
    InMemoryContinuousPageCursorStore(std::allocator_arg_t,Allocator a):cursors_(a.resource()){}
    InMemoryContinuousPageCursorStore(const InMemoryContinuousPageCursorStore&)=delete;
    InMemoryContinuousPageCursorStore& operator=(const InMemoryContinuousPageCursorStore&)=delete;
    Memory memory_resource()const noexcept override{return cursors_.get_allocator().resource();}
    AsyncOperation<PaginationResult<std::optional<ContinuousPageCursor>>>& get(Memory,std::string_view,std::uint64_t)override;
    AsyncOperation<PaginationWriteResult>& put(Memory,const ContinuousPageCursor&)override;
    AsyncOperation<PaginationWriteResult>& invalidate(Memory,std::string_view)override;
private:
    friend struct detail::QueryStoreAccess;friend struct detail::CursorStoreOperation;
    std::pmr::map<String,ContinuousPageCursor,std::less<>> cursors_;std::size_t max_entries_=4096;
    std::optional<ContinuousPageCursor> get_now(Memory,std::string_view,std::uint64_t);
    void put_now(const ContinuousPageCursor&);
    void invalidate_now(std::string_view);
};
class InMemoryIdSetStore final:public IdSetStore {
public:
    using allocator_type=Allocator;
    InMemoryIdSetStore(std::allocator_arg_t,Allocator a):sets_(a.resource()){}
    InMemoryIdSetStore(const InMemoryIdSetStore&)=delete;
    InMemoryIdSetStore& operator=(const InMemoryIdSetStore&)=delete;
    Memory memory_resource()const noexcept override{return sets_.get_allocator().resource();}
    AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>& get(Memory,std::string_view)override;
    AsyncOperation<PaginationWriteResult>& put(Memory,const RetainedIdSet&)override;
    AsyncOperation<PaginationWriteResult>& invalidate(Memory,std::string_view)override;
private:
    friend struct detail::QueryStoreAccess;friend struct detail::IdSetStoreOperation;
    std::pmr::map<String,RetainedIdSet,std::less<>> sets_;std::size_t max_entries_=64,max_bytes_=256*1024*1024;
    std::optional<RetainedIdSet> get_now(Memory,std::string_view);
    PaginationWriteResult put_now(Memory,const RetainedIdSet&);
    void invalidate_now(std::string_view);
    std::size_t retained_bytes()const;
};
}
