# Dynamic child enhancement: generated caller continuation

Reference: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Source: `teaql-runtime/tests/dynamic_field_query.rs`.

This continuation qualifies two focused original tests, not that complete
1,759-line source or all 23 of its test scenarios:

- `child_enhancement_dynamic_selection_reaches_typed_views_and_survives_root_loading`
- `malformed_dynamic_child_enhancement_fails_without_returning_partial_entities`

The whole source remains `pending_review`. This work supplies a generated,
provider-backed counterpart for child/root enhancement composition that was
not established by the earlier ordinary typed caller and direct-row helper
receipts alone.

## Counterpart and exact observations

`generated_child_dynamic_selection` in
`teaql_runtime/tests/dynamic_relation_tests.cpp` uses the retained generated
`cpp_examples::Product`, the actual SQLite executor and an independent dynamic
provider. The original `School` and C++ `Product` have the tested id/version/name
scalars and indexed/dynamic carriers. Model names, physical table, field-layout
revision and native slot order differ explicitly; no generated code is patched.

The controlled C++ provider constructs the original three owner payload shapes:
text for ID 1, explicit NULL for ID 2 and no value for ID 3. It delegates all
selection/batch validation, hydration and merging to production runtime code.
Like the original provider, it requires a trusted Context marker. It additionally
checks requested owner/count and nonempty validated intent. Definitions contain
`note` and `extra` at revision `extension-v1`.

Two success cases select `note` in the child query; the root either selects no
dynamic fields or selects `extra`. Both cases assert:

- Exactly two executed SQLite statements and three returned entities. The source
  uses a fixed-row executor returning the same ordered scalar fixture; the C++
  fixture observes statements through SQLite's trace hook after setup. This is
  not a general implicit ordering guarantee for SQL queries.
- One dynamic-provider call without root selection, two with root selection.
- Child `note` states Value/NULL/NotLoaded across the three rows; root `extra`
  states follow the same pattern only when selected and otherwise stay NotLoaded.
- No pending dynamic mutations; Value and NULL rows share indexed availability
  geometry while retaining distinct payloads.
- Actual generated `into_json` emits `#note` with the child value and emits
  `#extra` only for a selected root value.

Five malformed-child cases retain the original fault list: omitted owner,
unrequested owner, unselected value, wrong type and wrong owner definition.
Each returns an error with exactly one provider call, returns no partial typed
result and excludes `PRIVATE-CANARY` from the error. Fault injection is confined
to the test provider; no production validation is bypassed or reimplemented.

The existing raw child overlay, flat relation hydration, definition-revision
rejection and saved attachment-key tests remain in the executable as regressions.

## Reproduction and qualification

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_dynamic_child_source_5_0_6.py \
  --record docs/parity/dynamic_child_installed_5_0_6.json.gz
```

The controller requires a fresh receipt path, reproduces the complete generated
example SDK, then executes each of the two unchanged original integration tests
with an exact test filter against a byte-checked runtime view and locked
dependencies. Disabled external-exporter development dependencies are omitted.
It builds and runs the C++ executable in native and imported-module modes,
checks exact CTest selection and the seven-case completion marker, validates
canonical module consumers and no-work builds, then verifies captured inputs
and reference identities are unchanged.

No production runtime or generator implementation change is required by these
seven scenarios. The tests do not qualify every dynamic-query allocation,
stream, graph-serialization or mutation case in the complete Rust file. Nor do
they accept the whole resolved service, dynamic provider/storage, all generated
models or phase exits. Source/capability dispositions stay unchanged; no new
sanitizer, MIPS or physical-device result is claimed.

## Terminal result

[Installed receipt](parity/dynamic_child_installed_5_0_6.json.gz) passes
**original2, generated cases7, native1/module1**, complete example SDK
reproduction, canonical module consumers, no-work builds and unchanged inputs
and references. No source disposition or capability/phase verdict advances:
**92 accepted / 30 pending / 2 excluded_external**, **11 qualified / 6 partial**.
