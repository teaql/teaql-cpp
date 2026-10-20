# SQLite live-query isolation source review: Rust 5.0.6

Status: accepted for this complete named source under the existing Context
owner-thread scheduling contract. Original Rust, installed native, canonical
module and focused sanitizer verification pass.

## Locked source

This review covers all 332 lines of
`teaql-provider-sqlite/tests/live_query_trace.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, SHA-256
`465d67b396555f9122d673295f14bed68e5284d3ed855f74adf9d5d5cbb254cc`.
The source has one test, executed with logging disabled and enabled.

The C++ counterpart is
`tests/parity/teaql_provider_sqlite/live_query_trace.cpp`, with the shared
`fixture.hpp`. It observes actual SQLite results and forwards original requests;
it does not insert expected trace nodes into requests or result metadata.

## Complete responsibility mapping

| Rust responsibility | C++ counterpart |
| --- | --- |
| Descriptor lookup over four entities | Registered fixture metadata for `LiveRoot`, `LiveChild`, `LiveDetail`, `LiveLeaf` |
| Lowercase tables; I64 id, version and parent_id | Exact names, roles and types in each descriptor |
| Three successive many relations named children, id to parent_id | Same relation chain and key mapping |
| Provider schema setup and one seeded row per entity | `use_sqlite_provider`, `ensure_schema`, and native executor inserts with id/version/parent_id all 1 |
| Seed reason `seed native live-query fixture` | Exact reason on all four mutations |
| Observer forwards capabilities and query privacy classification | Delegates both protocols to the real SQL executor |
| Snapshot entity, intent and execution trace before executing unchanged request | Context-owned `Fact` fields captured before delegate call; result metadata copied afterward |
| Retain both actual root results before returning either | Two pending operation states own completed native root results; held-root count and observed physical roots are both 2 |
| No Context SQL publication or child work while roots are held | Empty SQL logs and exactly two observations before release in both logging modes |
| Alpha/beta operations share one Context without mixing intent | Two operations on one repository; beta completes before alpha after release |
| Traverse three real nested child lists and leaf identity | Exactly one element at each level, with I64 identity 1 checked at every nested row |
| Eight physical requests; four requests per root intent | Exact total and per-label entity sequence, comment and purpose assertions |
| Relation route lengths are depth when logging, zero otherwise | Exact counts plus every relation entity and `Parent.children` comment checked |
| Physical result counts when logging | Every observed metadata result count is 1 |
| Eight logs when enabled, none otherwise | Exact total and per-label counts |
| Safe trace kinds Operation, Request, relations, Provider, Sql | Exact length and kind at every position |
| Root/request entity LiveRoot, provider sqlite, SQL select | Exact entity assertions at the corresponding positions |

The original schema wrapper and C++ metadata provider use the same entity-name
lookup responsibility. Native connection, metadata, executor, observer and
operation storage retain the existing explicit Context lifetimes and independent
provider/executor/runtime layers.

## Scheduling adaptation and limits

Rust uses scoped worker threads, a mutex/condition variable, a five-second
release deadline and an unwind release guard. It explicitly requests a 32 MiB
stack for each worker. C++ Context allocation and polling are owner-thread
operations, as documented in `commit_audit_source_review_5_0_6.md` and
`transaction_scope_api_contract.md`. The counterpart therefore polls two live
operations cooperatively on the same Context. It holds actual completed SQLite
root results, verifies both operations are pending, then releases them and
completes them in reverse order. Failure unwinds the Context and cancels its
registered operations; there are no waiting worker threads to join.

This preserves the overlapping-result, request-isolation and publication-order
contract under the existing C++ scheduling model. It does not claim unrestricted
cross-thread Context allocation, equivalent thread-stack cost or timing. The
C++ test reports its own held-root/physical-read checks; it does not reuse Rust's
worker-stack metric. Original per-label ordering is checked independently of
which operation finishes first.

## Evidence

- `parity/live_query_trace_original_5_0_6.json.gz`: exact locked integration
  source, 1 passed, 0 failed/ignored/filtered; both original barrier/result
  records are retained, including the explicit Rust worker-stack size.
- `parity/live_query_native_candidate_5_0_6.json.gz`: retained failed initial
  candidate compile. The fixture used a nonexistent ready-operation helper and
  the transport error type instead of the composed SQL executor error type.
- `parity/live_query_native_candidate_v2_5_0_6.json.gz`: corrected candidate
  passes both logging modes. It uses the same pending-state implementation for
  all query results and the executor's actual error type. No production code
  change was needed.
- `parity/live_query_installed_5_0_6.json.gz`: installed native1/module1/
  ASan+UBSan1 pass, each running both logging modes. Canonical consumer and
  fixture bodies, no-work builds, module paths and final source/reference
  guards pass. Leak scanning remains disabled as required by the project.
  The pinned formatter and check pass for all 1,765 handwritten C++ files.

This source review does not qualify the other SQLite privacy/request-log tests,
full generator capabilities, physical-router execution or P0-P7 completion.
