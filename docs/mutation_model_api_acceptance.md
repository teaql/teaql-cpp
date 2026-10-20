# Mutation and trace model API acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
The complete mutation/trace model responsibility group is accepted against the
source review below. Mutation input, provider-generated values and persisted
snapshots remain separate. Execution, governance, audit/log and complete stage
acceptance remain open in their respective groups.

## Complete source responsibility review, 2026-10-08

The review reads the complete pinned `mutation.rs` and `trace.rs` production
bodies, including every derived trait and explicit implementation. This group
contains three distinct map wrappers, six commands, TraceNode, and two enums.
The following table is the acceptance contract; downstream mutation execution,
governance, provider-generated values, audit delivery and log projection retain
their separate module gates.

| Source type | Complete state, defaults, and public operations |
| --- | --- |
| MutationValues | Its single ordered field map defaults empty; `new`, Deref/DerefMut, owned Record conversions, owned iteration and shared borrowed iteration are preserved through explicit scoped construction, `fields`, `into_map` and const ranges. Mutable C++ ranges are an extension. |
| GeneratedValues | Separate provider-output map, empty Default/new, Deref/DerefMut and owned Record conversions. No implicit conversion to mutation input or snapshot. |
| EntitySnapshot | Separate persisted map, empty Default/new, Deref/DerefMut and owned Record conversions. No implicit conversion to mutation input or provider output. |
| MutationKind | Insert, Update, Delete, Recover; Copy/Clone, PartialEq/Eq and derived Debug. |
| InsertCommand | entity, values, trace_chain; constructor retains the supplied entity and starts with empty values/traces. `value` inserts or replaces the named field. |
| UpdateCommand | entity, id, expected_version, values, trace_chain, old_values; absent version/snapshot and empty values/traces at construction. `expected_version` replaces the optional version with Some; `value` inserts/replaces without generating a snapshot or trace. |
| BatchInsertCommand | entity, batch_values, trace_chains; both vectors initially empty. Their lengths are independent. |
| BatchUpdateCommand | entity, batch_ids, batch_expected_versions, batch_values, update_fields, trace_chains, batch_old_values. Constructor retains field order and duplicates; every other vector starts empty. The model does not validate or align independent batch lengths. |
| DeleteCommand | entity, id, expected_version, soft_delete, trace_chain; absent version, soft_delete=true and empty traces initially. Version replacement accepts all i64 values; repeated `hard_delete` keeps soft_delete=false. |
| RecoverCommand | entity, id, required expected_version, trace_chain. The version is mandatory, accepts all i64 values and is not optional; traces start empty. |
| TraceKind | Operation, Request, Relation, Entity, Provider, Sql, Comment, Purpose, AuditReason; Copy/Clone, PartialEq/Eq and derived Debug. |
| TraceNode | kind, entity_type, entity_id, comment. `new` selects Entity; `typed` preserves the selected kind. Both preserve optional u64 ID and complete strings, including empty comments and NUL. |

All three wrappers derive Debug, Clone, Default and PartialEq. Commands derive
Debug, Clone and PartialEq; they have no source Default, Eq, Hash or serde
implementation. TraceNode derives Debug, Clone, PartialEq and Eq. Neither enum
has a source Default, Hash or serde implementation. No source wire protocol is
invented; fixture JSON is only an observation envelope. Raw Debug retains
source type names, field order and formatter options and must not be used as a
privacy-filtered log projection. See the complete mutation and query/trace
Debug acceptance records.

C++ Clone always names a destination allocator; ordinary copy construction and
implicit default-resource construction remain unavailable for all ten structs.
Move-only ownership is the agreed C++ adaptation. In-place builders return the
existing command, replacing Rust's consuming builder receiver while preserving
its resulting state and operation ordering. `expected_version_value` avoids a
C++ field/method name collision; diagnostic output keeps the source label.
The `audit_as` convenience method is a C++ extension and supplies no audit-reason
validation or governance acceptance. A direct public field edit must still
follow the explicit allocator and borrow contracts.

The model API corpus now preserves the original 1,642 observations and adds
46 actual original-method inputs, for 1,688 total. Forty insert/update chains
cover every fixture Value variant, repeated keys, Unicode/NUL fields, version
replacement at i64 minimum/maximum and zero, and every intermediate state.
Three delete chains observe initial soft deletion, repeated hard deletion and
version replacement with null, maximum-u64 and Unicode/NUL IDs. Three wrapper
inputs call actual `new`/Default, delegated map insertion/replacement, mutable
lookup, removal/missing removal and clear. Expectations call the unchanged
original methods; command builders are no longer inferred from field assignment.

Native compile contracts independently enforce explicit scoped Clone for all
ten structs, role separation, enum Copy capability and const/rvalue iteration
boundaries. Default-PMR-disabled builder ownership checks transfer a same-User
8,192-byte payload, replace it with an explicit copy of Global data into User,
retain the 4,096-byte field and entire command in Global, actually delete User,
and read the result through another User's explicit rebinding. The methods add
no implicit snapshots or traces. Existing map-copy fault injection and every
command-kind retention contract remain active.

Fresh integrated verification completed after the expanded corpus and ownership
checks. Native, ASan/UBSan and OpenWrt MIPS/QEMU selections each pass 8/8 in
5.94, 11.92 and 9.72 seconds respectively. Current sanitizer checks use
`ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1` because sandbox
tracing prevents LeakSanitizer; this is not successful leak-scan evidence.
Explicit arena byte-reclamation assertions remain active.

Live original Rust comparisons match all 1,688 model API inputs, 120 mutation
Debug inputs (9,810 exact formatted outputs) and 120 query/Trace Debug inputs
(27,441 exact outputs) on both native and MIPS/QEMU. The complete native build
has no pending work and the final full native suite passes 281/281 in 74.57
seconds. Upstream lock, export-name, source inventory, independent layer,
451-test-disposition, formatting and whitespace checks pass. The complete model
group is accepted; no downstream execution/privacy/stage or physical-router
acceptance follows from this verdict.

## Structural comparison

MutationValues, GeneratedValues and EntitySnapshot now compare their field maps.
Insert, Update, Delete, Recover, BatchInsert and BatchUpdate commands compare
every source field, including independent trace vectors, expected versions and
optional old snapshots. Allocator identity is not part of value equality.
The implementation retains Value's variant-sensitive PartialEq behavior: NaN
is non-reflexive, signed and unsigned integer variants differ, and None differs
from an explicitly present empty snapshot. It does not invent total equality.

The earlier corpus had 1,642 unchanged original public-call observations: 1,323 wrapper,
229 command and 90 trace inputs. It exercises every command field independently,
empty and differently sized batches, field order and duplicates, optional
versions/snapshots, all Value variants, nested NaN, signed boundaries, Unicode
and embedded zero bytes. Original constructors, PartialEq, Clone, Record
conversion and owned/borrowed iteration supply the expectations. The C++ probe
calls the production models; it contains no comparison algorithm substitute.

## Explicit map conversion and iteration

All three wrappers construct from a Record with an explicit Memory or allocator.
A const Record is deep-copied into the destination; an rvalue is consumed only
after successful preparation. `copy_map(destination)` provides explicit retained
copies. `std::move(wrapper).into_map(destination)` provides the consuming form;
lvalue consumption is rejected by the type system.

Same-owner consumption transfers map nodes and preserves their addresses.
Cross-owner consumption first deep-copies all keys and nested values, then
clears the source. Allocation failure leaves the entire source usable. Every
returned map remains owned by its explicitly selected Context. A moved-from
managed wrapper remains a valid empty C++ object until its owner closes; moving
does not refund its object identity or extend a lifetime. Source-container
iterators and native borrows still follow their normal invalidation rules.

MutationValues has sorted const/mutable range iteration, preserving the source's
borrowed iteration and map access responsibilities. Its consuming iteration
form iterates the map returned by `into_map`. GeneratedValues and EntitySnapshot
retain their distinct typed roles and explicitly exposed `fields` map; they do
not gain implicit conversions into mutation input.

## Constructors and trace ownership

BatchUpdateCommand accepts dynamic string ranges, including PMR vectors and
views, and copies names into its selected allocator. It retains order and
duplicates. The existing initializer-list constructor remains available.

TraceNode's default constructor flavor selects Entity kind, matching original
`TraceNode::new`; the explicit-kind constructor retains `typed` responsibilities.
C++ allocator constructors order payload arguments as entity, comment, optional
ID; both flavors use that same ordering. Neither validates an audit reason or
removes an empty comment. All nine TraceKind variants remain distinct.

## Ownership and allocation failures

Native contracts disable the default PMR resource. All three wrappers check
same-domain node transfer, explicit foreign Record construction, retained map
copying, consumption and actual source User deletion. Nested maps, lists,
1,024-byte keys and 65,536-byte strings belong entirely to Global after copying.

Every command kind is explicitly copied through UserContext's Global proxy.
After actual User deletion, complete nested values, old snapshots, dynamic batch
fields, IDs and default-kind trace comments remain accessible through a new
User's explicit rebinding. Cross-domain equality checks every retained field.

Fault injection at successive upstream allocations covers consuming map copies
for all three wrappers. Failed copies preserve the source and previously
published Global content. Retrying consumes only after complete preparation;
all successful and abandoned blocks are returned on owner destruction. No
production dependency, alternate backend or generator change is introduced.

## Reproduction

On 2026-10-07 the complete rebuilt native Debug suite passed 193/193 checks.
The selected model API contracts, original-call comparisons and core-model tests
passed 3/3 under ASan/UBSan with leak detection and 3/3 in the OpenWrt MIPS
cross-build under QEMU. All 1,642 observations also match live original Rust on
native and MIPS. Pinned upstream, export-name, inventory and clang-format 18.1.8
checks passed. These selections do not replace complete regression or wider
model/API/privacy acceptance.

```sh
cmake --build build --target teaql_mutation_model_api_tests \
  teaql_mutation_model_api_probe
ctest --test-dir build --output-on-failure \
  -R '^(mutation_model_api_contracts|mutation_model_api_rust_parity|core_model_contracts)$'
python3 tests/parity/compare_mutation_model_api.py \
  --cpp build/teaql_mutation_model_api_probe \
  --rust build/rust-original-roundtrip/debug/mutation_model_api_oracle
```

The corpus generator records original public calls only. The complete model
review above closes this source group; downstream execution and privacy/log
audits and six-stage acceptance remain open. Physical-router execution is
unverified.

## Current 5.0.6 whole-source continuation

The preceding checkpoints remain historical 5.0.5 evidence. The complete current
[MU01-MU14 review](core_mutation_source_review_5_0_6.md) now accepts the
291-line mutation source. All 2,048 original/native/imported outcomes per
frontend, one positive/fourteen precise negative source compiler controls and
native 5/imported 5 pass. Four complete canonical consumers are permanently
imported; native model assertions cover all six role conversion/assignment
directions. Existing function bodies and all production implementations remain
unchanged. Whole execution, governance, privacy and G13/G14/G17/P0-P7 retain
their separate obligations. No historical full or sanitizer/MIPS result is
relabeled as a new run.
