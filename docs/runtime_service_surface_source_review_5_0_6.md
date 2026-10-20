# Runtime service surface and future regression: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
This review covers the complete contents of three files, independently of the
algorithms implemented by their child modules.

| Source | Complete responsibility | C++ mapping and verification |
| --- | --- | --- |
| `data_service/mod.rs` (17 lines) | Declare the nine production child modules and test-only graph-readback module; expose five public types/traits and three crate-internal service types. | `data_service.hpp` composes the child implementations; `runtime.hpp` exposes the five public names. The three infrastructure types remain distinct in `teaql::detail`. Original import-identity checks and native/module compile-time type checks verify the names and separation. Child algorithm acceptance remains independent. |
| `data_service/executor.rs` (6 lines) | Three `GraphTransactionBoundary` variants with Debug, Clone, Copy, PartialEq and Eq. | `graph.hpp` contains the enum; `graph_debug.hpp` renders its variants. Both languages exercise all three values, copies and all nine equality pairs. The native enum is trivially copyable. |
| `data_service/query_future_tests.rs` (84 lines) | The ordinary bound-intent typed query future remains lazy, preserves an unexecuted ID-set observation, produces the expected row, changes that observation only on execution and stays within an 8,192-byte frame budget. | A permanent `runtime_query_future_probe` uses the corresponding metadata, scalar model, intent, projection and limit. It checks zero provider calls before polling, exactly one on completion, the decoded ID/version/name and both ID-set observations. It measures the actual arena-resident `EnhancedEntitiesOperation` body. |

The original module/import observer is appended to the unchanged module source in
a byte-checked `ReferenceRuntimeView`. The original future-size test executes
unchanged. The observer does not implement query behavior, a transaction protocol
or a decoder. Compiler checks exercise the original private imports inside their
crate; the native fixture uses existing infrastructure accessors.

## Language adaptations and limits

Rust measures the concrete future behind its box. C++ measures the concrete
Context-owned operation state, following the existing SQL future-size adaptation.
Both use the same ceiling, but the byte values are not an ABI equivalence claim.
Neither measurement bounds total query allocations, result size or graph work.
The C++ public enhanced entry additionally prepares its query; the supplied bound
intent, complete projection and absence of hooks preserve the original test's
ordinary execution path. No runtime algorithm is changed for this test.

Rust crate visibility maps to the existing C++ internal namespace and trusted
infrastructure accessors. A C++ `detail` namespace is not compiler-enforced Rust
crate privacy and is not an authorization boundary. Public runtime aliases are
checked separately from those internal names. Transaction start/commit/rollback
behavior belongs to the graph and transaction implementations; accepting a
three-value enum cannot accept those implementations.

## Evidence

The [installed receipt](parity/runtime_service_surface_installed_5_0_6.json.gz)
passes one unchanged original test, one import/enum observer, native1/module1,
canonical module consumers and no-work rebuilds. The original Rust future is
5,504 bytes; both C++ operation states are
2,064 bytes in the current native profiles.
All are below the 8,192-byte ceiling; total allocation costs remain unmeasured.
Exact source hashes and unchanged verifier inputs are recorded. Formatting and
baseline/path checks pass. No new sanitizer, MIPS or physical-device claim is made.
The [three-source acceptance record](parity/runtime_service_surface_acceptance_5_0_6.json.gz)
accepts precisely the responsibilities in the table. The initial
fixture compile failures are retained separately: incorrect Context constructor
spelling/double allocator forwarding, missing allocator/debug API declarations,
and direct use of the private intent binder. The corrected fixture uses the
existing `make_user_context`, allocator-aware executor and internal binder.

The earlier [service types review](runtime_service_types_source_review_5_0_6.md)
and [enhanced query continuation](resolved_enhanced_typed_5_0_6.md) retain their
own scopes. This review does not accept `resolved.rs`, relation/graph/save code,
provider implementations, generated SDKs or any still-open P0–P7 exit criterion.
