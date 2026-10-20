# Readonly query schema builder: Rust 5.0.6

References are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. This continues the
[query source review](core_query_source_review_5_0_6.md). The earlier
[compiler witness](parity/core_query_schema_builder_gap_5_0_6.json.gz) records
the original public builder succeeding while C++ rejected the missing member.
That historical failure remains unchanged.

## Runtime responsibilities

`SelectQuery::with_dynamic_property_definitions(const DynamicPropertyDefinitions&)`
replaces only the optional readonly result schema and returns the receiving
query. Its existing allocator identifies the destination; this pure kernel
operation does not require another UserContext parameter. It does not select
fields, manufacture loaded values, append trace, execute SQL or authorize Save.
An absent schema and a present empty schema remain distinct.

The public schema field keeps its existing ScopedBox type. No query-private
allocation region, reference count, singleton or hidden owner is introduced.

| Input or operation | C++ ownership behavior |
| --- | --- |
| Registered metadata in the query's Context | Reuse the exact typed managed object. Its Context owns cleanup; the query contains an ordinary borrow. |
| Same-Context query Clone | Reuse registered immutable metadata. Replacing either query's schema does not change the other. Equal independently supplied schemas retain their distinct identities. |
| Allocator-aware stack value | Copy into a registered destination object before the input value is destroyed. |
| Metadata embedded in another managed type | Verify the actual registered type; copy the subobject into independent managed storage. Destroying the container cannot invalidate this copy. |
| Explicit destination query copy | Copy schema and nested map keys into the destination Context. Neither a source UserContext nor its allocator is retained. |
| Existing owning ScopedBox | Copy before replacing or outliving its wrapper. An owning box is not mistaken for a registered loan. |
| Explicit non-Arena kernel resource | Preserve the existing allocator-aware owning-box fallback. This is kernel resource behavior, not a third application Context lifetime. |

Registered metadata must remain live while its borrows are used. Dropping a
local reference does not destroy the Context-owned object. Explicitly destroying
that managed object invalidates its borrows, as does Context close. ScopedBox
`get()` and `release()` expose borrows to registered payloads; they do not
transfer the arena's destructor registration or extend its lifetime.

Binding prepares its replacement before publishing it. Object-budget or
upstream-allocation failure leaves the previous schema intact. Managed rebinding
and same-Context cloning need no new object identity. A destination copy does
consume a destination object identity; backing strings and map nodes do not each
consume one.

Private shutdown diagnostics must continue to use precharged storage without
publishing new object identities. Trusted kernel copies during closing must not
attempt ordinary live-reference resolution. Application execution still rejects
closed Contexts; diagnostic copying grants no execution authority.

## Source observations and native controls

The independent `rust_query_schema_builder_5_0_6` observer uses exact reference
registry versions and checksums. It calls the unchanged original builder,
Clone, equality, list preparation and derived Debug. It implements no schema,
query or hashing algorithm.

The 480-input matrix crosses ten DataTypes, six schema pairs and eight existing
query-state combinations. It retains empty/present schemas, replacement with
another type, separately allocated equal schemas, self-binding, input-handle
release and prepared-query identity. Unicode, embedded NUL and long names remain
complete. Each comparison includes all nine Debug modes for complete query
states and explicit schema-identity observations.

The native contract disables default PMR allocation. It separately exercises
stack and nested-container destruction, legacy owning boxes, actual source User
deletion after copies into another User and Global, nested allocators, object
quota exhaustion, upstream fault/retry and full arena reclamation. These are
C++ ownership controls, not normalized Rust outputs. The closing-reserve control
exhausts both public quotas, disables further upstream allocation and clones
during quiescence. The initial live-resolution path failed; its
[regression record](parity/query_schema_builder_shutdown_gap_5_0_6.json.gz)
is preserved. The repaired path creates private payloads in the precharged
reserve, publishes no new identities and returns every block.

## Acceptance boundary

The [qualification record](parity/query_schema_builder_qualification_5_0_6.json.gz)
joins the [fresh actual-source replay](parity/query_schema_builder_final_5_0_6.json.gz)
and terminal installed profiles:

| Gate | Completed evidence |
| --- | --- |
| Original Rust | 69 unchanged core tests and 480 exact schema-builder observations |
| Native | 15/15 selected contracts/corpora, including the 480 complete outputs |
| Named modules | 15/15 selected contracts/corpora, including the same 480 complete outputs |
| ASan/UBSan | 3/3 selected source/lifetime checks; leak scanning disabled |
| Canonical imported bodies | 951 complete consumer/fixture bodies verified |
| C++ formatting | Pinned format/check passes for 1,687 files |
| Preserved implementation | Apart from the schema branch in SelectQuery's copy constructor, all existing function bodies remain byte-identical; two functions and one declaration are added |
| Generated artifacts | 78 SDKs/3,457 files and 2,201 protected artifacts retain their bytes |

All compiler/linker/test processes finished successfully. Each installed profile
has a final no-work build; the source corpus reproduces byte-for-byte. Original
source, prior failure records and earlier observer projects/locks remain
unchanged. The artifact check does not rerun generation or Java.

The named schema-builder responsibility is qualified. Whole query.rs and full
G13/G17/P0-P7 remain unaccepted. Source status stays 50 accepted / 72 pending /
2 excluded_external; all seventeen capability statuses remain unchanged. Native
discovery is 755, without a current whole-suite claim. Historical failed full
epochs retain their original outcomes. No new MIPS, external-provider, GPU or
physical-device evidence is claimed.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target \
    teaql_query_schema_builder_tests teaql_query_schema_builder_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_query_schema_builder_5_0_6.py \
    --record "$replay_directory/schema.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^query_schema_builder_(contracts|rust_parity)$'
```
