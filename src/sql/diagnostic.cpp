#include <teaql/sql/detail/diagnostic.hpp>
#include <exception>

namespace teaql::detail {
StatementDiagnostic::StatementDiagnostic(std::allocator_arg_t,
                                         Allocator a,
                                         std::optional<ExecutionMetadata> metadata,
                                         std::optional<ExecutionObserver> observer)
    : metadata_(move_in(a.resource(), std::move(metadata))),
      observer_(move_in(a.resource(), std::move(observer))) {
    quiescence_.attach(async_arena(a.resource()), this, [](void* p) noexcept {
        static_cast<StatementDiagnostic*>(p)->cancel();
    });
}

StatementDiagnostic::~StatementDiagnostic() {
    cancel();
}

std::optional<ExecutionMetadata> StatementDiagnostic::success() {
    auto result = std::move(metadata_);
    metadata_.reset();
    quiescence_.reset();
    if (result) {
        result->ended_at = ExecutionMetadata::SystemTime::clock::now();
        result->sql_log.execution_outcome = SqlExecutionOutcome::Success;
    }
    return result;
}

void StatementDiagnostic::notify(SqlExecutionOutcome outcome) noexcept {
    auto metadata = std::move(metadata_);
    metadata_.reset();
    quiescence_.reset();
    if (!metadata)
        return;
    metadata->ended_at = ExecutionMetadata::SystemTime::clock::now();
    metadata->sql_log.execution_outcome = outcome;
    if (observer_)
        try {
            (*observer_)(std::move(*metadata));
        } catch (...) {
        } // Sink failures never replace provider failures.
}

void StatementDiagnostic::fail() noexcept {
    notify(SqlExecutionOutcome::Failure);
}

void StatementDiagnostic::cancel() noexcept {
    notify(std::uncaught_exceptions() ? SqlExecutionOutcome::Failure
                                      : SqlExecutionOutcome::Cancelled);
}

FailureJournal::FailureJournal(std::allocator_arg_t,
                               Allocator a,
                               std::optional<ExecutionObserver> observer)
    : resource_(a.resource()), observer_(move_in(a.resource(), std::move(observer))) {
    auto& arena = async_arena(resource_);
    if (observer_)
        entries_ = &arena.make<Vector<ExecutionMetadata>>();
    quiescence_.attach(arena, this, [](void* p) noexcept {
        static_cast<FailureJournal*>(p)->flush();
    });
}

std::optional<ExecutionObserver> FailureJournal::recorder() const {
    if (!entries_)
        return {};
    auto* entries = entries_;
    return ExecutionObserver(
        std::allocator_arg, Allocator(resource_), [entries](ExecutionMetadata metadata) {
            entries->emplace_back(std::move(metadata));
        });
}

void FailureJournal::disarm() noexcept {
    observer_.reset();
    quiescence_.reset();
}

void FailureJournal::flush() noexcept {
    quiescence_.reset();
    if (!observer_ || !entries_)
        return;
    auto observer = std::move(observer_);
    observer_.reset();
    auto entries = std::move(*entries_);
    for (auto& entry : entries)
        try {
            (*observer)(std::move(entry));
        } catch (...) {
        }
}
}
