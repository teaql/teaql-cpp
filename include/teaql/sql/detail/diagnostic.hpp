#pragma once
#include <teaql/data_service/executor.hpp>

namespace teaql::detail {
class StatementDiagnostic {
public:
    using allocator_type = Allocator;
    StatementDiagnostic(std::allocator_arg_t,
                        Allocator,
                        std::optional<ExecutionMetadata>,
                        std::optional<ExecutionObserver>);
    ~StatementDiagnostic();
    StatementDiagnostic(const StatementDiagnostic&) = delete;
    std::optional<ExecutionMetadata> success();
    void fail() noexcept;
    void cancel() noexcept;

private:
    void notify(SqlExecutionOutcome) noexcept;
    std::optional<ExecutionMetadata> metadata_;
    std::optional<ExecutionObserver> observer_;
    Arena::Quiescence quiescence_;
};

class FailureJournal {
public:
    using allocator_type = Allocator;
    FailureJournal(std::allocator_arg_t, Allocator, std::optional<ExecutionObserver>);

    ~FailureJournal() {
        flush();
    }

    FailureJournal(const FailureJournal&) = delete;
    std::optional<ExecutionObserver> recorder() const;
    void disarm() noexcept;
    void flush() noexcept;

private:
    Memory resource_;
    std::optional<ExecutionObserver> observer_;
    Vector<ExecutionMetadata>* entries_ = nullptr;
    Arena::Quiescence quiescence_;
};
}
