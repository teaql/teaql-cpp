# Scalar predicate execution: Rust 5.0.6

The reference runtime is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the
unchanged generated domain comes from generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. This extends G06/G07/G13/G14/G17
execution evidence; complete capability and phase acceptance remain open.

## Required behaviors and observation boundary

The existing field-branch corpus establishes construction behavior. This matrix
instead executes generated requests through each language's real Context,
data service, SQL executor and SQLite provider. Neither observer implements
filtering, decoding or persistence. The generated domain and the producer are
unchanged. The independent Rust observer checks package versions and registry
checksums against the locked reference before its results are accepted.

The matrix covers all seventeen scalar branches of the retained rooted model:
ID/version; ordinary and optional strings; text/password; Boolean/optional
Boolean; Decimal/optional Decimal; double/float/integer; and date/timestamp with
optional variants. Source-selected methods cover equality, inequalities,
before/after, inclusive DateRange, membership, availability, Boolean positive/
negative predicates, and ascending/descending ordering with and without GBK.
ID, version and Boolean guard exclusions remain distinct. Every operation is
applied twice, preserving expression deduplication and ordering behavior.

Operand-bearing methods receive native values, NULL, Unicode text containing
an embedded zero and the largest unsigned 64-bit integer. Ordinary/minimal
projection, default/deleted-row inclusion and compact-row/typed-list execution
are crossed explicitly. Complete query Debug, returned values, typed JSON,
field availability, errors and ordered SQL records are compared directly.
SQL observations retain SQL, parameter Values, comment, purpose and trace nodes.
Timing and other log-entry metadata are outside this observation shape.
Password predicates exercise actual safe binding redaction and canonical trace
publication; this is not a general authorization-policy qualification.

## Schema distinction discovered during verification

The first 5,936 original/native executions used the existing
`query_field_branches_sql.sql` fixture. All outcomes agreed, but 534 full typed
reads rejected `day: Text("2026-10-14")`: that fixture declares temporal columns
as TEXT. Its earlier float/reference checkpoint only required compact rows.
The initial matrix, full observer snapshots and executable hashes are retained
in [the TEXT-schema checkpoint](parity/query_predicate_text_checkpoint_5_0_6.json).
Those source errors are evidence of an invalid typed storage shape, rather than
successful typed temporal reads.

The separate `query_predicate_typed_sql.sql` fixture retains the same data and
changes date declarations to DATE, timestamp declarations to TIMESTAMP and
timestamp values to integer milliseconds. These follow the original SQLite
provider's `DataType::Date`/`DataType::Timestamp` schema mapping and timestamp
binding contract. No production behavior or historical fixture is rewritten.
Only the fixture path changes in the two observers; the recorded source
snapshots allow that difference to be checked exactly.

## Typed-schema results

All **5,936/5,936** actual original/native outcomes agree: **4,656** succeed and
**1,280** preserve the exact original unsigned-binding overflow error. Successful
requests return **2,840 compact rows** and **2,840 typed JSON/load-state snapshots**,
including **1,420** snapshots from full projections. The **4,656** ordered safe SQL
records agree, including password-parameter redaction and trace nodes. An
unsuccessful provider attempt is not counted as a successful result.

Native CTest and imported-module CTest each pass **1/1**. Both execute the full
matrix. All **744** canonical consumer/fixture bodies verify, and pinned format
apply/check covers **1,545** handwritten files. The first module-consumer command
omitted its required build-directory argument; its usage error was corrected
before the successful verification. A subsequent SDK-integrity assertion used
the C++ provenance key for the Rust manifest; it was corrected to the authentic
`generator_commit` key and verified before the final CTest runs. See the [terminal record](parity/query_predicate_execution_terminal_5_0_6.json.gz).

## Ownership and scope

The C++ observer disables default PMR allocation, registers the SQLite connection
as a UserContext resource, and destroys all observation values before actually
deleting UserContext. Every scenario requires zero remaining live User bytes.
Q wrappers, nested operands, results and diagnostics remain Context-bound.
The same canonical source is registered as an imported-module consumer.

This matrix does not qualify independent request policies, all custom model type
configurations, raw/computed/grouped projections, page/stream interleavings or
all remaining G06/G07 requirements. The [general query review](query_surface_review_5_0_6.md)
and capability ledger retain their full requirements. No generator change,
Java/regeneration gate, sanitizer, MIPS or full-suite run follows from adding an
execution observer.

## Reproduction

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_query_predicate_execution_probe
CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=/usr/bin/cc \
CARGO_TARGET_DIR=/tmp/teaql_cpp_inherited_business_binding_bridge_506 \
cargo build --offline --locked \
  --manifest-path tests/parity/rust_query_predicate_execution_5_0_6/Cargo.toml
TEAQL_LOG_ENDPOINT=off python3 tests/parity/compare_query_predicate_execution_5_0_6.py \
  --cpp build_native_current/teaql_query_predicate_execution_probe \
  --rust /tmp/teaql_cpp_inherited_business_binding_bridge_506/debug/teaql_query_predicate_execution_506
ctest --preset native --output-on-failure -R '^query_predicate_execution_rust_parity$'
```
