# Private memory Debug responsibility: Rust 5.0.6

Status: focused native/module/sanitizer qualification passes.
Complete private-source acceptance remains pending.

## Restored diagnostic responsibility

`MemoryDataServiceError` now has a DebugWriter overload separate from Display.
Poisoned is a unit variant; UnsupportedExpression and UnsupportedAggregate are
named tuple variants with escaped text. Existing error messages remain unchanged.

Rust derives service Debug only when its metadata type also implements Debug.
The C++ service intentionally erases the metadata implementation behind the
MetadataStore interface. Its explicit `write_debug_to(writer, metadata)` entry
therefore takes a typed metadata reference and requires a matching DebugWriter
overload at compile time. Before output, it verifies identity against the stored
checked borrow. A caller cannot substitute another metadata store to produce a
misleading service dump. MetadataStore itself does not acquire a new virtual
formatting requirement, and non-Debug implementations remain valid for execution.

The service writes metadata and the ordered data map through the existing core
formatters, retaining CompactRow shape and nested Value types. Its mutex wrapper
includes data, poisoned state and the non-exhaustive marker. A nonblocking lock
attempt renders `<locked>` if data cannot be acquired. Poison state is atomic so
that this diagnostic branch does not read an unprotected ordinary Boolean. If
formatting unwinds while holding the lock, the existing lock guard records poison;
failed lock acquisition itself does not. No diagnostic allocates through the
default resource or extends a Context lifetime.

This is raw test-oracle Debug support. It is not a replacement for production
execution logging, masking or audit delivery.

## Evidence

The [original observation](parity/memory_debug_original_5_0_6.json.gz) formats
unchanged Rust errors and services. Three error variants in compact/pretty form
are compared with their separate Display messages. Empty, seeded and poisoned
services are each compared in compact/pretty form, including metadata, full row
structure, escaped strings, maximum signed versions and mutex state. The
[native comparison](parity/memory_debug_native_5_0_6.json.gz) matches all twelve
outputs byte for byte after JSON transport; no whitespace or field normalization
is applied. The poisoned examples trigger actual original arithmetic panics.

C++ compile-time assertions distinguish the formattable concrete metadata type
from the erased base interface. A separate binding test verifies that another
live metadata object is rejected before any output is written. The default PMR
resource is disabled. These twelve cases do not claim a live concurrent-contention
or injected formatter-failure test; the try-lock/unwind paths are source-reviewed
implementation behavior with that explicit evidence limit.

## Follow-up lock-boundary qualification

The [contention/failure checkpoint](memory_debug_failure_5_0_6.md) resolves the
initial concurrency/fault-injection evidence limit above. It fixes quoted locked
output and narrows the guard to the data field, retaining all twelve earlier
outputs. Five additional source observations qualify the corrected lock boundary.

## Executed gates

The [module comparison](parity/memory_debug_modules_5_0_6.json.gz) and
[sanitizer comparison](parity/memory_debug_sanitize_5_0_6.json.gz) also match 12/12.
The [checkpoint](parity/memory_debug_checkpoint_5_0_6.json.gz) records native 2/2,
sanitizer 3/3 and the module metadata-binding control. Sanitizer regression
includes all 1,144 mutation/poison observations after the lock-guard/atomic change.
Leak scanning is disabled; undefined behavior stops execution. Canonical
verification checks 1,121 bodies and locked source/path checks pass. Pinned
format/check covers 1,789 handwritten files; affected builds report no work remaining.

## Reproduction

```bash
source scripts/toolchains/activate.sh
cmake --build build_native_current --target teaql_memory_debug_probe
ctest --test-dir build_native_current -R '^private_memory_debug_' --output-on-failure
```

The original runner removed 160,907,901 bytes of owned view/Cargo target after
completion and verified cleanup.
Stable preset build directories are reused. Generated artifacts, production
execution bodies and the baseline are unchanged. Final whole-source replay and
consolidation remain required before promoting the source disposition.
