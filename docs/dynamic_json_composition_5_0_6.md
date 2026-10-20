# JSON conditions, trusted query metadata and RequestPolicy

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This qualifies the recorded protected query-configuration composition. It does
not establish successful persistent dynamic-field hydration or complete G16/G17.

## Actual original/native matrix

The unchanged original generated ordinary domain executes against the existing
SQLite schema/data. The C++ observer uses its generated domain and registered
runtime/provider, with no local SQL/filter implementation. The two independent
observer projects retain original registry versions/checksums.

All **9,216/9,216** outcomes agree: **5,340** successes and **3,876** errors.
Four models cross ordinary/minimal projection, four policies, six execution forms,
four JSON inputs, logging switches and six trusted query configurations:

- No extra selection.
- Readonly `_computed` property with the trusted literal expression `7`.
- Readonly `_computed` property with the trusted expression `id + 7`.
- Explicit empty persistent dynamic-field selection.
- Select all persistent dynamic fields.
- Select the typed persistent field `title`.

JSON inputs include ordinary criteria and attempted `_computed`, `#title`,
`_dynamicFieldDefinitions` and `_dynamicFieldSelection` overrides. Translation
does not turn those inputs into trusted definitions, selections or SQL snippets.
Original request metadata and authoritative runtime configuration remain separate.

The four policies allow, restrict an ID, restrict and replace slicing, or deny.
List, one, stream, count, exists and page preserve actual results, complete errors,
before/after policy snapshots, invocation order, aggregates, intent/trace and
safe SQL/parameter records. Observations include **11,904** policy callbacks and
**4,320** retained SQL records. None of the **2,304** denied requests reaches a
SQLite SELECT-authorizer callback. Logging switches do not disable governance.

There are **1,572** source `DYNAMIC_FIELD_PROVIDER_MISSING` errors in configurations
without a persistent provider. Empty results and scalar execution retain their
original differences; the observer does not replace failures with fabricated
field values. These are protection-boundary observations, not successful storage
loading, carrier validation or a complete persistent-provider test.

## Ownership and source review

The new native observer retains the existing null-default-PMR discipline.
Queries, policy observations and SQLite resources remain Context-owned; no new
allocation domain, shared owner or production dependency is introduced.
Existing explicit User deletion/Global query and diagnostic-copy contracts remain
part of the relevant native integration selection.

Eight actual original generated translation bodies are retained with complete
text, source lines and hashes alongside shared C++ runtime mappings. They cover
parse, non-object handling, field/path translation, ordering and page precedence.
The review supplements actual execution rather than claiming parity from names
or hashes. See [the body review](parity/dynamic_json_composition_source_body_review_5_0_6.json).

The existing exact quoted-Line descriptor Debug adaptation applies to historical
query/policy snapshots. No SQL, result or error rewriting is added. Runtime,
generator, original generated files and old observation corpora remain unchanged.

## Verification and remaining acceptance

The new native, imported module and ASan/UBSan checks each pass **1/1**, with leak
scanning disabled. All **nine** current native JSON integration tests pass,
covering ordinary, scalar, inherited, constant/candidate/BID, one/stream, policy
and checked Context contracts. **659** canonical consumer/fixture bodies verify;
format/check covers **1,443** C++ files. See the
[terminal record](parity/dynamic_json_composition_5_0_6_terminal.json).

```bash
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_dynamic_json_composition_probe
ctest --preset native --output-on-failure -R 'dynamic_json'
```

The [G10 matrix](dynamic_json_qualification_matrix_5_0_6.md) records this composition
separately from successful trusted persistent storage, readonly/facet/privacy
configurations and full phase/integration acceptance. No full native suite,
generator/Java refresh, MIPS or physical-device verification is inferred.
