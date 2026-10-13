#pragma once
#include <teaql/core/entity_debug.hpp>
#include <teaql/runtime/error.hpp>
#include <teaql/runtime/check_result_debug.hpp>
#include <teaql/sql/error_debug.hpp>

namespace teaql {
// Raw diagnostics retain complete payloads. Safe log projection is separate.
inline void write_debug(DebugWriter& writer, const ContextError& error) {
    if (static_cast<std::size_t>(error.kind()) >= 3) {
        throw std::invalid_argument("Unknown Context error diagnostic variant");
    }
    constexpr std::string_view names[]{
        "MissingResource", "MissingTypedResource", "MissingEntityDataService"};
    writer.tuple(names[static_cast<std::size_t>(error.kind())], [&](auto& out) {
        out.text(error.name());
    });
}

inline void write_debug(DebugWriter& writer, const RuntimeError& error) {
    if (static_cast<std::size_t>(error.kind()) >= 17) {
        throw std::invalid_argument("Unknown runtime error diagnostic variant");
    }
    using Kind = RuntimeError::Kind;
    if (error.kind() == Kind::AuditAfterCommit) {
        writer.structure("AuditAfterCommit", [&](auto& fields) {
            fields.field("source", [&](auto& out) {
                write_debug(out, error.audit_after_commit_error());
            });
        });
        return;
    }
    if (error.kind() == Kind::DynamicField) {
        writer.tuple("DynamicField", [&](auto& out) {
            out.structure("DynamicFieldError", [&](auto& fields) {
                fields.field("code", [&](auto& inner) {
                    inner.text(error.dynamic_field_error().code);
                });
                fields.field("field", [&](auto& inner) {
                    inner.text(error.dynamic_field_error().field);
                });
            });
        });
        return;
    }
    if (error.kind() == Kind::RequestIntent) {
        writer.tuple("RequestIntent", [&](auto& out) {
            write_debug(out, error.request_intent_error());
        });
        return;
    }
    if (error.kind() == Kind::MissingRelation) {
        const auto& value = error.missing_relation_error();
        writer.structure("MissingRelation", [&](auto& fields) {
            fields.field("entity", [&](auto& out) {
                out.text(value.entity);
            });
            fields.field("relation", [&](auto& out) {
                out.text(value.relation);
            });
        });
        return;
    }
    if (error.kind() == Kind::OptimisticLockConflict) {
        const auto& value = error.optimistic_lock_conflict_error();
        writer.structure("OptimisticLockConflict", [&](auto& fields) {
            fields.field("entity", [&](auto& out) {
                out.text(value.entity);
            });
            fields.field("id", [&](auto& out) {
                out.text(value.id);
            });
        });
        return;
    }
    constexpr std::string_view names[]{"MissingEntity",
                                       "SqlCompile",
                                       "Behavior",
                                       "Event",
                                       "Policy",
                                       "Check",
                                       "Graph",
                                       "IdGeneration",
                                       "Language",
                                       "UnsupportedLocale",
                                       "Schema",
                                       "Transaction",
                                       "MissingRelation",
                                       "OptimisticLockConflict"};
    writer.tuple(names[static_cast<std::size_t>(error.kind())], [&](auto& out) {
        if (error.kind() == Kind::SqlCompile) {
            write_debug(out, error.sql_compile_error());
        } else if (error.kind() == Kind::Check) {
            out.sequence(error.check_results(), [](auto& inner, const auto& check) {
                write_debug(inner, check);
            });
        } else {
            out.text(error.message());
        }
    });
}

template <class E, class Element>
void write_debug(DebugWriter& writer, const DataServiceError<E>& error, Element&& element) {
    if (static_cast<std::size_t>(error.kind()) >= 3) {
        throw std::invalid_argument("Unknown data-service error diagnostic variant");
    }
    using Kind = typename DataServiceError<E>::Kind;
    constexpr std::string_view names[]{"Runtime", "Entity", "Executor"};
    writer.tuple(names[static_cast<std::size_t>(error.kind())], [&](auto& out) {
        if (error.kind() == Kind::Runtime) {
            write_debug(out, error.runtime_error());
        } else if (error.kind() == Kind::Entity) {
            write_debug(out, error.entity_error());
        } else {
            std::invoke(std::forward<Element>(element), out, error.executor_error());
        }
    });
}

template <class E>
    requires requires(DebugWriter& writer, const E& value) { write_debug(writer, value); }
void write_debug(DebugWriter& writer, const DataServiceError<E>& error) {
    write_debug(writer, error, [](auto& out, const E& value) {
        using teaql::write_debug;
        write_debug(out, value);
    });
}
}
