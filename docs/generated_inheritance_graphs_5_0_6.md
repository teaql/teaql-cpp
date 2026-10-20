# Inherited optional relation graphs: Rust 5.0.6

Current status: G04/P2 is qualified with documented language/ownership adaptations;
normal inheritance generation is enabled. See [the final acceptance](inheritance_acceptance_5_0_6.md).
The checkpoint status and guard statements below are retained historical evidence.

Status: partial P2/G01/G04/G13 evidence. Normal inheritance generation remains
guarded; this fixture does not qualify every inherited model configuration.

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Original generator: `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Configuration and source authority

`tests/generator/inheritance_graph.xml` adds a second inheritance configuration:
School declares an optional Specialist advisor; Academy and ResearchAcademy
inherit that forward field. Specialist declares an explicit human category,
a boolean field and a Platform Context root. Enrollment provides a reverse
relation on School. The initial observations exercise the forward advisor and
nested Platform; the subsequent reverse-collection extension below qualifies
selected Enrollment views and actual collection-member ledger saves.

The original producer generates 43 files plus PROVENANCE.json. The entire original
crate compiles against the frozen runtime without file edits. The paired C++
artifact set remains qualification-only. Eight C++ fixture sets now reproduce
88 files; the earlier seven-set counts remain historical checkpoint evidence.

Both probes create the same SQLite rows and observe all six tables. Negative
cases include a non-null advisor ID whose target is missing. Foreign-key
enforcement is disabled only while seeding that deliberately inconsistent input,
then restored before any observed operation. This is an explicit malformed-data
fixture, not a change to runtime enforcement or a normal application example.

## Relation availability repair

The direct comparison exposed 286 differences in the advisor's loaded flag.
`teaql-runtime/src/data_service/relation.rs` is the source authority:

- Compact forward installation marks fetched targets and explicit NULL keys as
  loaded. A non-null key without a fetched target remains unavailable.
- Unfetched detail belongs to an edge-owned sparse view, containing only the
  target key when present. It does not become a shared identity-table entry.
- Ordinary flat installation publishes graph data without adding compact
  relation-availability markers. An unfetched target is represented by its edge
  detail, without inserting a fake NULL scalar into the parent row.
- An explicitly selected inverse relation owns its projection/filter and must
  not be replaced by an automatically installed inverse snapshot.

The C++ shared relation installer now follows those rules. It validates Context
domains, decodes edge-owned detail through existing Context graph adapters and
keeps the allocator in the query Context. The old native expectation that an
ordinary flat miss inserts a scalar marker was updated against this 5.0.6 source;
historical 5.0.5 corpora were not relabeled.

## State attachment order and actual saves

The matrix retains both attachment orders. Mutating a queried advisor before
attaching its state materializes the source ledger, allowing its original version
and pending changes to be adopted. Four observed cases commit the inherited root
and advisor together; for example, root version 5 becomes 6 and advisor version 3
becomes 4, with its boolean set to false.

In the mutate-after-attachment cases, the original lazy source ledger can be
absent during adoption. The original save then attempts a duplicate advisor insert
and rolls back. The C++ result preserves that source failure; the qualification
does not silently rewrite the ordering behavior. Both paths use the generated
public attachment API and the actual audited ledger save boundary.

## Evidence

The initial forward corpus retains **702**
matching observations: **27 reads**, **202 successful saves**, **204 optimistic
conflicts**, **180 Check failures** and **89 Graph failures**. The matrix varies
three inherited models, normal/minimal/with-children queries, present/NULL/missing
advisor identities, scalar changes, explicit NULL assignment, soft deletion,
both attachment orders and root/advisor conflict/trigger injection.

Comparison includes loaded flags, Null/NotLoaded/Value evaluation, native values,
root/related snapshots, nested Platform values, complete diagnostics, deletion
markers, pending ledger fields and all database rows. Failures preserve all six
tables immediately before the save. Conflict and injected-trigger classifications
are explicit; other error strings are retained. Output is not normalized.

The native application test also disables the default PMR resource, performs an
actual two-entity save and checks the committed versions/boolean. It distinguishes
a loaded null advisor, explicitly copies the nested graph into Global storage,
deletes the source UserContext and verifies expired borrows and surviving
destination-owned values. A named-module consumer runs the same test.

The original-fixture regeneration workflow now validates all unaffected fixtures
while regenerating explicitly named fixtures, and performs full validation after
writing them. The completion verifier cannot use that regeneration exclusion.

Final selected native regression passes 8/8, including the earlier inherited
SQLite/save/return-type corpora and dynamic relation checks. Targeted ASan/UBSan
passes 3/3 for the new graph corpus, nested graph lifetimes and relation hydration,
with leak scanning disabled. Module integration passes 1/1; selected Java tests
pass 14/14 and all eight C++ artifact sets reproduce (88 files). Formatting checks
839 C++ files and source/path integrity checks pass. MIPS and physical-device
execution were not repeated.

Broader inherited configurations, remaining reverse graph shapes, constants/candidates,
remaining query/stream contracts and the full P0-P7 matrix remain open.

## Reverse collection and save extension

The same unchanged original crate now provides another **1,395 observations**,
varying normal/minimal/with-children School entries, unloaded/normal/minimal/
nested/filtered Enrollment selections, populated/empty owners, member update/
delete operations and root/member/second-member failures. The complete retained
corpus now contains **2,097** cases. Reverse outcomes comprise **45 reads**,
**270 successful saves**, **498 optimistic conflicts**, **450 Check failures**
and **132 Graph failures**. Six cases actually commit member changes, including
multi-member updates and soft deletions; unrelated root-only successes are not
counted as member commits.

`enrollmentListHandle()` preserves the original immutable RelationHandle view.
Unselected is NotLoaded; selected empty is Empty with a value list. The paired
observations include each member's native values, original snapshot and selected
inverse School view, including the explicitly nested inverse projection.

Members are copied into the same Context for modification. Changing a member
copy without adopting its state does not implicitly persist it through the owner.
Mutating or marking deletion before attachment adopts the pending changes and
original versions. The mutate-after-attachment path preserves the source failure
for missing loaded original versions rather than supplying versions artificially.
The generated recursive attachment visits stored forward objects, as source
does; it does not recursively adopt reverse collection members.

Fault injection on the second member tests whole-transaction rollback after an
earlier member can have been written. All failed reverse operations preserve all
six tables immediately before saving. Deletion keys and pending ledger fields
are also compared. The native application test verifies this failure, unchanged
owner/earlier-member values, successful retry and ledger clearing. It explicitly
copies a reverse graph into Global storage, deletes the actual UserContext and
checks expired member E borrows and surviving destination-owned list members.

This extension adds qualification and application tests over existing runtime
and generated adapters. It does not claim implicit collection persistence,
automatic reverse-member attachment or full inherited generator acceptance.

Final reverse-extension gates pass: native 2/2, targeted ASan/UBSan 2/2 with
leak scanning disabled, and module integration 1/1. The sanitizer run includes
all 2,097 source observations and actual reverse-member Context deletion/copy
checks. Formatting checks 839 C++ files; locked-source and ledger integrity pass.
Generated artifacts did not change in this extension, so earlier 88-file
reproduction evidence remains applicable. MIPS was not repeated.
