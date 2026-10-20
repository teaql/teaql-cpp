# Hydration, trace and core source qualification: Rust 5.0.6

Status: all listed original and installed gates are complete. The five named
source dispositions are accepted within the specific boundaries below.

## Complete source responsibilities under review

| Locked source | Complete responsibility review | Original evidence | C++ evidence |
| --- | --- | --- | --- |
| `teaql-core/src/lib.rs` | [Current root audit](core_root_source_audit_5_0_6.md), all 26 root tests, current exports and hidden JSON binding; historical review retained as historical | `core_root_original_5_0_6.json.gz`: 69 library tests including all 26 root tests | Four canonical fixtures within native7/module7 |
| `teaql-runtime/tests/support/allocation_counter.rs` | [Counter review](allocation_counter_source_review_5_0_6.md), all 47 lines and C++ measurement boundaries | Original hydration and retained-cache consumers; current full runtime library run is supplemental | Real per-thread Context counters, overlapping/concurrent controls, all consumers; native10/module10 and focused sanitizer1 |
| `teaql-runtime/tests/hydration_allocations.rs` | [Hydration review](hydration_source_review_5_0_6.md), complete helper and 24-row grid | `hydration_allocation_original_exploration_5_0_6.json.gz`, plus unchanged Plain helper input experiment | Complete current native and imported 24-row grids, original ceilings, helper and move/retention controls |
| `teaql-runtime/tests/trace_chain_requests.rs` | [Trace source audit](trace_chain_requests_source_audit_5_0_6.md), all seven tests, real planning and no-I/O/Policy controls | `trace_chain_requests_original_5_0_6.json.gz`: all seven unchanged tests | Installed direct fixture, native10/module10 and current counter/trace continuation |
| `teaql-runtime/src/data_service/graph_readback_tests.rs` | [Graph readback audit](graph_readback_source_audit_5_0_6.md), both source tests and executor/observer helpers | Both named tests in `runtime_library_original_5_0_6.json.gz` | Four logging/outcome cases and four mapped-column cases in native7/module7 |

All receipt names in this table are under `docs/parity/`. Test counts describe
the invoked executable selections; they are not numbers of accepted source files
or proof of capabilities outside the reviewed source.

## Terminal installed completion fixtures

`source_completion_module_exports_verification_5_0_6.json.gz` is complete:
native7/module7 pass, including both complete hydration output grids. Canonical
consumer preservation, no-work rebuilds, binary hashes and final captured
source/reference identities pass. The only production change since the preceding
failed module compilation is the existing numeric formatting helper's module
visibility; no function body changed.

The separate `hydration_trace_core_source_closure_5_0_6.json.gz` rebuilds the
installed counter and trace consumers against that current module interface,
then executes native2/module2 and final guards. Until it finishes, its running
state is not evidence. No full sanitizer suite or MIPS run is needed for this
module declaration change; the previous focused concurrency sanitizer result is
retained with its actual scope.

## Preserved failures and limits

The retained discovery failure concerns CTest omitting a not-yet-built command.
The retained output-capture failure concerns truncated successful JUnit output.
The retained module compilation failure exposed a missing append_float export.
Each has its own immutable receipt and a subsequent continuation; none is
rewritten into a passing record.

This group does not accept the private test-only `memory.rs` helper, complete
runtime root, all graph/data-service/provider sources, performance equivalence,
external integrations, physical-router execution, any remaining generator
capability, or P0-P7 as a whole. The private memory helper has additional current
caller observations but remains a separate pending responsibility review.

## Final disposition

The current counter/trace continuation is complete: native2/module2 pass with
canonical-body, no-work and final source/reference checks. Together with the
reviewed original and installed completion evidence above, the five named sources
advance to accepted. Active source totals become **99 accepted / 23 pending /
2 excluded_external**. Capability totals remain **11 qualified / 6 partial**;
P0-P7 remains incomplete. Earlier running-state paragraphs retain their checkpoint
context and are superseded by this final disposition.
