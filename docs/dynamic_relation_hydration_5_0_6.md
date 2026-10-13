# Dynamic relation and child-query hydration in Rust 5.0.6

Status: scoped implementation and original-source comparisons; G16 remains
partial. Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, principally
`teaql-runtime/src/data_service/relation.rs` and `resolved.rs`.

## Fetching and checked child overlays

Relation hydration loads selected dynamic values after the complete native fetch
(including any parent probes), using the Context provider and the existing batch
validator. It derives the trusted intent from the child service and base relation
query. The loader completes before descendant hydration and graph installation.
Flat and compact-flat decoders preserve the resulting read sidecars and indexed
availability through the shared typed decoder. Empty batches still skip provider
I/O; invalid owner IDs and provider errors preserve their existing contracts.

Child-query enhancement snapshots the selected query and validated child intent
only on the rare dynamic path. After the native child query completes, it loads
extensions through the same Context provider, then installs the overlay. Multiple
child selections can combine values and immutable selected-code geometry. The
shared `attach_child_query_rows()` boundary returns the checked `try_extend()`
result; incompatible definitions or readonly metadata fail before that row's
native fields change, rather than escaping as a generic C++ exception. As in the
source, an error on a later parent can follow successful earlier private-row
merges; the failed operation must not publish that partially assembled result.

No SQL, save implementation or alternate provider is introduced into examples or
emitted models. Query/service/executor/provider responsibilities remain distinct.
Nested operations forward wakes and are cancelled before Context reclamation;
rare child payloads propagate the explicit allocator and own no shared lifetime.

## Batched embedded traversal

Rust 5.0.6 changed embedded traversal from the historical per-record `mem::take`
procedure. C++ now loads each descendant plan once over the complete child batch,
then attaches that batch to the parents. It clears nested query relations at the
single-layer fetch boundary for both ordinary and compact planning so the recursive
planner owns the subtree. This avoids fetching the same nested request twice.

Attachment keys are captured before descendant hydration. A nested overlay can
change a child's visible fields without changing the original parent membership.
Inverse convenience wiring is suppressed when an explicit child request already
owns that inverse relation. A filtered to-one target retains the real foreign
identity as an identity-only row. Parents with a missing local key receive an empty
or NULL relation result under the source-defined attachment contract.

A parent does not receive a partially materialized embedded relation while its
child batch is pending or fails. The historical native fixture now checks two
batch fetches for its child/leaf tree and unchanged parent state on cancellation,
while retaining relation lineage, cardinality and native-worker cleanup checks.
Historical 5.0.5 corpus records and acceptance labels are not relabeled.

## Read sidecars and the original boundary

Embedded attachment retains a `SmartList<CompactRow>` sidecar when the original
non-facet condition admits it: a related row already has loaded relation-list
metadata. The graph flattening bridge consumes that sidecar before falling back
to ordinary nested maps. This preserves nested rows and their dynamic payloads
when the original path retains them. Ordinary map conversion intentionally
excludes persistent extensions and storage authority.

The original `CompactRow::has_loaded_relations()` tests relation lists, not a
standalone dynamic payload. A direct embedded relation with only dynamic fields
therefore does not necessarily retain its extension sidecar in the ordinary map
representation. The live corpus records that original behavior unchanged; it is
not described as complete typed embedded dynamic hydration. Facets are outside
this checkpoint. Typed flat/compact-flat graph loading has separate native
positive evidence and does preserve dynamic values and availability.

## Evidence and reproduction

The separate `dynamic_relation_5_0_6_cases.json` corpus records 138 unchanged-source
observations through public raw-row services and actual SQLite. It covers child
selection/overlay, successive child selections, NULL/zero/absence, empty batches,
provider/type failures, many/to-one relations, target filters, depth-one/two nested
requests and the source's sidecar retention boundary. The retained School descriptor
is configured with a self-relation for the isolated raw-row fixture; this is not
full generated School or polymorphic model qualification. Errors and metadata
retention are compared directly without normalization.

The existing dynamic-query oracle/probe gains additional modes; its original
250-case root/decode/stream corpus remains separate and unchanged. Native checks
add typed compact-flat graph hydration, incompatible-definition rejection before
native overwrite, and captured-key attachment after a child field changes.
Existing threaded relation checks cover cancellation/close joining native work.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin dynamic_query_5_0_6 --locked --offline
cmake --build --preset native \
  --target teaql_dynamic_relation_tests teaql_dynamic_query_probe
python3 tests/parity/generate_dynamic_relation.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_query_5_0_6 \
  --cpp build_native_current/teaql_dynamic_query_probe
ctest --test-dir build_native_current -R '^dynamic_relation_' --output-on-failure
```

Verified checkpoint: selected native regression 9/9; targeted ASan/UBSan 6/6
with `detect_leaks=0`; named-module import 1/1. Both the new 138-case relation
corpus and the retained 250-case root/decode/stream corpus pass against live frozen
Rust after the final oracle rebuild. Formatting checks cover 789 handwritten C++
files. The active path verifier retains 114 production units and 124 source links,
with pending semantic dispositions. No MIPS or physical-device run was needed.

## Remaining qualification

Graph-save transaction preflight, writes, persisted-value readback and audit
emission still need integration; the ledger's temporary pre-I/O save guard remains.
Facet sidecars, complete typed embedded configurations, generated indexed carrier
adapters, dynamic JSON decoding, full hook/trace/diagnostic/configuration coverage
and shared SQLite dynamic statement caching remain open. This checkpoint does
not qualify full G16/G15/G17, full 5.0.6 parity, or phases P0-P7.
