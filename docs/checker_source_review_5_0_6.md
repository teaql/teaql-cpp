# Complete Checker source review

CK01-CK22 qualify the complete named source under the explicit C++ Context,
owner-thread and enum/API adaptations below. Native11/imported11, all five
original tests and 2,716 exact observations per frontend pass. The independently
qualified generic-constructor repair supplies the missing compiler/capability
and lifetime evidence. Whole capabilities and P0-P7 remain separate.

Reference: all 745 lines of Rust 5.0.6 `teaql-runtime/src/checker.rs` at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`, including its five embedded tests.
The earlier [Checker contract](checker_api_contract.md) preserves its historical
5.0.5 review, source ranges and execution evidence.

## Complete responsibility mapping

| ID | Original responsibility | C++ mapping and evidence |
| --- | --- | --- |
| CK01 | Object status | Preserve the exact marker key, Create/Update/Unknown names, Copy/Clone/equality/Debug and Value conversion. Recognized create/update markers override ID; unknown/malformed markers fall back. Only absent/plain NULL ID means Create. Rust is_create/is_update map to direct enum equality in native code; status marking and clearing remain explicit helpers. |
| CK02 | Rules | Preserve all eight variants, exact wire IDs, equality and complete derived diagnostics. C++ enum-name/wire helpers replace Rust enum methods without changing rule identity. |
| CK03 | Location models | Preserve Member/Index, root/default/hash/array construction, ordered append, Clone/equality, level and is_root. Private segments retain their independent destination ownership. Empty members are not equivalent to an empty location. |
| CK04 | Model/native/Display paths | Model and native paths preserve canonical spelling; model rendering tests accumulated text before adding a dot, while Display tracks segment position and uses `$` only for no segments. Numeric indices retain the standing native-width boundary. |
| CK05 | Naming profile | Default CamelCase, strict empty/camelCase/snake_case/PascalCase tokens, unchanged unsupported-value text and complete diagnostics. Underscore splitting preserves the first part and Unicode uppercase expansion; Pascal additionally uppercases the resulting initial scalar. No acronym normalization is added. |
| CK06 | JSON Pointer | Render member casing first, escape tilde before slash, retain numeric decimal indices and empty members. Native Unicode tables retain the original Rust Unicode 17 mapping. Invalid UTF-8 cannot be represented by Rust String and retains its native rejection boundary. |
| CK07 | CheckResult models | Preserve all seven public fields, optional input/system Values and message/entity/source path, independent Clone and source PartialEq including NaN. No serializer is invented for raw CheckResult. |
| CK08 | Factories/builders/Display | Required, min/max, min/max-string factories set their distinct rules and input/system values. Builders replace only their named field. An explicitly present empty message wins over location/rule Display. Native scoped Value conversion retains the source Into responsibility. |
| CK09 | Wire models | Preserve all eight WireCheckResult fields and Property{name}/Index{index} variants, independent Clone, equality and complete Debug. Raw diagnostics remain distinct from publication/privacy projections. |
| CK10 | Wire conversion and serialization | Keep canonical property segments, profile-rendered instancePath and independent submitted sourceInstancePath. Convert Value optionals through their actual JSON codec; None and Some(Null) differ before wire conversion. Emit every camelCase optional key as explicit JSON null when absent and retain tagged segment JSON. |
| CK11 | Checker SPI and defaults | entity and check_and_fix remain required independent callbacks. Required reports only absent/plain NULL; string limits inspect only Text, count Unicode scalars, append the field location and preserve original input/bound. Generated requiredText/value helpers have their own broader rules and are not substituted here. |
| CK12 | Registry | Default/new/Clone, sorted entity keys, replacement, fluent installation and missing lookup retain the actual callback. Calling checker.entity during registration is guarded. Native checked resource borrows replace Arc sharing without extending callback lifetime. |
| CK13 | TypedChecker | Keep the arbitrary T typed callback SPI separate from the erased Checker adapter. Status, Context, mutable entity, location and findings remain separate arguments. No Entity constraint is imposed on the typed SPI alone. |
| CK14 | Generic carrier and construction | Source types can be named without model bounds; construction requires TeaqlEntity alone. The reproduced early C++ constraint is repaired with an inert primary carrier. Metadata-only, unrelated-logic and noncloneable-model construction do not grant Checker dispatch. The constructor matrix observes all five metadata calls and the valid erased name. |
| CK15 | Erased capability and Clone | Only Entity, allocator-aware copy capability and matching typed logic select the Checker specialization. Four paired negative controls preserve missing metadata at construction and missing Entity/Clone/logic at erasure. Source Send/Sync retains the standing native owner-thread adaptation. |
| CK16 | Annotation extraction and decode | Determine status before taking values. Remove loaded/dirty annotations even when malformed; List keeps only Text and collects sorted unique fields, including Some(empty). Decode the original sparse record before installing optional checker views. The 1,362 original scripts observe field boundaries, direct/Context/module calls, decode failure and callback order. |
| CK17 | Lazy dirty snapshot | The only 5.0.5-to-5.0.6 body change defers the extra MutationValues copy until decode succeeds and dirty fields are Some. Native materialization already follows that branch. Readonly checks do not build an unused dirty snapshot; annotation presence and actual loaded/dirty semantics remain separate. |
| CK18 | Typed callback and before/after state | Clone the materialized entity and serialize its before-view, invoke the selected typed callback once, then serialize the after-view. The constructor repair leaves this entire valid check_and_fix function body byte-identical. No extra Fix execution or callback algorithm is introduced. |
| CK19 | Required typed findings | Check nonnullable properties except ID/version. If the original was absent/plain NULL, require an after-value that is neither absent/NULL nor merely the unchanged materialized default. Typed adapter findings append independently; Context metadata-required suppression is a separate responsibility. |
| CK20 | Sparse merge | Restore the original sparse boundary and insert only after-values unequal to before-values. Do not expand missing columns into mutation intent or erase an original field simply because the typed view omits it. Preserve Value equality semantics and remove checker-only annotations before restoration. |
| CK21 | Materialization failure | Restore the sparse original after removing loaded/dirty annotations, append InvalidType at the supplied location with exact entity/materialization message, and never invoke typed logic. Callback throws retain their native exception contract and detach synchronous guards before reclamation. |
| CK22 | Ownership and original tests | Allocate nested paths, Values, JSON and names in explicit Global/User domains. Reject premature/reentrant close, expired payload use and shorter-lived retention. Explicit Global copies survive actual User deletion. Execute all five original checker tests and the complete canonical native ports; separate locale/catalog assertions retain their own source scope. |

This reviews full bodies, public/default/derive protocols, generic bounds and
failure order. It does not merge Checker with Context, generated logic, data
services, mutation governance or SQL/provider execution.

## Exact-source and frontend observations

Three retained drivers compile through the independent reference-locked
`rust_checker_source_5_0_6` project. All dependency versions/checksums match the
frozen reference. The original test view preserves all 67 source/test/fixture
bodies and appends no observer; its 273 test names and exact five checker tests
are checked. Successful compilation precedes execution.

| Family | Retained actual-source inputs |
| --- | ---: |
| Typed/default/Context Checker scripts | 1,362 |
| Complete model/Clone/Debug/wire observations | 220 |
| Paths, factories and runtime checks/errors | 1,134 |
| **Total per frontend** | **2,716** |

The larger runtime-check family overlaps the separate error source; its passing
count does not accept every error/caller responsibility. Comparison performs no
normalization. The model family keeps all nine formatter modes, optional NULL,
NaN/negative zero, Unicode, empty and escaped paths and exact serialized output.

Three old model/original-contract entries now run through canonical imports;
the new constructor contract adds a fourth. Existing body assertions, original
observers and immutable corpora remain unchanged.

## Reproduced constraint repair and limits

The [constructor review](checker_constructor_boundary_5_0_6.md) records the actual
Rust-success/C++-failure witness, shared runtime repair and paired positive/
negative controls. Existing 2,716-result gates were already green before that
repair and did not prove the missing constructor contract. They retain their
original checkpoint in [the earlier execution](parity/checker_source_execution_5_0_6.json.gz).
The current constructor-inclusive replay has its own receipt.

The first new lifetime assertion caught logic_error for an expired managed
reference. The actual API correctly throws ReferenceError; its
[failure record](parity/checker_constructor_initial_lifetime_fixture_failure_5_0_6.json.gz)
preserves the probe and failed run. The fixture now catches the exact API error.
No runtime exception type, expected source result or old assertion was changed.

Runtime naming-profile parsing rejects invalid tokens in both languages. This
does not settle G12/G14's distinct invalid generator-model naming behavior.
Generated model/clock/Business-ID fixes retain their accepted G02/G03 source and
actual persistence evidence; Checker source alone does not accept all save,
privacy, trace, caller or generated-assembly responsibilities.

Historical whole-suite failures, source-specific platform/width adaptations,
remaining capability decisions and all P0-P7 exits retain their full scope.
No GPU, external integration or physical-device evidence is inferred.

## Current qualification

The [current source replay](parity/checker_source_constructor_retry_execution_5_0_6.json.gz)
binds exact dependency identities, original source compilation, original tests,
all three immutable families and native11/imported11. The independent
[constructor gate](parity/checker_constructor_execution_5_0_6.json.gz) adds five
paired construction cases, four paired refusal programs, existing generated
compiler checks and targeted native/imported/sanitizer ownership checks. Its
existing generated native regression matches 1,620 direct/preflight and 540
governed SQLite Save outcomes. The five native/imported dispatch tests overlap
the eleven-test selection; they are not counted as extra unique checks.

The [preservation record](parity/checker_constructor_preservation_5_0_6.json.gz)
keeps all SDK/protected bytes and original observer/assertion bodies unchanged.
The only production change is the shared header's constructor/capability and
borrow-helper organization. The valid materialization/Fix/sparse-merge body is
byte-identical. Four additional canonical entries use existing model/original
tests and the new constructor contract; module comparison registration checks
targets exist, including when testing is disabled. The separate modules
configure with BUILD_TESTING=OFF succeeds; no installed/release execution is
inferred from configuration alone.

The [whole-source qualification](parity/checker_source_qualification_5_0_6.json.gz)
promotes only checker.rs: **56 accepted / 66 pending / 2 excluded_external**.
All seventeen capability verdicts remain unchanged. The native discovered count
is not a current full-suite pass. The new header is formatted with the pinned
tool; generator/SDK reproduction and MIPS are not rerun for this change.

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_checker_source_5_0_6.py --modules \
    --record /tmp/checker_source_fresh_5_0_6.json.gz
python3 scripts/verify_checker_constructor_5_0_6.py --sanitize \
    --record /tmp/checker_constructor_fresh_5_0_6.json.gz
```
