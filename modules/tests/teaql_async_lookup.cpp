// Isolated import verifies this package cannot borrow another facade's exports.
#include <cstdint>
import teaql_async;

namespace isolated_teaql_async_teaql {
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

namespace isolated_teaql_async_teaql_detail {
using ::teaql::detail::PendingOperations;
using ::teaql::detail::async_arena;
using ::teaql::detail::copy_or_take_completed;
using ::teaql::detail::copy_or_take_error;
using ::teaql::detail::copy_or_take_item;
}
