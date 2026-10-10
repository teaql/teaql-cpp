# Data-service root responsibility review

The complete source API contract is now accepted; see the current evidence in
[data-service-api-contract.md](data-service-api-contract.md). The partial/pending
statements below record the preceding review checkpoints. Provider/runtime
composition retains its independent gates.

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-data-service/src/lib.rs` (393 lines, including its one source test).

The complete production-body follow-up separately reviews the first 310 lines
before cfg(test), keeping all executor/transaction/schema/ID/descriptor roles
and the source test distinct. The four inherited observed defaults delegate
without creating observer callbacks or execution metadata; query_log_intent
retains its separate pure conservative fallback. The associated transaction
must preserve the executor Error and query/mutation/terminal capabilities.
Native Context pending-operation/consumption guards adapt Rust ownership and
borrows; optional stream/guard/schema/ID SPIs are not silently advertised on an
erased transaction. These declarations and earlier matrices do not accept
provider cancellation or runtime composition. The source group remains partial.
This review accounts for the complete production body. The group remains partial;
it must preserve all source capabilities, not just successful SQL execution.

| Source responsibility | C++ mapping and current gate |
| --- | --- |
| DataServiceCapabilities | Eight independent capability flags, all false by Default, scalar Clone and raw Debug. Extra C++ equality is an extension, not a source derive. |
| QueryRequest | Independent query, trace chain, optional comment and both diagnostic capture flags. Clone copies every selected scope; request annotation does not merge into query annotation. |
| QueryResult | Ordered CompactRows and independent ExecutionMetadata, with explicit nested destination copies and source Debug. |
| MutationRequest | Five separate Insert/Update/Delete/Recover/recursive Batch alternatives. trace_chain returns each leaf's chain; comment borrows the final node even when empty. Batch has no own trace/comment. |
| GuardedMutationRequest | Mutation and guard remain distinct. The provider must enforce the guard atomically in the write statement. Construction, Clone and raw Debug do not authorize a pre-read implementation. |
| MutationResult | affected_rows, GeneratedValues, optional persisted EntitySnapshot and metadata remain distinct. Generated fields are not ordinary input values or a guessed snapshot. |
| DataServiceOperation | Seven separate scalar cases with Copy/Clone/PartialEq/Eq and Debug; the field is independent from capability flags. |
| ExecutionMetadata | Every statement/log/backend/operation/time/count/trace/comment/request-ID/query/parameter/debug field is preserved. unrecorded_query has epoch times and only a present result_count. Full source time representation is repaired below; current consumer integration remains open. |
| StreamChunk | Ordered rows, chunk_index and is_last, with Clone and Debug. A chunk marker is independent from stream end/error/cancellation. |
| SchemaRequest / SchemaResult | Independent entity name and changed flag with Clone/Debug. No source Eq/Default/serde capability is invented. |
| DataServiceExecutor | Separate typed Error and capabilities SPI. Structured C++ errors adapt Rust Error bounds without requiring exception inheritance. |
| QueryExecutor | query and pure no-I/O query_log_intent; observed default delegates query and ignores the optional failure observer. It does not invent success notification or metadata. |
| StreamQueryExecutor | QueryStream plus observed default forwarding the exact request/chunk size; default does not manufacture diagnostics. |
| MutationExecutor / GuardedMutationExecutor | Separate mutate/atomic guarded mutate SPIs; observed defaults forward to their corresponding operation. |
| TransactionExecutor / Transaction | Associated transaction retains query/mutation/commit/rollback responsibilities. Terminal calls consume access; Context close cancels/joins native work. Additional concrete guarded/streaming capabilities are recovered explicitly, not advertised on every transaction. |
| SchemaExecutor / IdGeneratorExecutor | Independent ensure_schema and next_id futures, with source error/result boundaries. |
| SchemaProvider | Optional descriptor lookup; C++ descriptors borrow their explicitly selected Context instead of extending ownership through Arc. |
| ExecutionObserver / QueryStream | Scoped callbacks/streams preserve suspension, wake, failure and cancellation. Native work cannot mutate the thread-confined arena from a worker. |

The source models derive Debug/Clone as listed. Only capabilities derives Default;
only DataServiceOperation derives Eq/Copy. The source root models have no serde
derive. Diagnostic JSON in fixtures is an observation envelope, not a new
production serialization protocol. The separately owned sql_log module retains
its own provenance/private-state responsibilities and acceptance gate.

## Reproduced diagnostic omission and root facade

The earlier public compile fixture fails to find DataServiceCapabilities Debug.
The current model_debug.hpp provides all original root-model writers, retaining
field order, nested core models and private log placeholders. data_service.hpp
exposes the separate model/SPI root without provider/runtime dependencies. The
root export verifier now checks direct data-service declarations as well as its
log reexports; declaration checks remain weaker than semantic acceptance.

## Scoped validation

Forty additional source calls live in uniquely named model_scope fixtures. They
compile the original public model construction/Clone/Debug operations, normalize
no observations, and compare nine formatter modes with the current C++ writers.
All 40 native observations match live Rust. Names are separate from concurrently
maintained common model fixtures so their schemas do not overwrite each other.

The native scoped contract disables default PMR, retains full metadata/request
and reference diagnostics through User's Global allocation proxy, destroys User,
asserts every source block reclaimed, rebinds retained Global references and
checks nested allocation domains and complete qualified-facade diagnostics. Raw
bindings remain visible in raw Debug; private projection/intent payloads do not.
Native scoped contracts and standalone ASan/UBSan with leak scanning disabled
pass. The two named sanitizer/MIPS targets are registered and building; cross
and named integration remain pending for these additional fixtures. Existing executor/source contracts still
need complete default/ownership/composition acceptance.

## Reproduced SystemTime range gap and current repair

Rust ExecutionMetadata uses Unix SystemTime with signed 64-bit seconds plus
nanoseconds. C++ previously aliased std::chrono::system_clock::time_point, whose
signed 64-bit nanosecond count has a much smaller range. The same narrowing flows
into SqlLogEntry/UnifiedLogEntry and elapsed calculations. The native input
domain in current diagnostic corpora is explicit; passing those corpora cannot
close this gap.

The existing runtime SystemTime scalar already stores seconds/nanoseconds. A
repair must place a shared scalar below data-service/runtime, preserve ordinary
chrono input adapters, calculate full-span unsigned durations without overflow,
retain epoch/reversed-time behavior, and update metadata/log/diagnostic call
sites. It must not create a data-service dependency on runtime or clamp/drop
valid source times. Layout changes require a coherent consumer rebuild; the
current live builds retain their current layout until that repair is prepared.

## Integration checkpoint, 2026-10-08

The preceding final seventeen-check entity/Timestamp/Decimal/locking/source-tail
selection passes 17/17 under ASan/UBSan with leak scanning disabled (11.12 seconds)
and 17/17 on MIPS/QEMU (8.53 seconds). It includes the last inherited Decimal
identity methods and source fixtures. The additional data-service scope targets
are compiled afterward and retain their own pending acceptance limits. Full
native consumer integration remains running; new target registration requires
a final incremental refresh before complete regression.

The range gap is now reproduced directly, not inferred only from type sizes.
execution_time_range_scope_oracle calls the actual original metadata/SystemTime
API and accepts both +/-10,000,000,000 seconds, INT64_MAX seconds plus
999,999,999 nanoseconds, and INT64_MIN seconds. Clone preserves every value.
The committed source observations retain exact original Debug and acceptance.
The current native metadata probe reports a seconds domain of
-9,223,372,037 through 9,223,372,036; the existing separate wide scalar retains
all four seconds inputs. No overflow conversion is performed to manufacture an
apparent native success. A coherent common-scalar/layout repair remains required.

The two additional named scoped checks now pass 2/2 under ASan/UBSan with leak
scanning disabled (7.48 seconds) and 2/2 on MIPS/QEMU (6.01 seconds). All forty
MIPS observations additionally match live original Rust. Native named/full
consumer integration remains pending. These results preserve the explicit
SystemTime range gap and do not accept the whole data-service source group.

Current source inspection confirms the common core seconds/nanoseconds scalar
is now used by metadata/log fields and direct diagnostics. Subsecond checked
arithmetic, assignment and safe native duration adapters are implemented and
reviewed in system-time-arithmetic-contract.md. All 1,232 new direct Rust
observations match current native C++; native and standalone sanitizer scope
contracts pass. Final consumer/platform integration remains open. The earlier
range reproduction remains historical evidence, not the current storage verdict.

The complete stable standard-time surface review now preserves associated/module
constexpr epoch constants, typed reversed-duration errors, native hashing and
core-owned diagnostics. The 7,056-pair original error corpus matches native;
post-epoch selected time/error/model/logging checks pass MIPS/QEMU 8/8 and
ASan/UBSan 8/8 with leak scanning disabled. See the arithmetic contract for exact
evidence and remaining scope. The complete original SPI body is rechecked against
the native interfaces: all observer defaults delegate to the matching mandatory
operation; schema, ID, guard, transaction and stream responsibilities remain
distinct. Source reading and these selections do not replace generic
cancellation/provider/descriptor/transaction composition or full native
regression. This group remains partial.

## Direct observed-default protocol evidence

The independent executor_defaults_scope fixture implements only mandatory query,
stream, mutation and guarded-mutation SPIs. It calls the actual pinned inherited
observed defaults. It does not copy their bodies or override them. All 144 live
original/native observations match: three entity names (including Unicode and a
long name), four diagnostic flag combinations, optional observer presence,
polled/unpolled outcomes and chunk sizes zero, one and seventeen.

The observation retains all four raw received-request diagnostics, dispatch
counts before polling, poll counts afterward, untouched observer counters and
terminal error/end alternatives. Each mandatory method is invoked once before
polling. Each queried/mutated/guarded future returns a fixture failure; the empty
stream returns EOF. Unpolled defaults invoke no polls or observer callbacks.
Error payloads are deliberately outside this fixture: Rust uses an IO error,
while C++ uses an opaque fixture status and compares only the error alternative.
This does not accept generic Error bounds or provider-specific errors.

The registered parity check passes ASan/UBSan with leak scanning disabled in
1.53 seconds and MIPS/QEMU in 1.18 seconds. Selected builds finish first; all 96
sanitizer and 307 MIPS compiled dependency records are nonempty. Native fixture
states borrow stack counters, so explicit cancellation quiesces operations
before those counters leave scope. The original mandatory fixture contains no
native work, SQL I/O or manufactured metadata. Transaction/schema/ID defaults,
actual provider failures, cancellation metadata and whole runtime composition
retain their separate acceptance gates.

The shared native full refresh stops at a reference-probe link failure. Required
definitions are present in its object/archive files on subsequent inspection,
and Ninja reports recovery from a premature log ending. The cause is not proven
to be source code. A separate build-root-integration native directory is configured
for fresh whole-project object/archive acceptance. Existing shared-directory
source changes and generated artifacts are preserved; no source group or stage
is accepted from a stopped or cached build.
