#pragma once
#include <teaql/core/business_id.hpp>
#include <teaql/core/debug.hpp>

namespace teaql {
// Raw core diagnostics; safe log projection remains a separate responsibility.
inline void write_debug(DebugWriter& writer, BusinessIdErrorCode value) {
    writer.raw(business_id_error_code_name(value));
}

inline void write_debug(DebugWriter& writer, const BusinessIdError& value) {
    writer.structure("BusinessIdError", [&](auto& fields) {
        fields.field("code", [&](auto& out) {
            write_debug(out, value.code);
        });
        fields.field("message", [&](auto& out) {
            out.text(value.message);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BusinessIdDefinition& value) {
    writer.structure("BusinessIdDefinition", [&](auto& fields) {
        fields.field("field_name", [&](auto& out) {
            out.text(value.field_name);
        });
        fields.field("profile", [&](auto& out) {
            out.text(value.profile);
        });
        fields.field("prefix", [&](auto& out) {
            out.text(value.prefix);
        });
        fields.field("namespace", [&](auto& out) {
            out.text(value.namespace_name);
        });
        fields.field("split_by_date", [&](auto& out) {
            out.boolean(value.split_by_date);
        });
        fields.field("preserve_digits", [&](auto& out) {
            out.integer(value.preserve_digits);
        });
        fields.field("separator", [&](auto& out) {
            out.text(value.separator);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BusinessIdScope& value) {
    writer.structure("BusinessIdScope", [&](auto& fields) {
        fields.field("domain_root_key", [&](auto& out) {
            out.text(value.domain_root_key);
        });
        fields.field("aggregate_type", [&](auto& out) {
            out.text(value.aggregate_type);
        });
        fields.field("namespace", [&](auto& out) {
            out.text(value.namespace_name);
        });
        fields.field("period_key", [&](auto& out) {
            out.text(value.period_key);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BusinessIdGenerationRequest& value) {
    writer.structure("BusinessIdGenerationRequest", [&](auto& fields) {
        fields.field("definition", [&](auto& out) {
            write_debug(out, value.definition);
        });
        fields.field("domain_root_key", [&](auto& out) {
            out.text(value.domain_root_key);
        });
        fields.field("aggregate_type", [&](auto& out) {
            out.text(value.aggregate_type);
        });
        fields.field("business_date", [&](auto& out) {
            write_debug(out, value.business_date);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BusinessIdPlan& value) {
    writer.structure("BusinessIdPlan", [&](auto& fields) {
        fields.field("scope", [&](auto& out) {
            write_debug(out, value.scope);
        });
        fields.field("prefix", [&](auto& out) {
            out.text(value.prefix);
        });
        fields.field("date_text", [&](auto& out) {
            if (value.date_text) {
                out.tuple("Some", [&](auto& inner) {
                    inner.text(*value.date_text);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("preserve_digits", [&](auto& out) {
            out.integer(value.preserve_digits);
        });
        fields.field("separator", [&](auto& out) {
            out.text(value.separator);
        });
        fields.field("initial_sequence", [&](auto& out) {
            out.integer(value.initial_sequence);
        });
        fields.field("max_sequence", [&](auto& out) {
            out.integer(value.max_sequence);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BusinessIdAllocation& value) {
    writer.structure("BusinessIdAllocation", [&](auto& fields) {
        fields.field("scope", [&](auto& out) {
            write_debug(out, value.scope);
        });
        fields.field("sequence", [&](auto& out) {
            out.integer(value.sequence);
        });
    });
}

inline void write_debug(DebugWriter& writer, const BusinessIdValue& value) {
    writer.tuple("BusinessIdValue", [&](auto& out) {
        out.text(value.value);
    });
}
}
