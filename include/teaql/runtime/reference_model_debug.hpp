#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/runtime/round_trip_reference.hpp>

namespace teaql {
inline void write_debug(DebugWriter& writer, UtcDateTime value) {
    writer.raw(value.date_naive().to_string(writer.memory_resource()));
    writer.raw("T");
    auto seconds = value.timestamp() % 86400;
    if (seconds < 0) {
        seconds += 86400;
    }
    auto nanos = value.timestamp_subsec_nanos();
    auto second = static_cast<unsigned>(seconds % 60);
    if (nanos >= 1'000'000'000) {
        ++second;
        nanos -= 1'000'000'000;
    }
    auto digits = [&](unsigned number, unsigned width) {
        std::array<char, 16> buffer{};
        auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number);
        auto length = static_cast<unsigned>(result.ptr - buffer.data());
        for (auto i = length; i < width; ++i) {
            writer.raw("0");
        }
        writer.raw(std::string_view(buffer.data(), length));
    };
    digits(static_cast<unsigned>(seconds / 3600), 2);
    writer.raw(":");
    digits(static_cast<unsigned>(seconds / 60 % 60), 2);
    writer.raw(":");
    digits(second, 2);
    if (nanos) {
        writer.raw(".");
        if (nanos % 1'000'000 == 0) {
            digits(nanos / 1'000'000, 3);
        } else if (nanos % 1'000 == 0) {
            digits(nanos / 1'000, 6);
        } else {
            digits(nanos, 9);
        }
    }
    writer.raw("Z");
}

namespace detail {
template <class T>
void write_reference_optional(DebugWriter& writer, const std::optional<T>& value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
}
}

inline void write_debug(DebugWriter& writer, const EntityReferenceClaims& value) {
    writer.structure("EntityReferenceClaims", [&](auto& fields) {
        fields.field("entity_type", [&](auto& out) {
            write_debug(out, value.entity_type);
        });
        fields.field("id", [&](auto& out) {
            write_debug(out, value.id);
        });
        fields.field("version", [&](auto& out) {
            write_debug(out, value.version);
        });
        fields.field("issued_at", [&](auto& out) {
            write_debug(out, value.issued_at);
        });
        fields.field("expires_at", [&](auto& out) {
            write_debug(out, value.expires_at);
        });
        fields.field("purpose", [&](auto& out) {
            write_debug(out, value.purpose);
        });
        fields.field("key_version", [&](auto& out) {
            write_debug(out, value.key_version);
        });
    });
}

inline void write_debug(DebugWriter& writer, const EntityReferenceTokenError& value) {
    writer.structure("EntityReferenceTokenError", [&](auto& fields) {
        fields.field("code", [&](auto& out) {
            write_debug(out, value.code());
        });
    });
}

inline void write_debug(DebugWriter& writer, const RoundTripReferenceError& value) {
    writer.structure("RoundTripReferenceError", [&](auto& fields) {
        fields.field("code", [&](auto& out) {
            write_debug(out, value.code());
        });
    });
}

inline void write_debug(DebugWriter& writer, const TrustedReferencePrincipal& value) {
    writer.structure("TrustedReferencePrincipal", [&](auto& fields) {
        fields.field("authentication_realm", [&](auto& out) {
            write_debug(out, value.authentication_realm);
        });
        fields.field("subject", [&](auto& out) {
            write_debug(out, value.subject);
        });
        fields.field("domain_root_type", [&](auto& out) {
            write_debug(out, value.domain_root_type);
        });
        fields.field("domain_root_id", [&](auto& out) {
            write_debug(out, value.domain_root_id);
        });
    });
}

inline void write_debug(DebugWriter& writer, const ReferenceDocumentScope& value) {
    writer.structure("ReferenceDocumentScope", [&](auto& fields) {
        fields.field("document_id", [&](auto& out) {
            write_debug(out, value.document_id);
        });
        fields.field("purpose", [&](auto& out) {
            write_debug(out, value.purpose);
        });
        fields.field("aggregate_type", [&](auto& out) {
            write_debug(out, value.aggregate_type);
        });
        fields.field("aggregate_id", [&](auto& out) {
            write_debug(out, value.aggregate_id);
        });
        fields.field("aggregate_revision", [&](auto& out) {
            write_debug(out, value.aggregate_revision);
        });
    });
}

inline void write_debug(DebugWriter& writer, const ReferenceIdentity& value) {
    writer.structure("ReferenceIdentity", [&](auto& fields) {
        fields.field("entity_type", [&](auto& out) {
            write_debug(out, value.entity_type);
        });
        fields.field("id", [&](auto& out) {
            write_debug(out, value.id);
        });
        fields.field("version", [&](auto& out) {
            write_debug(out, value.version);
        });
    });
}

inline void write_debug(DebugWriter& writer, const ReferenceKey& value) {
    writer.structure("ReferenceKey", [&](auto& fields) {
        fields.field("key_id", [&](auto& out) {
            write_debug(out, value.key_id);
        });
        fields.field("master_key", [&](auto& out) {
            write_debug(out, std::string_view("[REDACTED]"));
        });
    });
}

inline void write_debug(DebugWriter& writer, DeploymentProfile value) {
    switch (value) {
    case DeploymentProfile::Development:
        writer.raw("Development");
        return;
    case DeploymentProfile::Test:
        writer.raw("Test");
        return;
    case DeploymentProfile::Production:
        writer.raw("Production");
        return;
    }
    throw std::invalid_argument("Unknown reference diagnostic variant");
}

inline void write_debug(DebugWriter& writer, ReferenceMode value) {
    switch (value) {
    case ReferenceMode::Governed:
        writer.raw("Governed");
        return;
    case ReferenceMode::Raw:
        writer.raw("Raw");
        return;
    }
    throw std::invalid_argument("Unknown reference diagnostic variant");
}

inline void write_debug(DebugWriter& writer, const ExternalEntityReference& value) {
    if (const auto* token = std::get_if<String>(&value.data)) {
        writer.tuple("Governed", [&](auto& out) {
            out.text(*token);
        });
    } else {
        const auto raw = std::get<RawEntityReference>(value.data);
        writer.structure("Raw", [&](auto& fields) {
            fields.field("id", [&](auto& out) {
                out.integer(raw.id);
            });
            fields.field("version", [&](auto& out) {
                out.integer(raw.version);
            });
        });
    }
}

inline void write_debug(DebugWriter& writer, const ResolvedEntityReference& value) {
    writer.structure("ResolvedEntityReference", [&](auto& fields) {
        fields.field("identity", [&](auto& out) {
            write_debug(out, value.identity);
        });
        fields.field("mode", [&](auto& out) {
            write_debug(out, value.mode);
        });
        fields.field("key_id", [&](auto& out) {
            detail::write_reference_optional(out, value.key_id);
        });
        fields.field("issued_at", [&](auto& out) {
            detail::write_reference_optional(out, value.issued_at);
        });
        fields.field("expires_at", [&](auto& out) {
            detail::write_reference_optional(out, value.expires_at);
        });
    });
}

inline void write_debug(DebugWriter& writer, const ReferenceStartupNotice& value) {
    writer.structure("ReferenceStartupNotice", [&](auto& fields) {
        fields.field("level", [&](auto& out) {
            out.text(value.level);
        });
        fields.field("message", [&](auto& out) {
            out.text(value.message);
        });
        fields.field("deployment_profile", [&](auto& out) {
            out.text(value.deployment_profile);
        });
        fields.field("telemetry_key", [&](auto& out) {
            out.text(value.telemetry_key);
        });
        fields.field("telemetry_value", [&](auto& out) {
            out.text(value.telemetry_value);
        });
        fields.field("response_headers", [&](auto& out) {
            out.sequence(value.response_headers, [](auto& inner, const auto& header) {
                inner.sequence(
                    std::array{header.first, header.second},
                    [](auto& item, auto text) {
                        item.text(text);
                    },
                    "(",
                    ")");
            });
        });
    });
}

inline void write_debug(DebugWriter& writer, ReferenceWireCodec) {
    writer.raw("ReferenceWireCodec");
}
}
