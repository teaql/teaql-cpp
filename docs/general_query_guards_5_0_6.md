# General query constructor and selection guards

The complete G06 source review now has an explicit
[requirements matrix](general_query_requirements_5_0_6.md). This checkpoint
supplies missing direct evidence for source Default and all ordinary selection
guard branches. The subsequent [Business-ID factory repair](generated_business_id_lookups_5_0_6.md)
addresses the omission found here. G06 is now qualified by the later
[complete acceptance review](general_query_acceptance_review_5_0_6.md). The source
and gate counts below describe this earlier constructor checkpoint.

## Original and native construction

Rust `Request<R>::default()` calls `new()`: project ID/version, retain default
query/options state and add no live predicate. Its result marker is unconstrained.
The explicit C++ mapping is `Request<false, R>(user_context, true, false)`.
The existing native constructor's ordinary full/live defaults are preserved as
a separate convenience; the explicit flags express the source bare request.
There is no implicit allocator or default Context.

The three Q entries keep separate source semantics: ordinary adds self projection
and live version, minimal adds live version without self projection, while
with-children removes slicing, selects self fields and enhances subtypes without
adding the default live filter. Ordinary relation selection is separate from
that subtype enhancement.

The original selectAll guard is true for forward objects or reverse to-one
relations. Its body adds those named relations. selectChildren adds reverse
collections only when that separate guard is true. The aliases self-fields,
without-parent and any retain their source delegate behavior. A false guard is
not inferred from the number of emitted methods: the actual model branches and
execution results are compared.

## Complete observations

The new matrix has 1,400 original/native observations across ten model bindings
in two complete unchanged generated Rust domains. It includes Platform, Person,
Order and Product plus inherited Platform, Specialist, School, Academy,
ResearchAcademy and Enrollment. Each crosses the four constructor states and
seven none/self/self-fields/without-parent/all/children/any selections.

There are 840 construction cases using model/integer/custom markers and 560
actual SQLite executions using compact rows or typed lists. All 560 executions
succeed. The matrix compares eight complete query/options/selection fields;
executions additionally compare exact compact values or typed JSON/availability,
errors and all 837 ordered safe SQL records. Timing fields are excluded; no
declared result, query, SQL or error field is normalized.

Both Rust observers call the original public Default, Q, selection, conversion
and execution APIs. They share observation code and the existing driver; neither
implements an alternative query algorithm. Original files, models, lockfiles,
registry versions/checksums and SQL fixture identities are verified before
replay. The C++ observer calls checked-in generated APIs, disables default PMR,
destroys allocator-aware observations before deleting each UserContext and
verifies zero remaining live User bytes. No production function or generator
implementation is changed for this matrix.

The initial observer build omitted the SqlLogContext debug declaration include.
Its full source and compiler output remain in
[the preparation archive](parity/general_query_guards_observer_preparation_5_0_6.json.gz).
The corrected observer includes that existing declaration; no runtime behavior
was changed to address the preparation error.

## Reproduction

| Completed gate | Result |
| --- | --- |
| Live original/native comparison | 1,400/1,400 |
| Successful physical SQLite operations | 560; 837 matching ordered SQL records |
| Focused native regression | 4/4 |
| Imported complete matrix | 1/1 |
| Complete generic source review | 97 function bodies and seven template helpers |
| Pinned formatting | 1,631 handwritten files formatted and checked |
| Canonical module consumers / exports | 782 bodies / 1,495 names |

The runtime and generator are unchanged. Java, SDK regeneration, sanitizer,
MIPS and full-suite checks are not claimed as new gates for this observer and
source-review change. Existing memory/marker qualification remains independent.

```bash
set -e
source scripts/toolchains/activate.sh
python3 scripts/verify_general_query_source_5_0_6.py
cmake --build --preset native --target teaql_general_query_guards_probe
ctest --preset native --output-on-failure -R '^general_query_guards_rust_parity$'
```

The [source review](parity/general_query_source_review_5_0_6.json) retains the
complete original bodies and model guards, with adjacent capability ownership
explicit. The [terminal record](parity/general_query_guards_terminal_5_0_6.json.gz)
binds current commands, source identities and native/module results. These are
focused construction/selection proofs, not complete G06/P4/P7, full-suite,
MIPS, GPU, external-provider or physical-device acceptance.
