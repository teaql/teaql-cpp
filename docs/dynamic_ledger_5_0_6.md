# Runtime durable dynamic values and mutation provenance

Status: runtime state/ledger mechanics implemented; generated typed carrier,
storage service and protected SQLite persistence qualification remain open.
Reference: Rust v5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, primarily
`teaql-runtime/src/entity_runtime.rs` and `data_service/types.rs`.

## Separate loaded view and pending write intent

`EntityRuntimeState::install_loaded_dynamic_fields` installs an independent
Context-owned value carrier without creating a mutation ledger. Ordinary state
has no dynamic payload. Same-Context Clone shares its lazy mutation ledger and
immutable graph, while copying dynamic row values. Cross-domain retention copies
values, definitions, geometry and mutation provenance into the destination.
State equality additionally compares dynamic values before ledger identity.

`update_dynamic_field` and `delete_dynamic_field` require installed definitions
and a matching entity owner, then apply the core type/name validation. A missing
carrier reports DYNAMIC_FIELD_DEFINITIONS_MISSING; an owner mismatch reports
DYNAMIC_FIELD_OWNER_MISMATCH. Update returns whether selection grew; delete
returns whether the field was selected. Existing-name operations use the immutable
definition's name bytes so a C++ caller's alias of a replaced payload cannot
invalidate the mutation-record name.

`EntityChangeSet` keeps native `MutationValues` separate from optional
`DynamicFieldChanges`. Every dynamic entity key retains immutable definitions and
a field map of first originals, distinguishing no stored value from stored NULL.
Repeated writes keep the first original and replace only pending Set/Delete
intent. `field_names` includes `#code`; native `get` never exposes dynamic intent.
Dynamic-only changes make a change set nonempty. Ordinary sets allocate no
extension metadata; rare changes/provenance share one C++ owned metadata block
with distinct maps, rather than removing their responsibilities.

Pushing a frame captures that view's current dynamic values on its first change.
Popping or clearing frames changes intent only, and does not roll back row values.
Entity clearing removes native/dynamic changes, definitions and originals together.
Commit clearing removes pending intent but preserves view-loaded values.

## Composition and original snapshots

Composition first checks original-version conflicts, then compares incoming
dynamic definitions against every receiving frame before importing any native or
dynamic intent. A mismatch records ConflictingDynamicDefinitions with the source
display contract. The existing checked-composition/error mechanism rejects later
protected work; no partial native import occurs on this validation failure.

Incoming pending mutations win while receiving first originals and root reason
remain authoritative. Composition changes ledger ownership, never the receiving
view's dynamic payload. `fresh_with_shared_graph` has no loaded extensions;
`with_loaded_view_from` copies the source's loaded view while preserving the
receiver's ledger binding. `set_original_compact_row` strips read sidecars before
retaining persistence snapshots, preserving projection/relation geometry and
readonly result schema as the source does.

Private definition/original access lives behind EntityRuntimeAccess for storage
and privacy adapters. C++ dynamic payloads remain registered Context objects and
are borrowed by state, following the same arena adaptation as read-row sidecars.
Replacing a carrier detaches the old payload; its owning Context reclaims it.
No shared owner extends a User Context's lifetime.

## Governed privacy

The ledger privacy collector includes dynamic-only entity keys when requiring
descriptors. It gathers pending Set operands first, then actual original values;
Delete has no proposed operand. All durable operands use Unknown parameter
policy, matching the source's conservative treatment regardless of a similarly
named native field. They remain redacted for both safe and debug logs. Native
mapped-field policy and ID/version exclusions retain their existing contract.
Collected provenance flows through the existing private bound mutation/readback
service copies, without a new ambient registry or wire override.

This collector does not execute or grant trusted storage authority. Transaction
binding and dynamic graph/ledger write preparation remain separate service/provider
work. No ordinary native CRUD or mutation marker is claimed as durable graph save.

The ledger executor currently rejects pending dynamic intent before native
classification, reads or DML because no transaction-bound dynamic storage SPI
is installed. It returns the distinct `RuntimeError::DynamicField` carrier with
`DYNAMIC_FIELD_TRANSACTION_BINDING_REQUIRED: save` and keeps all pending changes.
This prevents native writes from committing while silently discarding extensions.
It is an interim dependency rejection, not an implementation of persistence.
Replace it with qualified dynamic write planning/execution when the actual
store/provider protocol is connected. The typed error preserves its source
display/debug payload and copies both strings into the destination Context.

## Evidence and reproduction

The dedicated locked Rust observer calls original public EntityRuntimeState and
EntityChangeSet APIs. The corpus records 147 original/C++ observations with no
normalization: missing/type/owner failures, selected-state changes, NULL/delete,
loaded-value isolation, shared ledgers, frames, fresh/rebound views, composition,
root reason and state equality. Public comparisons do not inspect private original
maps or first-composition-error storage and do not qualify full Debug layouts.

Native contracts separately verify first originals, absent versus NULL, definition
provenance conflicts before native import, explicit Global retention after actual
User deletion, and conservative dynamic operand/original privacy. Those private
map checks are supported by source review and C++ execution, not relabeled as
externally observed Rust private calls. Future qualification must cover complete
typed model, storage, audit and generated configurations.
The native missing-storage test also checks zero query/mutation executor calls,
retained native/dynamic intent and allocator-aware copying of the typed error.

```sh
source scripts/toolchains/activate.sh
CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build \
  --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --bin dynamic_ledger_5_0_6 --locked --offline
cmake --build build_native_current \
  --target teaql_dynamic_ledger_tests teaql_dynamic_ledger_probe
python3 tests/parity/generate_dynamic_ledger.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/dynamic_ledger_5_0_6 \
  --cpp build_native_current/teaql_dynamic_ledger_probe
ctest --test-dir build_native_current -R '^dynamic_ledger_' --output-on-failure
```

The capability ledger remains partial. Remaining work includes typed generated
carrier hooks and indexed availability updates, Context storage/provider SPI,
dynamic audit values, bound SQLite transactions/readbacks, JSON/stream hydration,
and complete dynamic diagnostics and source configuration qualification.


Final gates: native 6/6 (runtime/trace/generated API/loaded-state regressions plus
ledger contracts/comparison), targeted ASan/UBSan 4/4 with leak scanning disabled,
module import 1/1, 1,370 catalog exports and seven facade archives. The live
original/C++ comparator matches all 147 observations. Formatting checks 770
handwritten files. No generator output, MIPS or physical-device result is newly
claimed by this checkpoint; full G16/G17/P0-P7 remains unqualified.
