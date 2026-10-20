# Document model diagnostics and boundary qualification

Status: qualified diagnostics/copy and collection/token checkpoint. All relevant
registered profile and adapter regression gates are terminal and pass. The
[terminal receipt](parity/document_model_diagnostics_terminal_5_0_6.json.gz)
retains actual commands, outputs, inputs and executable hashes.
This is an SQ08 checkpoint, not complete G13 or P0-P7 acceptance.
Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.

## Runtime change

`document_model_debug.hpp` adds the missing source-shaped `write_debug` overloads
for the two document enums, nine document data/request/snapshot structures and
DocumentRoundTripError. Every source field appears in its original order, with
the original set/list, JSON, identity, duration and UTC timestamp formatting.
The runtime umbrella exposes these through the existing `write_debug` export;
no new generator or application service implementation is introduced.

The error type already provided `source()` returning null. That implementation,
all factories, validation, codecs, token cryptography and service dispatch remain
unchanged. The initial duplicate-method mistake and its actual compiler failure
are retained separately; it is not presented as a missing source responsibility.

These are raw model Debug interfaces, including source-visible document tokens
and JSON bodies. They do not replace safe logging projections or authorize data
publication/persistence. Automatic runtime logging behavior is unchanged.

## Actual model observations

The original observer includes the unchanged retained document observer and calls
actual frozen model constructors and derived Debug/Clone/PartialEq implementations.
It does not implement an alternate formatter or equality function. Its independent
Cargo workspace preserves exact reference registry versions and checksums.

The new matrix contains **192** observations: four text profiles, six normalized
duration/time profiles and eight JSON bodies. It includes escaped/NUL/Unicode and
long text, empty and populated field sets, all mutation/completeness variants,
full integer widths, negative zero, nested objects/lists, pre-epoch/year-10000
timestamps and nanosecond precision. Direct public request assembly retains the
source distinction between an assembled value and a validated request.

Each case compares normal/pretty diagnostics, Clone equality and 47 independent
public-field equality mutations. The nine error factories additionally compare
Display and the existing empty error chain. Five enum variants are observed
independently. The new corpus has **8,832 exact diagnostic strings** and **9,024
field-mutation equality observations** without output normalization.

C++ additionally makes **4,416 explicit Global copies** and invokes their actual
formatters after deleting the source UserContext. Each input has a null default
PMR resource, nested allocator copies, zero retained User bytes and independent
Global output. No shared owner or additional query allocation domain extends the
source lifetime. Targeted ASan/UBSan checks include this complete matrix.

The unchanged **661** historical document inputs also reproduce through the new
exact-dependency workspace and actual C++ runtime. Their corpus and original
observer bodies are unchanged.

## Collection and token limits

A separate **21-case** source/native matrix exercises 9,999/10,000/10,001 entities
and relation rows, 1,023/1,024/1,025 fields and relations, and permitted empty
collections. Maximum field counts and maximum row counts are separate controls.
The initial 9,999-row fixture with additional fields exhausted the default
64 MiB Context budget. Its actual stderr and input-generation history are retained.
Minimal legal projections isolate protocol cardinality from that independent
budget; neither runtime limits nor logical maximum counts are changed.

Token cases are calibrated through actual Rust issuance, not a copied encoder.
Canonical lengths 255,995, 255,998, 255,999 and **256,000** consume successfully;
the first tested over-limit canonical length **256,002** is rejected. A retained
observer modification gives an exact 256,001-byte submitted-token control.
Every issued token byte, error and authorization count matches actual Rust.

The original issuer can return a token larger than the consumer limit for a
valid large snapshot. In particular, legal maximum collection sizes need not
fit a consumable token. C++ preserves this source behavior; model validity and
successful document roundtrip are distinct requirements.

## Canonical module adapter

The new observer includes the retained observer under a renamed entry point.
This exposed an adapter cache that keyed only by source path: when a source
first appeared as a fixture, its standalone executable view was never generated.
The adapter now distinguishes fixture and entry views. A verifier requires an
actual entry record for every canonical target, in addition to comparing bodies.

All **828** current consumer/fixture bodies retain their original statements and
assertions. Five other included-source views are now separate from their entry
prologues. Their nine affected executable targets and eleven registered checks
form a separate regression selection, preserving whole corpora and timeouts.
The complete bootstrap comparator runs alone under its original timeout.

## Reproduction and remaining scope

The final selections pass native **7/7**, imported **6/6** and targeted
ASan/UBSan **4/4**, combining the original model/service selections with the
complete 21-case boundary comparator. The additional adapter regression passes
**11/11**: the unchanged whole bootstrap runs alone in 106.98 seconds under its
original timeout, then the other ten checks pass in 45.40 seconds.

[Pinned format/check](parity/document_model_debug_final_format_5_0_6.json.gz)
passes **1,651** C++ files without changing a byte or timestamp after compilation.
All **828** canonical bodies and **1,503** exports verify. Compiler/linker
processes are terminal before execution; no generated SDK or generator changes
are part of this checkpoint. The scope remains the explicit observations above,
not the full 717-check native registry or all Rust features.

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_document_model_debug_probe teaql_document_round_trip_probe teaql_document_round_trip_tests teaql_round_trip_reference_tests
python3 scripts/verify_document_models_5_0_6.py --record /tmp/document_models.json.gz
python3 scripts/verify_document_boundaries_5_0_6.py --record /tmp/document_boundaries.json.gz
ctest --preset native --output-on-failure -R '^(document_(model_debug|boundaries|round_trip_5_0_6)_rust_parity|document_round_trip_context_contracts)$'
```

Use fresh record paths. Existing corpora must not be overwritten. Imported
observers use the same canonical bodies; sanitizers use the committed preset
with leak scanning disabled. No MIPS or physical-device execution is added.

The later [document service checkpoint](document_service_async_5_0_6.md) qualifies
432 actual source dispatch/replacement/cancellation observations with native2/
module2/SAN2 gates. Custom key/clock/nonce/authorization-provider completeness
is subsequently qualified by the
[whole protocol acceptance](document_reference_acceptance_5_0_6.md). The broader
[G13 requirements](semantic_qualification_requirements_5_0_6.md), all five pending
capabilities and final integration retain their full scope.
