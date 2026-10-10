#pragma once
#include <teaql/core/query.hpp>
#include <teaql/data_service/types.hpp>

namespace teaql {
// Pure row operations need an allocation domain, not contextual runtime services.
class ExprEvaluator {
public:
    static bool eval(Memory destination, const Expr&, const CompactRow&);
    static Value resolve(Memory destination, const Expr&, const CompactRow&);

    static bool eval(const Expr& expr, const CompactRow& row) {
        return eval(row.memory_resource(), expr, row);
    }

    static Value resolve(const Expr& expr, const CompactRow& row) {
        return resolve(row.memory_resource(), expr, row);
    }
};

class InMemoryQueryEngine {
public:
    // Consume rows, moving within a domain and copying explicitly across domains.
    static QueryResult execute(Memory destination, const SelectQuery&, Vector<CompactRow> rows);
    // Borrow input only for this call; all returned storage belongs to destination.
    static QueryResult
    execute(Memory destination, const SelectQuery&, std::span<const CompactRow> rows);
};
}
