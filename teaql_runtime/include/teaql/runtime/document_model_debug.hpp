#pragma once
#include <teaql/core/system_time_debug.hpp>
#include <teaql/runtime/document_round_trip.hpp>
#include <teaql/runtime/reference_model_debug.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, DocumentMutationKind value) {
    switch (value) {
    case DocumentMutationKind::Update:
        writer.raw("Update");
        return;
    case DocumentMutationKind::CreateChild:
        writer.raw("CreateChild");
        return;
    case DocumentMutationKind::RemoveChild:
        writer.raw("RemoveChild");
        return;
    }
    throw std::invalid_argument("Unknown document mutation diagnostic variant");
}

inline void write_debug(DebugWriter& writer, DocumentRelationCompleteness value) {
    switch (value) {
    case DocumentRelationCompleteness::Complete:
        writer.raw("Complete");
        return;
    case DocumentRelationCompleteness::Partial:
        writer.raw("Partial");
        return;
    }
    throw std::invalid_argument("Unknown document completeness diagnostic variant");
}

inline void write_debug(DebugWriter& writer, const DocumentEntitySnapshot& value) {
    writer.structure("DocumentEntitySnapshot", [&](auto& fields) {
        fields.field("row_key", [&](auto& out) {
            write_debug(out, value.row_key);
        });
        fields.field("identity", [&](auto& out) {
            write_debug(out, value.identity);
        });
        fields.field("loaded_fields", [&](auto& out) {
            out.sequence(value.loaded_fields, DefaultDebugElement{}, "{", "}");
        });
        fields.field("writable_fields", [&](auto& out) {
            out.sequence(value.writable_fields, DefaultDebugElement{}, "{", "}");
        });
        fields.field("allowed_mutations", [&](auto& out) {
            out.sequence(value.allowed_mutations, DefaultDebugElement{}, "{", "}");
        });
    });
}

inline void write_debug(DebugWriter& writer, const DocumentRelationSnapshot& value) {
    writer.structure("DocumentRelationSnapshot", [&](auto& fields) {
        fields.field("relation_name", [&](auto& out) {
            write_debug(out, value.relation_name);
        });
        fields.field("row_keys", [&](auto& out) {
            out.sequence(value.row_keys, DefaultDebugElement{});
        });
        fields.field("completeness", [&](auto& out) {
            write_debug(out, value.completeness);
        });
        fields.field("writable", [&](auto& out) {
            write_debug(out, value.writable);
        });
        fields.field("allowed_mutations", [&](auto& out) {
            out.sequence(value.allowed_mutations, DefaultDebugElement{}, "{", "}");
        });
    });
}

inline void write_debug(DebugWriter& writer, const DocumentSnapshot& value) {
    writer.structure("DocumentSnapshot", [&](auto& fields) {
        fields.field("model_fingerprint", [&](auto& out) {
            write_debug(out, value.model_fingerprint);
        });
        fields.field("business_id", [&](auto& out) {
            write_debug(out, value.business_id);
        });
        fields.field("scope", [&](auto& out) {
            write_debug(out, value.scope);
        });
        fields.field("aggregate", [&](auto& out) {
            write_debug(out, value.aggregate);
        });
        fields.field("entities", [&](auto& out) {
            out.sequence(value.entities, DefaultDebugElement{});
        });
        fields.field("relations", [&](auto& out) {
            out.sequence(value.relations, DefaultDebugElement{});
        });
    });
}

inline void write_debug(DebugWriter& writer, const ContextualDocument& value) {
    writer.structure("ContextualDocument", [&](auto& fields) {
        fields.field("business_id", [&](auto& out) {
            write_debug(out, value.business_id);
        });
        fields.field("document_token", [&](auto& out) {
            write_debug(out, value.document_token);
        });
        fields.field("body", [&](auto& out) {
            write_debug(out, value.body);
        });
    });
}

inline void write_debug(DebugWriter& writer, const SubmittedContextualDocument& value) {
    writer.structure("SubmittedContextualDocument", [&](auto& fields) {
        fields.field("business_id", [&](auto& out) {
            write_debug(out, value.business_id);
        });
        fields.field("document_token", [&](auto& out) {
            write_debug(out, value.document_token);
        });
        fields.field("command_id", [&](auto& out) {
            write_debug(out, value.command_id);
        });
        fields.field("body", [&](auto& out) {
            write_debug(out, value.body);
        });
    });
}

inline void write_debug(DebugWriter& writer, const DocumentOpenRequest& value) {
    writer.structure("DocumentOpenRequest", [&](auto& fields) {
        fields.field("aggregate_type", [&](auto& out) {
            write_debug(out, value.aggregate_type);
        });
        fields.field("business_id", [&](auto& out) {
            write_debug(out, value.business_id);
        });
        fields.field("purpose", [&](auto& out) {
            write_debug(out, value.purpose);
        });
        fields.field("lifetime", [&](auto& out) {
            write_debug(out, SystemDuration{value.lifetime.seconds, value.lifetime.nanoseconds});
        });
    });
}

inline void write_debug(DebugWriter& writer, const DocumentAcceptRequest& value) {
    writer.structure("DocumentAcceptRequest", [&](auto& fields) {
        fields.field("expected_aggregate_type", [&](auto& out) {
            write_debug(out, value.expected_aggregate_type);
        });
        fields.field("purpose", [&](auto& out) {
            write_debug(out, value.purpose);
        });
        fields.field("submitted", [&](auto& out) {
            write_debug(out, value.submitted);
        });
    });
}

inline void write_debug(DebugWriter& writer, const AcceptedContextualDocument& value) {
    writer.structure("AcceptedContextualDocument", [&](auto& fields) {
        fields.field("document", [&](auto& out) {
            write_debug(out, value.document);
        });
        fields.field("previous_revision", [&](auto& out) {
            write_debug(out, value.previous_revision);
        });
        fields.field("new_revision", [&](auto& out) {
            write_debug(out, value.new_revision);
        });
        fields.field("changed_entities", [&](auto& out) {
            out.sequence(value.changed_entities, DefaultDebugElement{});
        });
    });
}

inline void write_debug(DebugWriter& writer, const VerifiedDocumentSnapshot& value) {
    writer.structure("VerifiedDocumentSnapshot", [&](auto& fields) {
        fields.field("snapshot", [&](auto& out) {
            write_debug(out, value.snapshot);
        });
        fields.field("key_id", [&](auto& out) {
            write_debug(out, value.key_id);
        });
        fields.field("issued_at", [&](auto& out) {
            write_debug(out, value.issued_at);
        });
        fields.field("expires_at", [&](auto& out) {
            write_debug(out, value.expires_at);
        });
    });
}

inline void write_debug(DebugWriter& writer, const DocumentRoundTripError& value) {
    writer.structure("DocumentRoundTripError", [&](auto& fields) {
        fields.field("code", [&](auto& out) {
            write_debug(out, value.code());
        });
    });
}
}
