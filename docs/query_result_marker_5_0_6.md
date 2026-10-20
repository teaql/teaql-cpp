# General request result markers: Rust 5.0.6

The [subsequent generated migration](generated_query_result_marker_5_0_6.md)
implements the generator follow-up below. This document retains the earlier
runtime-first checkpoint and its original evidence boundaries.

Status: shared runtime repaired; generated facade migration remains required.
G06, G09 and the P0–P7 goal remain open. This checkpoint corrects a source
guard discrepancy discovered during the general-query responsibility review.

## Original responsibility

The locked `request/index.stg` declares `Request<R>` and its ordinary methods
under unconstrained `impl<R>`. `return_type<T>` transfers request state without
requiring `T: Entity`. Clone, query/options inspection, consuming conversion,
matching, enhancement and relation selection can operate on these requests.
The marker does not represent a materialized result object at this stage.

The **public** `PurposedQuery<Request<R>>` implementation has the enclosing
`R: teaql_core::Entity` guard. It governs all eight execution entries and
`new_entity`, including compact rows/count/exists whose internal helpers do not
decode `R`. Reviewing those internal helpers alone would miss this requirement.
An arbitrary marker does not authorize scalar execution or entity creation.

The [complete source review](parity/query_result_marker_source_review_5_0_6.json)
retains twelve full ordinary method bodies and the entire guarded public
implementation at generator `45e1572c4956ca963410d8ea436ead3c8857087a`.
The runtime reference remains `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Runtime change

`EntityQuery<Source, Executor, Purposed, Result>` now accepts an unconstrained
result marker while retaining `Entity` for Source. `returnType<Result>()` and
typed child-query conversions preserve that distinction. All nine public
execution/creation methods explicitly require `Entity<Result>` and purpose;
the existing stream-executor constraint remains intact.

No marker instance is constructed, destroyed or allocated. Existing request
roots, nested allocator propagation, checked Context identities, snapshots,
explicit destination copies and Global rebinding are unchanged. A deleted
marker constructor/destructor is accepted by the runtime ownership tests.
Restoring an Entity result preserves the source model and permits execution.

The [before-state archive](parity/query_result_marker_runtime_before_5_0_6.json.gz)
preserves the actual initial compiler refusal and complete previous runtime
header. The source verifier reconstructs the allowed constraint/comment change,
formats it with the pinned tool and requires exact equality with the current
header. This check proves the bounded implementation change; actual behavior
and compiler checks provide separate evidence.

## Behavioral and compiler evidence

The new observer uses the complete unchanged original generated dynamic-JSON
domain, existing locked dependency graph and actual public request APIs.
Its 144 constructions cross Person/Order, default/minimal entries, integer/tuple/
custom non-Entity markers, ordinary/empty/Unicode-NUL text and four plain/options/
child/combined shapes. Child requests also have a custom non-Entity marker.
The eight complete fields are bare query, options, relation selections, relation
filters, child enhancements, consuming core query, metadata-applied core query
and complete QuerySelection. No output normalization is used.

The native observer deliberately enters through `runtimeQuery().returnType<T>()`
because generated `returnType<T>()` still imposes the early Entity constraint.
This comparison qualifies shared mechanics, not generated application parity.
It performs construction and inspection only; it is not physical SQL evidence.

Compiling the same Rust observer with `--cfg teaql_marker_negative` produces
nine actual E0599 refusals for list/rows/stream/first/one/page/count/exists and
creation. C++ compile-time availability checks reject all nine corresponding
runtime operations. Ownership controls verify independent lvalue adaptation,
moved-from rejection, cross-Context composition refusal, explicit copies,
Global retention/rebinding and access after actual UserContext destruction.
A restored Entity marker executes a real SQLite count through the generated
runtime setup and existing service/provider layers.

| Completed gate | Result |
| --- | --- |
| Original/native runtime state comparison | 144/144, eight complete fields per case |
| Original compiler | Positive observer compiles; all nine execution/creation calls refuse non-Entity markers |
| Relevant native regression | 4/4 |
| Imported module checks | 2/2, including the complete new corpus |
| Targeted ASan/UBSan | 2/2, including the complete new corpus; leak scanning disabled |
| Canonical module consumers | 781 bodies |
| Module export/path checks | 1,495 exports across seven packages; active paths verified |
| Pinned formatting | 1,630 handwritten C++ files formatted and checked |
| Locked producer ledger | 36 sources / 17 capabilities verified; ten capabilities remain unqualified |

The accepted native run rebuilt all four selected consumers after formatting
completed. Earlier rehearsal and observation logs remain archived separately;
they are not the final stable-build evidence. The independent generator is
unchanged in this checkpoint, so Java and SDK regeneration gates are deferred
to the required adapter migration rather than reported as new results.

## Required generator follow-up

1. Change generated Request result/friend/child-result and `returnType` template
   parameters to preserve unconstrained construction and source conversions.
   Keep source model constraints and execution/creation Entity guards.
2. Replace the old `return_type_not_entity` negative compiler expectation with
   a positive construction/composition case and nine execution/creation
   refusals. Preserve the former expectation as historical evidence.
3. Emit accurate Assist explaining marker construction versus execution.
   Regenerate every affected SDK with a controlled signature/constraint change
   proof; preserve unrelated bodies and historical qualification records.
4. Execute the same original corpus through generated facades, then run the
   relevant Java, compiler, native/module, lifetime and strict reproduction
   gates. Runtime-only acceptance does not waive these obligations.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
cargo build --offline --locked \
    --manifest-path tests/parity/rust_request_state_5_0_6/Cargo.toml \
    --bin query_result_marker
cmake --build --preset native --target \
    teaql_query_result_marker_probe teaql_generated_request_state_tests
python3 scripts/verify_query_result_marker_source_5_0_6.py --check-original-compiler
python3 tests/parity/compare_query_result_marker_5_0_6.py \
    --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/query_result_marker \
    --cpp build_native_current/teaql_query_result_marker_probe
ctest --preset native --output-on-failure \
    -R '^(query_result_marker_runtime_rust_parity|generated_request_state_context_contracts)$'
```

Completed command output, source identities and gate results are recorded in
[the terminal record](parity/query_result_marker_runtime_terminal_5_0_6.json.gz).
This checkpoint adds no dependency and makes no full-suite, MIPS, GPU, external
integration, physical-device or complete generator-parity claim.
