#include <teaql/runtime/error.hpp>

namespace teaql {
RuntimeError RuntimeError::audit_after_commit(Memory r, const RuntimeError& source) {
    RuntimeError result(r, Kind::AuditAfterCommit, String(r));
    result.data_.emplace<detail::ScopedBox<RuntimeError>>(
        detail::make_box<RuntimeError>(r, source));
    return result;
}

String RuntimeError::to_string(Memory r) const {
    String out(r);
    switch (kind_) {
    case Kind::AuditAfterCommit:
        return String(
            "AUDIT_DELIVERY_AFTER_COMMIT: transaction committed but audit delivery failed; do not retry the mutation blindly",
            r);
    case Kind::DynamicField:
        return dynamic_field_error().to_string(r);
    case Kind::RequestIntent:
        return request_intent_error().to_string(r);
    case Kind::MissingEntity:
        out = "missing entity descriptor: ";
        out += message();
        return out;
    case Kind::Behavior:
        out = "entity data service behavior error: ";
        out += message();
        return out;
    case Kind::Event:
        out = "entity event error: ";
        out += message();
        return out;
    case Kind::Policy:
        out = "request policy error: ";
        out += message();
        return out;
    case Kind::Graph:
        out = "graph write error: ";
        out += message();
        return out;
    case Kind::IdGeneration:
        out = "id generation error: ";
        out += message();
        return out;
    case Kind::Language:
        out = "language error: ";
        out += message();
        return out;
    case Kind::UnsupportedLocale:
        out = "unsupported locale: ";
        out += message();
        return out;
    case Kind::Schema:
        out = "schema provider error: ";
        out += message();
        return out;
    case Kind::Transaction:
        out = "transaction error: ";
        out += message();
        return out;
    case Kind::SqlCompile:
        return sql_compile_error().to_string(r);
    case Kind::Check: {
        out = "check failed: ";
        bool first = true;
        for (const auto& error : check_results()) {
            if (!first) {
                out += "; ";
            }
            out += error.to_string(r);
            first = false;
        }
        return out;
    }
    case Kind::MissingRelation: {
        const auto& e = missing_relation_error();
        out = "missing relation ";
        out += e.relation;
        out += " on entity ";
        out += e.entity;
        return out;
    }
    case Kind::OptimisticLockConflict: {
        const auto& e = optimistic_lock_conflict_error();
        out = "optimistic lock conflict on ";
        out += e.entity;
        out += '(';
        out += e.id;
        out += ')';
        return out;
    }
    }
    throw std::logic_error("Invalid runtime error kind");
}
}
