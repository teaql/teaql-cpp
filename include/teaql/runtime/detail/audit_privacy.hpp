#pragma once
#include <teaql/runtime/event.hpp>
namespace teaql::detail {
bool audit_plaintext_enabled(Memory);
// Log projection: does not mutate the original event or its execution values.
RawAuditEvent audit_event_projection(Memory,const RawAuditEvent&,bool allow);
}
