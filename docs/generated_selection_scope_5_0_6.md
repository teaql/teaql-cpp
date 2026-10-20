# Generated selection, removal and projection scope

Runtime: Rust 5.0.6 at `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This source review and actual ordinary-model matrix qualify the recorded
self/relation selection family. Full G06/G07/P4/P7 remain separate requirements.

## Original responsibilities and actual results

The [source review](parity/selection_scope_source_review_5_0_6.json) retains fourteen
complete original methods: self/self-fields/without-parent/all/children/any,
forward selection/selection-with/removal, scalar raw/unsafe-raw/removal, grouping
alias and text search. Their source lines and body hashes supplement behavior;
method names or hashes alone are not acceptance. Fixture SQL hashes are checked
before replay. The original generated domain remains unchanged.

All **288/288** actual original/native SQLite outcomes agree and succeed.
Person, Order and Product cross full/minimal roots, twenty-four operation
sequences and compact rows versus typed lists. Sequences exercise repeated
selection, raw scalar removal/reselection, independent grouped aliases and
readonly properties, explicit child options followed by relationship removal,
selection replacement, search and deleted-row controls.

Complete integrated query Debug, independent options, relation-selection state,
actual compact maps or typed JSON/load-state maps, and all **468** safe SQL records
agree without rewriting results. The typed observations contain **330** entity
snapshots. They are actual runtime/query/provider outcomes, not a local graph
implementation or a schema-bootstrap acceptance claim.

The following distinctions are preserved:

- selectSelfFields and selectSelfWithoutParent delegate to self selection; they
  do not drop native fields merely because a parent exists.
- selectAll includes forward and reverse to-one relationships. selectChildren
  adds reverse collections; selectAny delegates to it.
- A bare named relation selection does not import a child Q's default live-row
  filter. In this fixture, Person.selectChildren includes the already soft-deleted
  Order 13. Explicit child requests carry their own filters and projection.
- Scalar removal clears its ordinary/raw projections but preserves independent
  expression aliases and readonly properties. A grouped `_scalarAlias` therefore
  remains available while the removed native field is NotLoaded.
- Object removal clears the FK projection and matching core relation selection;
  replacing it later with selectAll uses the named default selection again.
  Scalar and object removal have separate source responsibilities.

Missing fields remain unavailable in typed load state and are omitted from JSON;
loaded values, nullable relationships and loaded empty collections preserve their
own source presentation. JSON readback performs no additional SQL after loading.
Logs are collected after loading and presentation, so comparison includes the
complete recorded execution path.

## Fresh general-query dependency replay

The full retained general_query observer and driver are copied byte for byte into
the independent original-registry workspace. All **892/892** live original/current
C++ observations match the unchanged historical corpus. This supplies current
reference dependency evidence for the old general query family, rather than
using the older root project's differing registry resolution.
The source review records both original/copy hashes. No historical source or
corpus is overwritten. The existing GBK unsupported-operation wording is corrected
in general_query_5_0_6.md using the separately qualified projection/text compiler
review; default pass-through is not a locale-collation guarantee.

## Ownership and generated guidance

The observer disables default PMR, explicitly deletes UserContext after each
request, and verifies zero remaining Global live User bytes. Query/options,
SQLite resources, decoded entities and presentation payloads stay in that scope.
Existing general-query lifecycle checks separately cover destination copies,
expired requests and Global rebinding. No runtime algorithm, production dependency
or additional ownership domain is introduced for this matrix.

Generated Assist now states the named-relation version boundary and recommends
explicit select...With(child_request) when child filtering is intended. A Java
regression checks the guidance. Generation changes instructions only; all generated
C++ bodies remain byte-identical under the complete SDK reproduction proof.

## Reproduce and remaining acceptance

```bash
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_generated_selection_scope_probe teaql_general_query_probe teaql_general_query_tests teaql_generated_request_state_probe
python3 scripts/verify_selection_scope_5_0_6.py
ctest --preset native --output-on-failure -R '^(generated_selection_scope_rust_parity|general_query_(rust_parity|context_contracts)|generated_request_state_rust_parity)$'
```

Relevant native checks pass **4/4**, imported execution **1/1** and ASan/UBSan
**1/1**, with leak scanning disabled. Java checks pass **57/57**. All **24**
workspaces/**288** files and **44** SDKs strictly reproduce; **30** SDK Assist files
change and **zero** C++ code files change. Format/check covers **1,449** files;
**665** canonical consumer bodies and **1,485** exports verify. An integrity check
run before provenance publication finished is retained as a preparation failure;
its terminal replay passes after the existing pipeline completes.

The import target is teaql_module_generated_selection_scope_probe; its test has
the module_ prefix. Sanitizer uses the native names in the separate profile,
with leak scanning disabled. See the [terminal record](parity/selection_scope_5_0_6_terminal.json)
for completed native/module/lifetime/generation gates.

The [general source surface review](query_surface_review_5_0_6.md) keeps broader
no-forward-relation, inherited/constant/candidate/optional configurations and
complete policy/privacy/pagination/cache/async and phase integration required.
This matrix establishes the documented ordinary shapes, not every combination
or a full capability. No full-suite, MIPS or physical-device run is inferred.
