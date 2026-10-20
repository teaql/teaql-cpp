# Deferred continuous cursor pages and interleavings

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This qualifies the declared cursor-store wait/interleaving matrix through actual
original/generated typed list execution and SQLite.

## Actual source matrix

The complete unchanged dynamic_json domain executes ordered Order queries with
continuous-page optimization. Both observers assemble the existing SQLite schema,
dataset, generated runtime, active root and explicit diagnostic identity. Custom
cursor-store implementations provide valid, missing, invalid, expired, read-error
or write-error responses and controlled get/put waits. They do not implement
querying, seek construction, pagination or typed decoding.

The matrix has 480 sequential cases plus 160 interleavings, for **640** cases.
Sequential inputs cross ten store modes, none/get/put waits, resume/cancel,
ascending/descending order and sizes 0/1/2/4. Each runs three consecutive offsets.
The store records exact callback keys, entity, direction, boundary, page size and
next offset. Results retain typed IDs/versions, errors, Pending/wake/cancel,
Context plan and cursor-presence observations.

Interleaved inputs hold a first get or put, execute another query in the same
Context, then resume or drop/cancel the first and perform a follow-up request.
In the recorded-store case, a deferred get can read the cursor that its peer
published while it waited. Cancellation before put completion must not publish
that unfinished cursor. Callback order and later page plans remain actual
observations; they are not sorted or reconstructed by the comparator.

Generated cursor IDs and absolute expiry depend on wall time. Only those two
fields are symmetrically projected: generated IDs must have the original `cpg_`
hex syntax, and expiry must be in the future. Boundary/key/entity/direction/
size/offset and logical results are retained exactly. Returned synthetic valid
cursors use a fixed ID; Context cursor presence is observed rather than a random
wall-clock ID. No error, key or business-value normalization is applied.

## Explicit C++ store ownership

Eight native cases cross get/put waits with registration replacement, explicit
old-store destruction, cancel-before-destroy and actual UserContext deletion.
A pending operation keeps its selected checked store borrow through read/write;
a later request uses the newly registered store. Destroying the old resource
must reject access, cancel the pending callback once and leave the replacement
usable. Closing UserContext must quiesce either waiting phase before reclamation.

These are C++ ownership adaptations. Rust safely permits simultaneous immutable
Context borrows, but its mutable registration setter cannot replace a store on
that borrowed Context during the original future. The native cases do not claim
an invented original-Rust mutation/destruction action.

Controls and retained cursor observations allocate explicitly in GlobalContext;
store, request and operation state allocate in UserContext. No shared owner
extends a destroyed C++ store lifetime. The native observer disables default PMR
allocation and actually destroys input Contexts. Existing runtime cursor/query/
reference/cancellation algorithms are unchanged.

## Reproduction

```sh
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_deferred_continuous_page_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --bin deferred_continuous_page_5_0_6
TEAQL_LOG_ENDPOINT=off python3 tests/parity/generate_deferred_continuous_page_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/deferred_continuous_page_5_0_6 \
  --cpp build_native_current/teaql_deferred_continuous_page_probe --check
ctest --preset native --output-on-failure \
  -R '^deferred_continuous_page_(rust_parity|context_contracts)$'
```

Broader grouped/aggregate/facet/inherited/constant/dynamic model configurations,
protected diagnostics, cache coordination and full integration remain separate
requirements. This does not complete all G09/G13/P0-P7 scope.

## Terminal verification

All **640/640** original/native scenarios agree and reproduce, including 160
interleavings. Eight native ownership cases pass. Native, targeted ASan/UBSan and
imported module selections each pass **2/2**, after terminal successful builds;
sanitizer leak scanning is disabled. Canonical consumer verification covers 161
bodies, with 1,479 exports and seven acyclic module dependencies. Pinned format/
check covers 1,022 handwritten C++ inputs. See the
[terminal record](parity/deferred_continuous_page_5_0_6.json). No production or
generator body changed; Java/generation reproduction and MIPS/physical-device
gates were not repeated. Full capabilities and phases remain incomplete.
