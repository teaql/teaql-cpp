# Graph readback source audit: Rust 5.0.6

Status: accepted for the complete two-test source and fixture helpers. This review covers
`teaql-runtime/src/data_service/graph_readback_tests.rs`, source SHA-256
`ac0ec1fdb9f7a37c0d3ddb36a02a5e88c35845c7ae2818153d563e9b77183176`.
Both original tests pass in the [complete original library run](parity/runtime_library_original_5_0_6.json.gz).

## Required fixture and assertions

The controlled executor verifies the exact mutation reason `save fixture 7123`
and the generated `record_key = U64(7123)` filter. It supplies parameterized SQL,
a plain ID binding, actual request trace, SQLite metadata and generated-SQL
provenance. Failure publishes metadata through the supplied observer before
returning an error. Mutation execution is forbidden. Runtime readback code must
create the provenance; the fixture must not fabricate runtime trace frames.

1. Exercise logging enabled/disabled crossed with success/failure. The result's
   error state must match the executor outcome. Logging produces exactly one
   record when enabled and none otherwise. The logged reason is
   `save fixture [REDACTED]`, while the bound U64 ID remains plain. After readback,
   the parent has no query intent snapshot and still has its exact original
   mutation reason.
2. Register `record_key` as the ID, `revision` as version, and logical `note`
   mapped to physical `physical_note`. A complete row with logical `note = NULL`
   succeeds and retains NULL. Removing each of ID, version and note separately
   fails with `missing mapped field Fixture.<field>`. An omitted logical field
   cannot be accepted merely because it could be NULL.

## Located C++ implementation and remaining evidence

`teaql_runtime/include/teaql/runtime/detail/data_service/graph_current_row.hpp`
scopes a copied service, builds the ID filter and captures the readback target.
Its completion path checks mapped logical properties before returning the row.
The access entry is `detail::EntityDataServiceAccess::fetch_graph_current_row_internal`.
These are implementation locations, not proof of the assertions above.

Existing graph save fixtures exercise broader save paths and intent binding, but
no complete counterpart to these two exact source tests was located in this
review. Add an executor fixture using the existing service/executor protocols,
exercise the real readback operation, and preserve success/failure observation
and parent-scope assertions. Qualify the canonical native and imported consumer
bodies before changing this source's pending disposition. Do not infer acceptance
from SQL-executor readback tests, which cover a different layer.

## Isolated native candidate

The staged candidate calls the real internal readback operation with a controlled
executor. It covers all four logging/outcome combinations and all four complete
or missing-column shapes. The parent privacy assertion uses
`EntityDataServiceAccess::has_intent`, which observes the query-log redaction
state: Rust's `query_intent_snapshot` also refers to `query_log_intent`, not the
separate validated request-intent object. Parent request intent is checked
separately for its unchanged reason.

The [first compilation](parity/graph_readback_native_candidate_5_0_6.json.gz)
rejected a copied ExecutionMetadata argument. The observer consumes its metadata,
so the candidate was corrected to move it on the failure path. The
[second candidate](parity/graph_readback_native_candidate_v2_5_0_6.json.gz)
compiles, links and passes all eight outcomes against unchanged native archives.
It creates a new Context for each missing-column shape. The [third candidate](parity/graph_readback_native_candidate_v3_5_0_6.json.gz)
also passes all eight outcomes with one shared Context across the four
missing-column shapes, matching the original test lifetime. It is the candidate
to install. None of these candidates is installed or has canonical imported-module
evidence yet; source acceptance remains pending.

## Final source acceptance

The complete source responsibility reviewed here is accepted under the
[current closure evidence](hydration_trace_core_source_closure_5_0_6.md).
Original and installed native/imported gates are terminal and successful; the
relevant focused concurrency sanitizer evidence is retained with its own scope.
Earlier pending/isolated-candidate statements describe historical checkpoints.
This accepts only the named source, not the enclosing runtime, provider, generator
capability or P0-P7 goal.
