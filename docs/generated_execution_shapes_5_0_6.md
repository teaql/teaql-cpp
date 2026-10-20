# Grouped and aggregate execution/page qualification

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This checkpoint supplements G09/G08/G13/G17 evidence; complete capability and
P4/P5/P7 acceptance remain open.

## Actual source execution

The original observer calls the unchanged complete generated dynamic-json domain
through its generated runtime and real SQLite provider. C++ uses the same existing
schema/data and generated domain through its separate Context/service/executor/
provider layers. Neither observer implements query or persistence algorithms.
Original generated files, historical observers and historical corpora are unchanged.

The **6,048/6,048** exact execution matrix crosses four models (Person, Product,
Order and Line), ordinary/minimal projections, native/adapted Person results,
count/exists/first/one/list/page, ordinary/ID-set/continuous configurations,
zero-sized/offset pages and actual missing tables. Nine query shapes cover plain
requests, grouping by ID or a scalar, count and sum metrics, grouped sums,
empty groups, a preexisting aggregate colliding with COUNT_ALIAS and unavailable
statistical functions.

The matrix retains **4,400** successful outcomes, **1,648** complete errors and
**5,360** ordered safe SQL log records, including bindings and intent. Those log
counts are not physical statement counts: generated application executors do not
publish every failure statement as an ordinary SQL record. Comparison performs
no SQL, result or error normalization.

Grouped count returns the source first count-row value rather than silently
counting every group. Empty grouped results preserve the missing/non-numeric count
error. Existing aggregate state survives derived count construction. A statistical
aggregate can still cause count to fail because count does not erase all prior
aggregates. Exists retains its selected query shape while replacing its limit.
Typed aggregate list/page results follow the original decoder and result-model
adaptation; they are not converted into fabricated fully loaded business objects.

## Reference dependency repair and preserved historical evidence

The first build used the historical shared `rust_field_layout` development
workspace. Its Cargo lock has **46** registry identity differences from the
reference. That compilation is excluded from this checkpoint's source evidence;
the original workspace and lock remain untouched.

The independent `rust_execution_shapes_5_0_6` observer instead verifies every
registry version/checksum and runtime package version against the reference lock.
Its direct async-trait dependency is pinned to the original 0.1.89 and remains
Rust test tooling only. Adding three deferred observers initially exposed missing
direct development dependencies; the manifest was corrected before recording
their outcomes. No C++ production dependency was introduced.

Three deferred observers are copied byte-for-byte from their retained originals,
with a hash manifest. The new workspace reproduces the existing complete corpora:

| Matrix | Actual original/native outcomes |
| --- | ---: |
| New grouped/aggregate execution | 6,048 |
| Retained generated execution | 1,440 |
| Retained pagination conveniences | 8,448 |
| Deferred ordinary pages | 108 |
| Deferred ID-set pages | 252 |
| Deferred continuous pages/interleavings | 640 |
| **Total** | **16,936** |

All match the immutable corpora. Only the new observer accepts an absent shape
as plain, allowing old stimuli to call the same original APIs. This does not
rewrite historical input/output records or the generated implementation.

## Ownership and verification

The new C++ observer disables default PMR allocation. Input, result and query
values leave scope before actual UserContext deletion, which must retain no User
bytes. Its SQLite connection and executor resources use Context registration.
Request/result ownership is unchanged and there is no additional allocation scope.

The new native, imported-module and ASan/UBSan checks each pass **1/1**, with leak
scanning disabled. The independent original replay checks all six matrices.
Existing scalar pending/cancellation/Context ownership assertions remain a
separate native check. Formatting/check covers **1,438** C++ files; **646** canonical
module consumer/fixture bodies verify. No production function, generator or
generated artifact changes, full-suite run, MIPS or physical-device result is
claimed. Java/generation reproduction is not rerun for observer-only changes.

## Reproduction

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_execution_shapes_probe
python3 scripts/verify_generated_execution_protocols_5_0_6.py
ctest --preset native --output-on-failure \
  -R '^(generated_execution_shapes_rust_parity|generated_execution_context_contracts)$'
```

The helper waits for a successful locked original build before executing any
observer. It rejects dependency drift and checks exact deferred observer copies.
The wider relation/facet, inherited/constant/dynamic model, policy/privacy and
final integration requirements retain their full scope. See the
[terminal record](parity/generated_execution_shapes_5_0_6_terminal.json).
