// Publish existing global declarations; implementation bodies remain canonical.
module;
#include <teaql/memory/arena.hpp>
#include <teaql/memory/clone.hpp>
#include <teaql/memory/context_box.hpp>
#include <teaql/memory/function.hpp>
#include <teaql/memory/function_ref.hpp>
#include <teaql/memory/limits.hpp>
#include <teaql/memory/once_function.hpp>
#include <teaql/memory/reference.hpp>
#include <teaql/memory/scoped.hpp>
#include <teaql/memory/strings.hpp>
#include <teaql/memory/types.hpp>
export module teaql_memory;

export namespace teaql {
using ::teaql::AllocationStats;
using ::teaql::Allocator;
using ::teaql::Arena;
using ::teaql::BytesRef;
using ::teaql::ContextBox;
using ::teaql::ContextFunction;
using ::teaql::ContextLimits;
using ::teaql::ContextOnceFunction;
using ::teaql::FunctionRef;
using ::teaql::GlobalContext;
using ::teaql::Memory;
using ::teaql::MemoryLimitError;
using ::teaql::MemoryLimits;
using ::teaql::Ref;
using ::teaql::ReferenceError;
using ::teaql::StaticString;
using ::teaql::String;
using ::teaql::StringPoolLimits;
using ::teaql::StringRef;
using ::teaql::StringStorage;
using ::teaql::StringStorageStats;
using ::teaql::UserContext;
using ::teaql::Vector;
using ::teaql::context_identity_limit;
using ::teaql::object_slot_limit;
}

export namespace teaql::detail {
using ::teaql::detail::ByteBudgetResource;
using ::teaql::detail::ContextState;
using ::teaql::detail::DomainCopy;
using ::teaql::detail::ScopedBox;
using ::teaql::detail::ScopedDelete;
using ::teaql::detail::assign_scoped;
using ::teaql::detail::clone_in;
using ::teaql::detail::context_arena;
using ::teaql::detail::context_shift;
using ::teaql::detail::copy_callable;
using ::teaql::detail::domain_copyable;
using ::teaql::detail::issue_runtime_identity;
using ::teaql::detail::make_box;
using ::teaql::detail::move_in;
using ::teaql::detail::move_to;
using ::teaql::detail::owner_shift;
using ::teaql::detail::reference_context;
using ::teaql::detail::reference_global;
using ::teaql::detail::reference_id;
using ::teaql::detail::reference_runtime;
using ::teaql::detail::reference_slot;
using ::teaql::detail::relocatable_callable;
using ::teaql::detail::runtime_identity_limit;
using ::teaql::detail::runtime_shift;
}
