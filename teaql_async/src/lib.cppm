// Publish existing global declarations; implementation bodies remain canonical.
module;
#include <teaql/async/operation.hpp>
#include <teaql/async/pending.hpp>
#include <teaql/async/stream.hpp>
export module teaql_async;

export import teaql_memory;

export namespace teaql {
using ::teaql::AsyncOperation;
using ::teaql::AsyncStatus;
using ::teaql::AsyncStream;
using ::teaql::OperationCancelled;
using ::teaql::Result;
using ::teaql::StreamPoll;
using ::teaql::StreamStatus;
using ::teaql::WakeSignal;
using ::teaql::make_operation;
using ::teaql::make_resource_operation;
using ::teaql::make_resource_stream;
using ::teaql::make_stream;
}

export namespace teaql::detail {
using ::teaql::detail::PendingOperations;
using ::teaql::detail::async_arena;
using ::teaql::detail::copy_or_take_completed;
using ::teaql::detail::copy_or_take_error;
using ::teaql::detail::copy_or_take_item;
}
