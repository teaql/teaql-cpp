# CompactRow public API and layout acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`.
This checkpoint restores missing CompactRow responsibilities. It does not
complete the wider query/runtime/API migration.

## Complete query/row review checkpoint, 2026-10-08

The complete query.rs production body includes all fifteen CompactRow methods,
the owned Record conversion trait, both JSON projection functions and the exact
partition-rank property constant. Forty-five actual JSON/key/value iterator inputs
extend the corpus from 356 to 401 without discarding the earlier operation
sequences. Original calls retain duplicate column order while JSON/map conversion
keeps the last value; nonfinite, nullable, Boolean/date/timestamp and recursive
values use actual Value projection rather than a probe implementation.

All 401 inputs match live original Rust on native and MIPS. Current native,
ASan/UBSan and MIPS ten-check selections pass 10/10, including this final fixture,
query builders/helpers and source-derived diagnostics. Native/MIPS also match
all 169 row/list and 120 query/Trace Debug inputs. Current leak scanning remains
disabled under tracing, with explicit reclamation assertions intact. The
[complete query source review](query_source_acceptance.md) is accepted after
the final full native regression passes 281/281 in 72.92 seconds; the build is
clean. Downstream execution/runtime/stage acceptance remains open.

## Immutable layouts and copies

Every published row now has an immutable, Context-owned ColumnLayout. This
includes empty rows, rows constructed from a Record and rows whose shape was
changed. `shared_columns()` supplies that layout, rather than reporting a
physical shared-versus-owned implementation flag. Same-owner copies share the
layout but copy the values. Cross-owner copies deep-copy both layout and nested
values into the explicit destination; no shared owner prolongs Context lifetime.

Replacing an existing value preserves layout identity. Adding or removing a
column prepares a new layout before publishing it; previous layout borrows
retain their original contents until their owner closes. Original duplicate
column ordering and first-occurrence lookup/removal remain intact. Layout
payloads and cleanup belong to the row's Context, consume bytes and do not
consume another independently referenceable object identity.

The row stores one layout pointer and its value vector. The previous extra
mutable column vector is removed. Supplied same-owner ColumnLayout references
retain the existing native-borrow contract: the layout must outlive its rows.
Internally created layouts are registered in the owner arena. Layout borrowing
requires that explicit Context arena; arbitrary external PMR resources cannot
own the registered layout lifecycle.

Moved-from C++ rows remain valid and empty. Their accessor materializes an empty
owner-local layout when first needed. This is a C++ moved-from-state adaptation;
Rust code cannot reuse the consumed source object. It creates no third ownership
domain, hidden cache or implicit lifetime extension.

## Iteration, extension and consumption

`iter()` exposes paired const column/value references in original projection
order, including duplicate names. The returned range is a native borrow of the
row; value/shape mutation follows container invalidation rules. Existing keys
and values spans remain available.

The source's consuming Record boundary is supported by an rvalue constructor
and `std::move(row).into_map(destination)`. Same-owner conversion allocates all
map nodes and copied keys before moving any value, then transfers nested value
storage. Repeated names retain their last value, matching BTreeMap collection.
Foreign conversion deep-copies all content before clearing the source. Failed
preparation preserves the complete row and its original layout.

`copy_map(destination)` makes retention intent explicit. The existing const
lvalue `into_map(destination)` forwards to that copy for compatibility. A
consuming Record constructor prepares the complete column layout and value
capacity before consuming source values; foreign input is copied before clearing.
Same-owner moves of rows retain layout identity and independent Context lifetime.

Self-extension snapshots the input, matching `row.extend(row.clone())` in Rust.
It is not a no-op for duplicate column names: the later duplicate updates the
first matching target position. This original behavior is tested directly.

## Original-call observations

The corpus records 356 original public-call sequences: fixed empty/unique/
duplicate/Unicode layouts and 256 seed-505 sequences with sixteen operations
plus an initial saved layout. Calls cover construction from columns or maps,
Clone, shared_columns, get/get_mut, contains_key, insert/remove, paired iteration,
keys/values, self/foreign extension, consuming map conversion and reconstruction.

Every step compares typed value Debug pairs, last-duplicate maps, row length,
empty state, clone contents/equality, layout identity, layout changes and every
retained historical layout snapshot. Original Rust Arc pointer identity is
compared with C++ owner-local layout pointer identity. Value variants, nested
content and NaN comparison behavior are preserved. Expectations come from the
unchanged original public methods, not copied query/row algorithms.

The complete JSON corpus is stored losslessly as `compact_row_api_cases.json.gz`:
38,094,518 bytes compress to 337,507 bytes. Compression removes no sequence,
operation, field or result. Python's standard gzip module is a test-tool
dependency only; the C++ runtime adds no dependency. The generator fixes gzip
mtime for reproducibility, and the comparator validates the pinned baseline.

## Ownership and allocation failures

Native tests disable default PMR allocation. They verify stable old layouts,
same-owner cloning, independent value edits, duplicate paired iteration,
self-extension and same-owner nested-text transfer. Explicit Global row/layout
copies remain complete after actual source User deletion and rebinding through
a new User. Existing SQLite decoding and in-memory query tests now assert
destination-owned layouts instead of nullable physical-storage flags.

Successive upstream allocation failures during shape growth and foreign
consumption preserve values and original layout identity. All successful and
abandoned layout/value blocks return on owner destruction. Constructors retain
the existing explicit column/value count precondition. Native borrowing is not
a universal check on later raw C++ dereferences.

## Reproduction and remaining gates

On 2026-10-07 the complete rebuilt native Debug suite passed 195/195 checks.
The five selected row/collection/SQLite/in-memory checks passed on native,
ASan/UBSan with leak detection and the OpenWrt MIPS cross-build under QEMU.
All 356 observations also match live original Rust on native and MIPS. Pinned
upstream, inventory, root export-name and clang-format 18.1.8 checks passed.
These checks do not establish complete query/API or migration-stage acceptance.

```sh
cmake --build build --target teaql_compact_row_api_tests teaql_compact_row_api_probe
ctest --test-dir build --output-on-failure \
  -R '^(compact_row_api_contracts|compact_row_api_rust_parity|core_collection_contracts|provider_sqlite_decoding_contracts|inmemory_context_contracts)$'
python3 tests/parity/compare_compact_row_api.py --cpp build/teaql_compact_row_api_probe \
  --rust build/rust-original-roundtrip/debug/compact_row_api_oracle
```

The source reference and model generator remain unchanged. Full core/query API,
defaults, serialization, provenance/privacy and migration-stage audits remain
required. Physical-router execution is unverified.
