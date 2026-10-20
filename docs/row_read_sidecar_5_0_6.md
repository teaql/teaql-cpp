# Read-result sidecar presence and row enhancement: Rust 5.0.6

References are runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
producer `45e1572c4956ca963410d8ea436ead3c8857087a`. This continues the
[complete query-state review](query_full_state_5_0_6.md). The named carrier-presence
responsibility is qualified; no whole-source or capability promotion follows.

## Source responsibility

`CompactRow` has an optional `LoadedRelationResults` allocation. Its optional
presence is distinct from its contents. The carrier holds query-result relation
lists and optional persistent dynamic-field values; it is not an SQL column,
mutation payload, availability mark or persistence authority.

| Original method | Responsibility |
| --- | --- |
| `set_loaded_relation` | Allocate the carrier if absent and replace the named list, preserving the list's own loaded state. |
| `take_loaded_relation` | Remove only the list; leave a present carrier allocated even after removing the last list. |
| `set_loaded_dynamic_fields` | Allocate the carrier if absent and replace its optional dynamic payload. An empty payload is distinct from no payload. |
| `take_loaded_dynamic_fields` | Remove only the optional dynamic payload; preserve the carrier. |
| `has_loaded_relations` | Report whether the list map has entries; it does not report carrier presence or dynamic payload availability. |
| `clear_loaded_relations` | Discard the entire carrier, including dynamic read metadata. |
| `try_extend` / `extend` | Validate schemas and final values before merging. A present right carrier makes the left carrier present, even when the right list map is empty and its dynamic payload is absent. |
| Clone, equality and Debug | Preserve and distinguish no carrier, an allocated empty carrier and a carrier containing an empty dynamic payload or loaded/unloaded list. |
| Map/JSON presentation | Serialize projected row values; read metadata is separate. |

The source's `try_extend` unconditionally calls `get_or_insert_with(Default::default)`
inside its right-carrier branch before extending the list map. The C++ branch
previously called `set_loaded_relation` only for actual list entries, so an empty
right carrier could disappear. Ordinary `has_loaded_relations` and projected JSON
both hide this difference; complete equality and diagnostics expose it.

The [actual semantic witness](parity/row_read_sidecar_semantic_gap_5_0_6.json.gz)
preserves all 972 observations and 24 differences. They occur only when the left
has no carrier and the right has an empty carrier obtained by taking either a
relation or a dynamic payload. All four affected outputs concern presence:
merged Debug, equality to a pristine row, Debug after taking a relation and Debug
after taking dynamic values. Projected values and JSON remain unchanged.

The repair adds one `read_metadata()` call inside the existing right-carrier
branch before its list loop. Existing dynamic merges, provenance validation,
error ordering, projected values and immutable layout behavior remain unchanged.
It reuses the existing Context allocator and carrier destructor; no new ownership
mechanism or lifetime is introduced.

## Actual-source observer

A new example in the independent source-observer project calls frozen public
constructors and methods. It implements no row merge, metadata or Debug algorithm.
The C++ observer calls the corresponding shared core APIs through UserContext.
No generator or generated domain is involved.

Nine initial states cover an absent carrier; empty carriers obtained by taking a
relation or dynamic payload; an empty relation list; an empty dynamic payload;
a loaded-NULL payload; long text; a nonempty relation list; and combined relation
and dynamic data. Both checked and unchecked merge entries run with loaded and
unloaded lists and three relation names: ordinary, Unicode with NUL and long.
The full Cartesian input set contains 972 cases.

Each input records complete before/right/merged/Clone state, nine Debug modes,
logical presence, relation and dynamic take results, state after each take,
clear-to-pristine equality, projected JSON and source/clone isolation. Hash-based
collections contain at most one field so their complete original diagnostics
have stable order; no output sorting or normalization is used.

The native observer additionally copies each merged row and its expected Debug
into Global through UserContext. It destroys the source UserContext, rebinds the
Global references through a new UserContext and compares all nine Debug modes.
Default PMR allocation is disabled. Result JSON is also copied explicitly before
source destruction. These are C++ ownership controls alongside the exact source
comparison, not an implicit extension of the source lifetime.

## Current verification

The first native fixture retained all output records in one GlobalContext and
correctly exhausted its configured byte budget before finishing the batch. That
[failed execution](parity/row_read_sidecar_gap_5_0_6.json.gz) is preserved. The
fixture now uses one GlobalContext per independent input, still destroys the
source UserContext before reading its retained row, and releases all retained
records before the next input. No Context limit is increased or bypassed.

The [qualification record](parity/row_read_sidecar_qualification_5_0_6.json.gz)
joins the [fresh actual-source replay](parity/row_read_sidecar_final_5_0_6.json.gz),
[retained query/error replay](parity/row_read_sidecar_retained_state_5_0_6.json.gz)
and the terminal [native](parity/row_read_sidecar_native_installed_5_0_6.json.gz)
and [imported](parity/row_read_sidecar_modules_native_installed_5_0_6.json.gz) gates.

| Gate | Result |
| --- | --- |
| New carrier-state comparison | 972 exact original/native/imported observations; all prior 24 differences repaired |
| Retained complete query and error comparison | 472 exact observations per frontend; original core 69/69 |
| Native selected regression | 22/22, followed by a no-work build |
| Imported selected regression | 22/22, followed by a no-work build |
| Context retention | Every native/imported carrier case reads its complete retained row after actual source UserContext destruction |
| Canonical module bodies | 956 consumer/fixture bodies verify |
| Formatting | 1,692 handwritten C++ files pass the pinned format/check |
| Artifact preservation | All 78 SDKs/3,457 generated files and 2,201 protected artifacts keep their bytes |

Every compiler, linker and test process exited successfully. Byte proofs isolate
the single carrier-presence call from the previous diagnostic-only repair.
Generated code, generator implementation, prior corpora and frozen source are
unchanged. Artifact checks are not a new Java/generation run. No sanitizer, MIPS,
external integration or physical-device result is inferred.

Native discovery is 760 and imported discovery is 515, not whole-suite pass
claims. Source status remains 50 accepted / 72 pending / 2 excluded_external;
all seventeen capability statuses remain unchanged. Whole query.rs still needs
the combined initialized-cache, checked-enhancement and shared-shape/conversion
body reconciliation. G15's separate quoted-FK decisions, protected orchestration,
complete G13/G17 and P0-P7 retain their requirements.

```bash
set -e
source scripts/toolchains/activate.sh
cmake --preset native
cmake --build --preset native --parallel 2 --target teaql_row_read_sidecar_probe
replay_directory=$(mktemp -d)
python3 scripts/verify_row_read_sidecar_5_0_6.py \
    --record "$replay_directory/sidecar.json.gz"
ctest --preset native --no-tests=error --output-on-failure \
    -R '^row_read_sidecar_rust_parity$'
```
