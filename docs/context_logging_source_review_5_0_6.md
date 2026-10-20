# Complete Context logging and shared-vector source review

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This covers all 406 lines of `teaql-runtime/src/context/logging.rs` and all
138 lines of `context/logging_vectors_tests.rs`. Both complete sources are
accepted under CL01-CL18 with explicit Context/owner-thread adaptations. Current
native15/imported15 and actual original2/formatter4 pass. The historical 5.0.5
logging acceptance is separate; complete G14/G17 and P0-P7 remain open.

## Complete source responsibilities

| ID | Source responsibility | C++ mapping and boundary |
| --- | --- | --- |
| CL01 | Operation model | Five distinct Select/Insert/Update/Delete/Recover values, exact Debug/equality and select-versus-mutation helpers. Batch and Schema are metadata operations mapped later, not invented SqlLogOperation variants. |
| CL02 | Options model | Default all, disabled/select-only/mutation-only/all factories and operation-sensitive gating. Preserve the two independent booleans, Debug and equality. |
| CL03 | Complete SQL entry | Every provenance, operation, intent, trace, SQL/parameter, display, timestamp/duration and result field remains independent. parameter_count counts positions, including projected NULLs, without exposing bindings. Complete Debug is a raw model API, not a safe sink. |
| CL04 | Unified models | Preserve timestamp, optional diagnostic identity, trace and distinct Sql/Info payloads, including full nested Debug/conditional equality and source NaN behavior. No source Debug derive is invented for UnifiedLogBuffer. |
| CL05 | Buffer Clone and ownership | Source Arc/Mutex clones share mutation; same-Context C++ handles borrow registered storage. Destination-owned explicit copies retain nested entries after actual source User destruction without extending that User. Owner-thread confinement replaces shared mutation/lock poisoning, not arbitrary Send/Sync support. |
| CL06 | Context controls | Fluent/set/options/select/mutation/all controls preserve independent switches. Setting disabled options does not erase history; disable_sql_log clears SQL history. C++ fluent calls keep the established borrowed Context API rather than moving an owner. Closed Context calls reject explicitly. |
| CL07 | Retrieval and clear | sql_logs returns an independent allocator-aware snapshot; clear removes logical SQL history. Neither clears a separately registered unified buffer or promises immediate arena reclamation. |
| CL08 | Statement recursion and operation mapping | Recurse ordered child statements and do not also emit their parent. Query maps Select, Insert/Delete/Recover remain distinct, and Update/Batch/Schema map Update. Apply operation gating before formatting, projection, retention or publication. |
| CL09 | Canonical trace | An existing Operation/Provider/Sql path preserves order and IDs while dropping Comment/Purpose/AuditReason nodes. Otherwise use the first nonblank entity, the last nonblank Entity for mutation statements, original Relation order, backend-or-unknown and the exact SQL operation name. Empty/Unicode whitespace and already-canonical identities remain distinct cases. |
| CL10 | Request-owned intent | Latest Comment overrides metadata.comment; Purpose uses its latest trace node. Mutation audit_reason prefers invocation-owned metadata.comment over local AuditReason. Query acquires an audit reason only with local audit lineage; an independent later query does not inherit it. Preserve input lineage and pure latest-node folding. |
| CL11 | Provenance and source preservation | Copy the full SqlLogContext, filling database_kind from backend only when absent. Original bindings, raw debug SQL, trace and metadata remain unchanged. A backend label in a trace is not a new provider implementation. |
| CL12 | Time and result facts | Preserve supplied start/end; reversed time yields zero elapsed. result_count wins over affected_rows, including zero. Exact plural result strings remain source behavior; result_type is absent and missing SQL/debug fields start empty. |
| CL13 | Pretty SQL | Preserve the eight literal keyword replacements and separate AND indentation exactly. This helper is not a parser or a sanitizer and never changes execution SQL. Later privacy projection governs publication. |
| CL14 | Retention and output ordering | Build the sensitive entry, project safe, retain safe SQL, then append safe unified data with supplied start time and diagnostic user. Publish ordinary before sensitive through the manager. An output exception does not erase already retained entries. |
| CL15 | Allocation and shutdown | Per-call entries/copies use the explicit Context. Registered Global buffers receive real destination copies; no lazy service retains a request payload. Actual User close and quiescence precede reclamation; closed/foreign scopes retain their established checks. |
| CL16 | Integration boundary | This module captures supplied ExecutionMetadata; it does not authorize a request or prove that every service/provider caller publishes metadata. Real generated execution, commit/delivery and actor/tenant assembly retain their separate obligations. |
| CL17 | Twelve shared vectors | Preserve exact original JSON bytes, contract/unique IDs, every expected path and separate intent, source immutability and canonical idempotence. Fixture-authored frames are not generated-query/provider execution evidence. |
| CL18 | Root reason and invocation isolation | Retain the original Insert/Update/Delete/Recover/Query matrix, two nested audit reasons, unchanged local lineage, intent removal from canonical paths and an independent follow-up query with no audit reason. |

The current production mapping is `logging.hpp`, `log_model_debug.hpp`, the
Context controls in `context.hpp`, and `src/context/logging.cpp`. Existing
complete runtime-log/model/Context/buffer tests retain their bodies. No runtime
logging algorithm or generated SDK needs editing for this qualification.

## Original source and public C++ observation boundaries

The permanent [reference project](../tests/parity/rust_log_publication_reference_5_0_6/Cargo.toml)
uses unchanged retained observers and actual reference crates. Its Context
observer includes the unchanged private logging implementation against a field/
resource shell; this does not prove the surrounding resource implementation.
The durable gate also executes both original vector tests inside a complete
byte-checked runtime view, with no appended observer. This separately qualifies
the original real Context path instead of relying only on that shell.

The [C++ vector contract](../tests/parity/teaql_runtime/logging_vectors.cpp)
reads an exact-byte [copy of the original fixture](../tests/parity/teaql_runtime/data/sql_trace_path_v1.json).
It calls public record_metadata_log/sql_logs rather than copying a canonical
trace or intent-fold algorithm. Each expected path is also passed through the
public capture path a second time for idempotence. Separate intent is observed
with no owned metadata comment, preserving the source's pure latest-node result.
The five-operation root-reason and independent-query controls use the same public
path. Default PMR allocation is disabled; actual User destruction returns all
charged bytes.

The fixture's PostgreSQL/backend labels remain trace metadata only. Every test
uses SQLite diagnostic rendering and performs no database I/O. These checks do
not add PostgreSQL support or prove generated/provider execution.

## Durable qualification

Run the permanent driver from the repository root with a fresh receipt path:

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --preset modules_native
python3 scripts/verify_log_publication_source_5_0_6.py \
    --record /tmp/teaql_log_publication_source_fresh_5_0_6.json.gz
```

The [execution record](parity/log_publication_durable_execution_final_5_0_6.json.gz)
is terminal and complete. It joins both original Context tests, the four original
formatter tests, complete immutable source families, native and imported
contracts and stable final builds. Existing
implementation/test/corpus bytes remain preserved in the
[before archive](parity/log_publication_durable_before_5_0_6.json.gz) and
[model-header archive](parity/log_publication_model_headers_before_5_0_6.json.gz).
The first native configure's relative-path
[failure](parity/log_publication_initial_configure_failure_5_0_6.json.gz)
is retained separately from its repaired retry.
The [initial test-selector failure](parity/log_publication_initial_driver_failure_5_0_6.json.gz)
also remains preserved: the driver rejected zero selected tests instead of
counting them as success. The corrected selector names the actual original
diagnostic_log_tests module and still requires all four tests. The
[imported fixture-path failure](parity/log_publication_module_fixture_failure_5_0_6.json.gz)
retains native15 and imported14/15; a literal CMake variable in JSON was replaced
by the registry's supported repository-relative argument. Neither failure
required a runtime or test-assertion change.
The [inventory postprocessing record](parity/log_publication_inventory_driver_failure_5_0_6.json.gz)
separately preserves a same-line status assumption rejected by the original
test's `--nocapture` output. All six actual test names are accounted for using
the successful, zero-failure/zero-ignored selection summaries.

The seven matrices contain 327 Context-capture, 425 SQL-privacy, 1,427 audit/model,
172 complete log-model, one shared-buffer Clone, 927 formatter/configuration and
94 isolated manager observations. All **3,373 per frontend** match fresh actual
original, current native and current imported execution. The larger families
overlap adjacent responsibilities; they are not 3,373 distinct Context rules.
The manager's direct replay completes **282 isolated processes**, one original
and one per C++ frontend for each fixture, in addition to selected CTest checks.
Only the retained human UTC, executable-name and one informational-header
adapters apply; no new normalization is introduced.

All **15/15 checks per frontend** pass, including both complete existing log
model/Clone ownership bodies and the new original-vector contract. Successful
builds precede execution and final builds report no work. All **925** canonical
consumer/fixture bodies verify. Exactly **32** archived implementation/test/
corpus/header files remain byte-identical. The original private view preserves
all 67 source/test/fixture files and appends no observer; only disabled optional
exporter dev dependencies are omitted.

The [scoped acceptance receipt](parity/log_publication_source_acceptance_5_0_6.json.gz)
promotes exactly these two sources and log_formatter.rs, bringing dispositions
to **39 accepted / 83 pending / 2 excluded_external**. All seventeen capability
statuses remain unchanged. Native registry **741** is a discovered count, not
a new whole-suite pass; historical failed epochs keep their outcomes.

Complete Context assembly, governed query/save lineage, actual commit/delivery,
G14/G17 and P0-P7 remain open. No production algorithm, generator/SDK, runtime
dependency, new sanitizer/MIPS/full-suite or physical-router acceptance follows
from this source gate.
