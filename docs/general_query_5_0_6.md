# General generated Q interfaces: Rust 5.0.6 checkpoint

Current status: G06 and G07 are qualified by the
[complete general-query review](general_query_acceptance_review_5_0_6.md) and
[field-interface acceptance](predicate_projection_acceptance_5_0_6.md).
Earlier checkpoint counts and open-scope statements below remain historical;
other capabilities and P4/P7 retain their independent requirements.

Status: scoped G06/G07/G09 implementation and qualification. The full generator
matrix and P0–P7 goal remain active.

## Source contracts and runtime mechanics

Runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and producer
`45e1572c4956ca963410d8ea436ead3c8857087a` are fixed. The complete unmodified
generated dynamic_json crate supplies the application reference.

Runtime EntityQuery now provides withDeletedRows/deletedRowsOnly,
appendSearchCriteria, filterProperty, pageOffset/pageNumber/pageNumberDefault,
page/pageDefault, offsetOnly/offsetSize/two-value offset, top, streamDefault,
topNProbeParentThreshold, expression/function projection helpers, field/relation
unselect and raw SQL/property/projection boundaries. Generated methods delegate
those responsibilities to runtime/core rather than duplicate ownership or SQL
execution. Existing one-value offset remains available. All application methods
use lowerCamelCase and fluent lvalue/rvalue overloads.

Deleted-row modes remove only the exact default `version > I64(0)` conjunct,
including recursively nested AND groups. They do not rewrite OR branches or
ordinary application criteria. Deleted-only then adds `version <= I64(0)`.
Repeated criteria follow the core expression deduplication behavior.
Property-to-property comparison produces column operands and source-supported
operators; invalid kinds fail with the full original diagnostic.

Page numbers use saturating subtraction/multiplication; zero page numbers and
zero sizes retain source behavior. Default pages use size ten. Offset-only/top
preserve the other slice field; unlimited removes slicing. Full fixed query
state remains visible, including default hard limits and optimization options.

Field unselect removes all ordinary and raw projections of that property but
leaves independent expression aliases. Object unselect removes its foreign-key
projection and matching relation selections. Source property-as/raw/filter APIs
keep distinct storage; computed properties do not become persistence fields.
Raw string conveniences preserve the original trusted-segment behavior. The
UnsafeRawSqlSegment overload requires an explicit owned segment, not an implicit
string conversion. SQL must be controlled by the application; these interfaces
are not a parser, sanitizer or permission grant.

Generated scalar helpers now include source negative contains/start/end and
sound predicates, with authentic centralized human/non-human naming, GBK ordering,
project/raw/unsafeRaw/unselect, and numeric function selections. The later
[projection/text review](generated_projection_text_5_0_6.md) verifies that the
locked SQLite compiler preserves GBK flags through its default argument path;
this is not a locale-collation claim. The earlier unsupported-operation wording
was incorrect. Source guards and further model
variants still require full qualification; this checkpoint is not an exhaustive
G07 surface claim. No ordinary query/expression wrapper extends Context lifetime.

## Evidence and discovered environment issue

**892** original/C++ observations retain complete SelectQuery Debug and failure
messages without output normalization. **780** construct actual generated APIs;
**112** execute real SQLite compact-row queries on the same existing foreign-key
fixture. Cases cover ordinary/minimal Person and Order requests, mutation of
filter modes, pagination extremes/composition, raw/unsafe SQL/property storage,
projection deletion, negative text/sound predicates and GBK behavior.

The comparison exposed the native profile linking system SQLite 3.37.2, whose
prepare errors do not report input offsets. Rust uses newer SQLite and preserves
SQL plus offset in its driver error. Native/sanitize/module presets now select a
separate host archive built from the already locked SQLite 3.53.2 amalgamation,
with thread safety, column metadata and SOUNDEX enabled. Source checksum and build
provenance are verified; no new dependency/download or error normalization is
introduced. The MIPS archive/path is unchanged. This checkpoint does not newly
qualify MIPS runtime feature coverage.

Offset without limit remains the source-generated SQLite syntax failure during
compact-row execution; the plan does not silently insert a different limit.
The full failure (SQL and offset) now matches the frozen source. Existing 1,440
execution and 1,181 dynamic JSON observations also pass on the native archive.

Context tests use null default PMR, transient values, explicit Global retention
and actual source UserContext deletion. Projection/raw/property payloads inherit
the destination resource, including mutation after Global rebinding. Kernel
methods and typed criteria do not introduce implicit queries.

## Reproduction

```sh
source scripts/toolchains/activate.sh
python3 scripts/toolchains/setup_native_sqlite.py --check
cmake --preset native
python3 scripts/generate_examples.py --check
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc CARGO_TARGET_DIR=/tmp/teaql_cpp_field_layout_rust_506 cargo build --locked --manifest-path tests/parity/rust_field_layout/Cargo.toml --bin general_query_5_0_6
python3 tests/parity/compare_general_query.py --cpp build_native_current/teaql_general_query_probe --rust /tmp/teaql_cpp_field_layout_rust_506/debug/general_query_5_0_6
```

Final gates completed after every compiler/linker exited successfully:

| Gate | Result |
| --- | --- |
| Relevant native API/provider/candidate/constant/checker regressions | 11/11 |
| Targeted ASan/UBSan general Q/Context and retained execution/JSON corpora | 4/4; leak scanning disabled |
| Module import contracts | 1/1 |
| Java generator/field-layout/naming/pluralization | 36/36 |
| Application compiler contracts | 3 positive, 2 negative |
| Generation reproduction | 15 C++ sets, 165 files |
| Formatting | 919 handwritten files checked |
| Module exports | 1,458 entries across seven modules |
| Local toolchain/source/archive checks | Pass |

The old native SQLite shared library is not used by current native/sanitize/module
profiles. The new host archive is not sanitizer-instrumented; these sanitizer
gates qualify the project code and its Context/lifetime boundaries, not SQLite
internals. MIPS and physical device execution are not part of this checkpoint.

## Remaining work

- Complete membership conversion and model configurations (see
  [generated_matching_any_5_0_6.md](generated_matching_any_5_0_6.md)), property/criteria/temporal/range
  surfaces, all function/alias/removal and ordering variants/configurations.
  [generated_field_criteria_5_0_6.md](generated_field_criteria_5_0_6.md) now records
  the generic operators, managed independent criteria and before/after/range
  checkpoint; its scoped cases do not close all configurations.
- Grouping/aggregate/facet/roll-up runtime and generated surfaces, relation-aware
  deletion, inherited/constant/candidate/optional/business-ID variations.
- Complete grouped/raw/derived query execution, stream and page configurations,
  trust/policy/privacy/trace matrices and asynchronous interleavings.
- Native SQLite dependency change is a profile checkpoint, not a relabeling of
  historical 5.0.5 corpora or a complete 5.0.6 migration claim.

At this historical checkpoint no complete generator capability was qualified.
Current capability acceptance is tracked by the active ledger; G06/G07 remain
partial until their complete gates and source dispositions are closed.

## Request-state follow-up

[The request-state checkpoint](generated_request_state_5_0_6.md) restores four
state views and the direct consuming core conversion. Actual complete original
execution agrees for 432 configurations/3024 views and conversions. The later [core query view checkpoint](generated_core_query_view_5_0_6.md)
qualifies the source bare query(&self) responsibility through explicit-domain
coreQuery snapshots. Existing integrated C++ query() callers keep their separate
boundary. These two checkpoints close the named state/conversion responsibilities;
they do not establish complete G06/P4 acceptance.

## Scalar execution follow-up

The [scalar predicate execution matrix](query_predicate_execution_5_0_6.md)
extends the construction evidence to 5,936 exact original/native SQLite outcomes
across all seventeen scalar branches, including optional dates/timestamps,
Boolean availability, DateRange and all four order variants. Typed DATE/TIMESTAMP
storage produces 4,656 successes, 1,280 authentic unsigned-binding errors,
2,840 typed snapshots and 4,656 ordered safe SQL records. The earlier TEXT-schema
fixture's 534 typed materialization errors are preserved separately. Generic
membership/selection/matching input repairs are recorded in the
[active source review](query_surface_review_5_0_6.md); the initial remaining-work
list above is historical, not a statement that those repairs are still absent.
Complete source/guard dispositions, configured model variants and composed
policy/raw/projection/page/stream integration remain required for G06/G07/P4/P7.
