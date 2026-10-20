# Invocation-local diagnostic sources for derived queries

The [exact-reference replay](generated_support_reference_5_0_6.md) subsequently
rechecks all 576 observations and the 360 composition observations with reference
registry versions/checksums. Earlier observer locks and the temporary facet
harness described below remain historical: their dependency identities differ
from the reference, including futures-util 0.3.34 versus reference 0.3.32.
Preserve their original outputs; the independent replay supplies its own lock
and terminal evidence without relabeling those earlier runs.

This checkpoint implements the locked runtime's diagnostic-source responsibility
for PurposedSelectQuery. The source query is classified for logging and is never
executed as additional SQL. Complete G09/G10/G17 and P0-P7 remain partial.

## Source contract

Rust 5.0.6 PurposedSelectQuery stores its executable query separately from an
optional diagnostic source. `for_exact_count` saves the original source once,
clears scalar/expression projections, orders, slice and relations, and appends
the requested COUNT. Repeated transformations retain the first source.

`with_facet_diagnostics` expands future facet selections into a separate source
tree, including selected relation aggregates and nested facets. It composes with
an existing COUNT source and leaves the executable query unchanged. Debug prints
the executable query and a presence flag, excluding the diagnostic tree itself.
`into_query` deliberately returns only the executable query.

Original entity-service public entries select a private service from the source,
validate its intent and collect each node through the executor's classification
SPI before execution. This performs no database access. The service remains
local to one request; unrelated later queries retain their independent intent.

The complete unchanged original SQLite request/trace suite compiles through a
test-only wrapper. Its three COUNT tests pass: successful removed-relation
privacy, failed-query privacy and required intent with logging disabled. The
original suite also includes a positive control demonstrating that rebuilding
a wrapper from `into_query` loses this source and exposes its intent canary.
No frozen runtime, original test or producer file is edited.

## Native implementation

The C++ wrapper keeps both query values in its explicit allocation domain.
Allocator-aware copies and moves retain the diagnostic source; Global retention
requires an explicit destination copy. No shared owner extends User lifetime.

One shared diagnostic-query operation owns the execution snapshot, source snapshot
and aggregate arguments, creates the private service when polled, then delegates
to the existing public entry's executor/decoder. It forwards wakeups, results and
cancellation. The source is never inserted into the executed query's relations
or enhancements. Ordinary wrappers without a source use their existing path.

The initial comparison found 24 stream privacy differences. Streaming used the
Context observer directly, bypassing private service redactions. It now uses the
service observer, including the dynamic-field stream path. The expanded typed
matrix then found six entity-read differences: its Context-layer metadata view
also bypassed private redactions. A checked borrow in the metadata view now
preserves that classification through existing observation and record paths.
Context, entity service, SQL executor, transport and provider remain separate.

Both initial failures are retained:

- [Stream differences](parity/derived_query_diagnostics_5_0_6_initial_gaps.json).
- [Entity-read differences](parity/derived_query_diagnostics_5_0_6_typed_initial_gaps.json).

## Actual SQLite qualification

The **576/576** original/native matrix uses fresh in-memory SQLite databases and
the existing handwritten Order model on both sides. It covers removed relation,
expression-projection and order bindings, plus a future facet source. Five raw
entries exercise borrowed/owned rows, SmartList, relation-aggregate SmartList and
stream. Four typed entries exercise entities, enhanced entities, aggregate
enhancement and owned enhancement for a scalar query with facet diagnostics.

Each route includes logging on/off, actual missing-table failure, invalid comment
and ASCII/Unicode/long canaries. It compares complete wrapper Debug, physical
rows/counts, exact errors and retained SQL, parameter Debug, intent, outcome and
trace. A subsequent ordinary query on the same service verifies that request-local
classification does not escape into later requests. There is no normalization.

The corpus contains 144 successes and 432 exact errors, 144 initial SQL logs and
144 independent-query logs. Invalid intent prevents dispatch even with logging
disabled. Removed/future private bindings are absent from execution SQL but still
redact intent at successful and failing log boundaries. Ordinary ACTIVE bindings
remain visible.

Native ownership controls exercise explicit Global copies after actual origin
User deletion, pending executor cancellation and Context deletion, and actual
User deletion with an open SQLite cursor. They run with the default PMR resource
disabled. The pending executor is a gated test SPI, not a physical-router or
asynchronous-device claim.

## Boundaries

This supplies the runtime protocol used by the locked source. It does not silently
change frozen generated COUNT transformations that do not request this protocol.
Generator templates and artifacts remain unchanged. Complete generated facade,
nested facet/aggregate/readonly/dynamic model combinations, protected external
sinks, sensitive plaintext controls and final phase qualification remain required.

Terminal gates and source review are recorded in
[the verification record](parity/derived_query_diagnostics_5_0_6.json) and
[source review](parity/derived_query_diagnostics_5_0_6_source_review.json).

Focused native tests pass 2/2, targeted ASan/UBSan tests pass 4/4 with leak scanning
disabled, and canonical module tests pass 4/4. Both profiles include the unchanged
dynamic JSON execution and RequestPolicy corpora. The complete native rebuild
subsequently succeeds. Its 549-test integration finishes with 472 passing and
77 failures: 76 historical corpus baseline rejections and one Context factory
fixture failure. The generated API compiler gate passes. The
[scalar requalification checkpoint](scalar_baselines_5_0_6.md) records actual
original replays and the fixture correction; the full suite is not accepted.

Two additional byte-preserved original facet tests pass in an isolated development
package: repeated facet diagnostics compose with removed COUNT sources, and future
facet bindings protect root rows, entities and streams. The harness uses already
locked futures-util 0.3.34 and adds no project or production dependency.
Its `--complete` option also reproduces all ten unchanged original SQLite facet
tests. This establishes the source suite's outcomes, including shared targets,
missing keys and slow hydration; it does not establish complete matching native
coverage for those additional scenarios.

An independent [360-case composition source corpus](parity/derived_query_composition_source_5_0_6.json)
records direct/nested/aggregate future facets, repeated facet registration,
intermediate COUNT or clone, empty options, five public read paths, logging
switches, missing-table failure and invalid intent. A separate C++ adapter reuses
the existing observer's request-injection boundary and shared runtime; all
**360/360** source/native observations match without normalization. Its standalone
build and live comparison reproduce, and a known wrong privacy projection exits 1.
The same 360 observations pass in a standalone ASan/UBSan build with leak scanning
disabled; the host SQLite archive is not sanitizer-instrumented.
An independent imported consumer also matches 360/360 using the existing terminal
canonical module consumer, PCM maps and module archives. Only the fixture include
path changes to select the import frontend; adapter and observer bodies remain
unchanged. A first harness attempt incorrectly added compile-time `-pthread` to
the PCM configuration; the corrected helper uses link-only `-lpthread`.
Formal CMake targets and native/module comparison tests are now registered and
pass. The module preparer now retains include-site macro state and the actual
canonical fixture destination; missing-path and reordered-include controls both
reject. Canonical verification covers 166 bodies. Initial scanner/compilation
failures remain recorded. The original 576-case corpus remains unchanged; full
native integration still has separately recorded historical baseline failures.

## Reproduce

```sh
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --target teaql_derived_query_diagnostics_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo build --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --offline --bin derived_query_diagnostics_5_0_6
python3 tests/parity/generate_derived_query_diagnostics_5_0_6.py \
  --cpp build_native_current/teaql_derived_query_diagnostics_probe \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/derived_query_diagnostics_5_0_6 --check
ctest --preset native --output-on-failure \
  -R '^derived_query_diagnostics_(rust_parity|context_contracts)$'
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
  CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 \
  cargo test --manifest-path tests/parity/rust_field_layout/Cargo.toml \
  --locked --offline --bin original_request_trace_5_0_6 count
python3 scripts/verify_original_facet_diagnostics.py
python3 scripts/verify_original_facet_diagnostics.py --complete
python3 tests/parity/generate_derived_query_composition_source_5_0_6.py \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/derived_query_diagnostics_5_0_6 --check
python3 scripts/build_derived_query_composition_probe.py
python3 tests/parity/compare_derived_query_composition_5_0_6.py \
  --cpp /tmp/teaql_derived_query_composition_native \
  --rust /tmp/teaql_cpp_field_layout_rust_506/debug/derived_query_diagnostics_5_0_6
python3 scripts/build_derived_query_composition_probe.py \
  --build-dir build_sanitize_current --output /tmp/teaql_derived_query_composition_sanitize
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tests/parity/compare_derived_query_composition_5_0_6.py \
  --cpp /tmp/teaql_derived_query_composition_sanitize
python3 scripts/build_derived_query_composition_probe.py \
  --build-dir build_modules_native --modules --output /tmp/teaql_derived_query_composition_modules
python3 tests/parity/compare_derived_query_composition_5_0_6.py \
  --cpp /tmp/teaql_derived_query_composition_modules
```
