# Document service dispatch and lifetime qualification

Status: qualified dispatch/replacement/cancellation checkpoint. Native **2/2**,
imported **2/2** and targeted ASan/UBSan **2/2** pass. This is an SQ08 checkpoint;
whole G13 and P0-P7 acceptance remain open.
Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
The [terminal receipt](parity/document_service_async_terminal_reconciled_5_0_6.json.gz)
records all commands, outputs and registered executable hashes.

## Source responsibility

The observer invokes the actual frozen `UserContext::open_document` and
`accept_document` methods with an application-provided implementation of
`ContextBoundDocumentService`. It manually polls the returned Rust futures;
the C++ observer polls the real Context-owned operations. Neither observer
implements an alternate dispatcher or application persistence service.

Both Context entries resolve the installed document service before the trusted
principal when first polled. Missing service therefore takes precedence over
missing principal. Construction without polling does not invoke the service.
The callback receives the complete UserContext, including a separate typed
application resource. Service, principal and application-resource replacement
must be visible to subsequent operations.

## Complete compared observations

The final matrix has **432** inputs: open/accept, zero/one/three preliminary
polls, successful results and all nine document error variants, completion/drop/
Context-close/unpolled paths, and resource replacement after the preceding
operation has completed or been cancelled. Twelve controls cover absent service
and/or principal. Each case records:

- Every open/accept request field, including duration and submitted JSON.
- All trusted-principal fields and the callback's typed Context resource.
- Every returned document field, previous/new revisions and two changed entity
  identities in their original order.
- Service-call and child-poll counts, completion/cancellation counts, forwarded
  wake notification, phase results/errors and final absence of a retained waker.

All 432 actual Rust/C++ observations agree. The
[complete source comparison](parity/document_service_async_complete_comparison_5_0_6.json.gz)
retains complete inputs/outputs and source, dependency and binary hashes.

Unicode, embedded NUL, long service labels and null/object/list bodies exercise
explicit nested copies. The comparator preserves complete JSON types, object
fields, array order and floating-point identity through the existing `same`
function. It performs no result normalization or partial-case exclusion.

The initial 222-case comparison passed but omitted several public fields and
five error variants. Its corpus, observer source text and receipt are preserved
as a retired initial projection. They do not establish the final matrix's
coverage. The refinement changes test stimuli/observations, not runtime behavior.

## Ownership and cancellation adaptations

Rust's borrowed future prevents safe mutable Context replacement or destruction
while that future is live. The source observer drops the future before replacing
resources or dropping its Context. Completed and dropped futures both release
the fixture's retained waker.

C++ uses exactly the existing Global/User ownership domains. The observer's
service and marker belong to UserContext; callback logs and returned observations
are explicitly copied to Global storage. The default PMR resource is disabled.
Every input actually deletes UserContext and requires zero live User bytes and
no retained child wake pointer before publishing the Global observations.

For the close path C++ leaves an operation pending and lets actual Context
destruction cancel it. This is the agreed cancellation/quiescence adaptation,
not a claim that safe Rust permits destruction through a live borrowed future.
The fixture has no background worker; existing foreign-work/async kernel gates
retain their separate responsibility for joining actual native threads.

Resource replacement happens only between operations in this shared comparison.
Rust may destroy an old Arc-held service immediately; C++ arena objects remain
registered until Context close. Callback routing and results agree, while
physical destructor timing follows the explicit C++ ownership contract.

## Reproduction and remaining work

`scripts/verify_document_service_async_5_0_6.py` verifies the frozen reference,
checks exact registry versions/checksums, builds the independent Rust observer
offline/locked, and compares actual Rust and C++ outputs. It saves process
stdout/stderr and exit codes before propagating a failure. The committed corpus
comes from actual Rust results only after the comparison succeeds.

```bash
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_document_service_async_probe \
    teaql_document_round_trip_tests
ctest --preset native --output-on-failure \
    -R '^(document_service_async_rust_parity|document_round_trip_context_contracts)$'
```

Native, imported and targeted sanitizer gates are recorded separately. This
checkpoint does not require new production exports or generated artifacts.
All **829** canonical consumer/fixture bodies pass the module verifier. Formatting
and its check pass for **1,652** handwritten C++ files. The three production
dispatch/Context/async inputs hashed before and after this checkpoint are byte
identical. No new generator, runtime or dependency implementation is added.

The initial terminal receipt collected all six successful gate results but then
used the wrong module executable directory while hashing artifacts. Its failure
is preserved; the final receipt resolves the actual executable from CTest's
registered command and reuses those terminal results without rerunning the gates.
Sanitizer leak scanning is disabled. No MIPS or physical-device run is added.

The [model and collection/token checkpoint](document_model_diagnostics_5_0_6.md)
retains its own scope. The later
[whole protocol acceptance](document_reference_acceptance_5_0_6.md) completes the
custom key/clock/nonce/authorization-provider and source-disposition review for
SQ08. Application document projection, editing and persistence remain
application-owned; whole G13/P7 is still open.
