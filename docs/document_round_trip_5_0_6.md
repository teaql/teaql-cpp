# Document round-trip runtime checkpoint: Rust 5.0.6

This initial protocol checkpoint retains its original scope and pending list.
The later [model diagnostics and boundary qualification](document_model_diagnostics_5_0_6.md)
closes its Debug/equality/copy and full collection/token-size questions with
actual source comparisons and current native/module/sanitizer evidence.
The later [service checkpoint](document_service_async_5_0_6.md) and
[whole protocol acceptance](document_reference_acceptance_5_0_6.md) complete the
remaining custom-provider/registration/async and source-group obligations.
Historical pending statements below retain their original checkpoint scope.

Status: implemented core protocol checkpoint; complete source and generator
qualification remains open. The active runtime/producer locks are unchanged.
This resolves the 13 previously missing runtime root declarations with actual
models, cryptographic snapshot operations and a separate document service SPI.
Declaration coverage alone is not acceptance of the complete source file.

## Responsibilities

DocumentEntitySnapshot, DocumentRelationSnapshot and DocumentSnapshot retain
separate identity, loaded/writable field sets, mutation-kind sets, ordered relation
rows, completeness and aggregate scope/fingerprint/business identity. Their
validated `create(memory, ...)` factories preserve source validation; allocator
constructors also allow direct model assembly, as source public fields do.
Validation preserves source ordering, uniqueness/subset checks and exact limits.
An explicit empty entity list is permitted unless a relation references a missing
row. Relation names are not silently deduplicated. Lookup returns the first match.

ContextualDocument, SubmittedContextualDocument, AcceptedContextualDocument and
VerifiedDocumentSnapshot remain distinct types. Open and accept requests retain
their independent validation and command identity. Nested strings, sets, vectors
and JSON propagate their declared allocation domain; retention into Global or
another UserContext requires an explicit copy. The nine DocumentRoundTripError
codes remain separate from RoundTripReferenceError while preserving its mapped
code during runtime/authorization failures.

ContextBoundReferenceRuntime now issues and consumes `tqd1` authenticated document
snapshots. It reuses the existing key provider, authorization policy, clock, nonce
source, HMAC/HKDF and AES-GCM implementation. The document encryption label and
four-byte document payload/AAD text lengths remain distinct from entity-reference
tokens. No library dependency or network/cloud component is added. Parsing,
canonical field/mutation order, actor/environment/purpose binding, expiry/future
checks and authorization ordering follow the frozen source. Decode reconstructs
and validates its scope/identity/snapshot instead of treating authenticated bytes
as already valid metadata. Completed plaintext buffers are wiped after cryptographic work.

ContextBoundDocumentService is an independent asynchronous protocol for
`open_document(user_context, principal, request)` and `accept_document(...)`.
The runtime does not invent a default CRUD/graph-save implementation. UserContext
registers a scoped service, resolves the principal and delegates through checked
Context-owned operations. Missing service precedes missing principal. Child wakes
are forwarded, completed values are consumed, foreign operations are cancelled
before rejection, and Context destruction joins pending work before returning its
arena blocks. Services receive the full UserContext for metadata, logs, locking
and existing data-service protocols. Snapshot convenience calls likewise delegate
through the Context's registered reference runtime/principal.

## Evidence

The initial 106 direct original/C++ cases construct actual public frozen snapshot types
and call the unchanged original issue/consume methods. They compare exact
deterministic token bytes, all error codes, authorization call counts, reconstructed
snapshot equality and key/time/lookup observations. Inputs include actor changes,
expiry and future-leeway boundaries, corrupt tags/prefix/segments/keys, purpose/type
mismatch, zero/overflow/subsecond lifetimes, blank/space-bearing metadata, unloaded
writable fields, duplicate fields/rows, complete/partial and readonly relations,
negative versions, field-count boundaries and duplicate/missing entity rows.
There are 20 successful cases and 86 expected failures; errors are not normalized.

The independent C++ service contract verifies resource precedence, no callback
without a principal, deferred child wakes, accepted revision/change identities,
actual UserContext deletion with a pending child, foreign-child cancellation and
explicit Global retention of nested JSON with default PMR disabled. This fixture
exercises the service protocol and ownership adaptation. Another 64 paired cases
now call actual frozen UserContext open/accept methods with an independent
async-trait service fixture. They compare missing-service/principal precedence,
validated versus directly assembled requests, all four application service error
codes, callback counts, nested submitted JSON, command/token/purpose/type fields,
duration components and accepted revisions/changed identities. This qualifies the
dispatch contract, rather than application-specific save behavior.

The corpus now totals 661 cases: 80 successes and 581 expected failures. Thirty-eight
additional cases cover retained decode-only keys after rotation, removed keys,
service/environment separation, 16,383/16,384/16,385-byte text boundaries and
subsecond/normalized durations. A further 453 cases use independently re-authenticated
malformed plaintext. The public original decoder remains the oracle; the fixture
helper only generates its input. Both helpers decrypt a valid issued fixture,
change selected bytes and re-encrypt with the known fixture key/nonce. The corpus
compares the forged token bytes as well as decoder results, preventing different
stimuli from being mistaken for equivalent failures. The helpers never patch or
extract a replacement decoder from the frozen source.

Authenticated mutations include every truncated prefix of the 275-byte base
payload, trailing bytes, unknown versions/enums/boolean values, actor bytes,
invalid UTF-8, text/count bounds, identity/revision mismatches, and expiry/future/
timestamp-range validation order. Decoder errors, authorization counts and any
successfully reconstructed metadata remain exact. The Rust oracle exposes
already locked runtime crypto and async-trait dependencies for this fixture only;
C++ runtime dependencies are unchanged.

The initial native checks pass 3/3 (the new corpus, document Context/service contracts and the
retained reference ownership/crypto contracts). Targeted ASan/UBSan checks pass
2/2 with `detect_leaks=0`; module service/import checks pass 2/2. The expanded
corpus and existing Context contract pass a fresh native 2/2 and targeted sanitizer
2/2; no production interface changed in that expansion. Formatting checks
902 C++ files. Root declarations and the
1,448-entry module export catalog now pass, replacing the preceding missing-name
audit. Remaining source gates include structured Debug/equality/copy breadth,
full collection/token size boundaries, custom provider/configuration and Context
registration/replacement modes, and additional asynchronous source cancellation
observations. Generated graph presentation, model assembly, privacy/governance,
dynamic query translation and the complete P0-P7 objective remain separate and open.

## Reproduction

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin document_round_trip_5_0_6
cmake --build build_native_current --target teaql_document_round_trip_probe teaql_document_round_trip_tests teaql_round_trip_reference_tests
python3 tests/parity/compare_document_round_trip_5_0_6.py --rust /tmp/teaql_cpp_field_layout_rust_506/debug/document_round_trip_5_0_6 --cpp build_native_current/teaql_document_round_trip_probe
ctest --test-dir build_native_current --output-on-failure -R '^(document_round_trip_5_0_6_rust_parity|document_round_trip_context_contracts|round_trip_reference_context_contracts)$'
python3 scripts/verify_public_exports.py
python3 scripts/verify_module_exports.py
```
