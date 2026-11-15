# AP02 standard-container entity projection diagnostics

The complete locked Rust generated libraries accept six entity-type
projections that the current C++ generated libraries reject. These are
reproduced C++ generation defects. No production repair or compatibility
exception is claimed by this diagnostic.

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Actual compiler results

The [eight-case diagnostic](parity/ap02_container_entity_types_diagnostic_5_0_6.json.gz)
compiles fresh original and current Java sources, generates complete domain
libraries, builds each original library with its unchanged derives and compiles
each complete C++ model facade. Successful Rust binaries print the generated
Item entity identity. Native C++ observation is syntax compilation, not SQL,
E evaluation or JSON execution.

| Encoded relation JSON type | Locked Rust | Current C++ |
| --- | --- | --- |
| `Self` | Accept | Accept |
| `<Vec<Self> as IntoIterator>::Item` | Accept | Reject |
| `<std::vec::Vec<Self> as std::iter::IntoIterator>::Item` | Accept | Reject |
| `<<Vec<Self> as IntoIterator>::IntoIter as Iterator>::Item` | Accept | Reject |
| `<Result<Self, ()> as IntoIterator>::Item` | Accept | Reject |
| `<Box<Self> as std::ops::Deref>::Target` | Accept | Reject |
| `<&Self as std::ops::Deref>::Target` | Accept | Reject |
| `<u64 as ToOwned>::Owned` | Reject: non-entity type | Reject |

The receipt SHA-256 is
`2ef5fc7935184aabcab5980a1bba1245c9ede2ae75afa3fcd929ad91a29c5157`.
It retains the exact diagnostic driver, model XML, generated file hashes,
compiler commands, dependency identities and terminal outcomes. Both reference
checkouts and all 11,977 captured source inputs remain unchanged. All eight
diagnostic cases completed; parity is explicitly false for the six named cases.

## Next implementation package

The [reviewable source patch](proposals/container_entity_type_projection.patch)
contains the exact corrected parser and six permanent regression methods.
It passes `git apply --check` against the unchanged production workspace.
Apply it only after the active frozen execution epoch terminates.

1. Extend the generator's existing entity-type projection resolver before its
   nominal module lookup. Preserve the difference between a container and its
   entity item or dereference target. This belongs to generation and introduces
   no runtime container, allocator or SQL behavior.
2. Add permanent Java regressions and actual generated-library compiler
   controls. Include previous associated and nominal type cases, unsupported
   projections, bare containers and prelude/module identity boundaries. A
   parser-only test does not establish generated compiler acceptance.
3. Reproduce affected SDKs. Qualify generated JSON/relationship consumers through
   execution or an exact, explicit unchanged-artifact reuse proof. Preserve
   Context ownership and the established runtime/service/provider boundaries.
4. Admit exact producer, manifest and validation-source transitions in ordinary
   and optimized Python. Keep the failed diagnostic immutable and distinguish
   copied candidates from installed production results.

The current native integration epoch was started before this diagnostic and
keeps its execution inputs frozen. Install production changes after that epoch
terminates. Its outcome must not be relabeled as validation of a later repair.

The separately pending Option-model and column-attribute decisions concern
reference defects. They do not prevent fixing these C++ defects and are not
approved implicitly here. Complete AP02/G14, G17 and P0-P7 remain open; capability
status stays **14 qualified / 3 partial**.

The owned probe removed 354,685,784 measured bytes and its 11,888-byte temporary
driver. Source, compact evidence and stable preset build outputs are retained.

## Copied-candidate lifetime correction

The first isolated resolver preserves a recursive type shape so containers,
items, pointer targets and intermediate iterator types remain distinct. Its
172 Java tests pass; substituting the current production parser fails exactly
four of the five new regression methods. Production source is unchanged.

Source review of the original derive's `borrowed_json` method found that the
injected relation type appears inside a method without named lifetime
parameters. The separate
[six-case lifetime diagnostic](parity/ap02_container_entity_types_lifetime_diagnostic_5_0_6.json.gz)
reproduces two false acceptances in that first candidate: references naming
undeclared `'a` and `'missing`. Elided, `'static` and `'_` references agree;
the mutable-reference `ToOwned` refusal also agrees. The original Rust compiler
rejects the two undeclared names while the copied C++ candidate accepts them.
This diagnostic is retained with parity explicitly false; the first candidate
is not approved for production installation.

The [corrected copied candidate](parity/ap02_container_entity_types_candidate_v2_5_0_6.json.gz)
checks explicit reference lifetimes before reducing a target type. All ten
complete original/candidate compiler cases match: five accept and five reject,
including nested pointers and mutable references. All 173 Java tests pass.
Rebuilding the first candidate with the current six regression methods fails
exactly the newly added lifetime method, demonstrating the correction.
The corrected parser SHA-256 is
`f3d346d1ffdf390392803e16934406c225c7c084e77e3e18f5ffe4afcba56009`.

All 11,977 captured source inputs and both reference checkouts remain
unchanged. The lifetime diagnostic removed 270,428,002 measured owned bytes
and its 358,776-byte driver; the corrected candidate removed 316,918,607 bytes
and its 488,190-byte driver. Compact receipts retain the exact copied Java,
test and driver sources, compiler commands, models and generated file hashes.

## Complete copied-candidate reconciliation

The [first candidate's complete compiler matrix](parity/ap02_container_entity_types_candidate_5_0_6.json.gz)
finishes all 46 original/C++ pairs: the preceding thirty cases, the eight-case
container diagnostic and eight new nested-composition controls. All 35 accepts
and eleven refusals agree. This finite result does not erase the separate
lifetime diagnostic's two false acceptances in that same first candidate.

The [corrected candidate's artifact replay](parity/ap02_container_entity_types_candidate_v2_reuse_5_0_6.json.gz)
then compiles fresh corrected Java and reproduces all 46 complete SDK trees
byte for byte. Its source differs from the first candidate only at the
qualified reference-lifetime guard. The preceding compiler outcomes are
**explicitly reused through that artifact identity**. This replay starts no
C++ compiler and makes no fresh forty-six-case compilation claim. Eight
missing/substituted artifact, outcome, model, source, reference and execution
scope controls reject in each Python mode.

The artifact replay ran from stdin. Its exact shell/Python controller is
retained in a [separate source receipt](parity/ap02_container_entity_types_candidate_v2_reuse_controller_5_0_6.json.gz);
the original replay record's null `driver_source` field is preserved. Its
executed validation helper is also embedded in the original record.

Together, the corrected candidate has ten actual current compiler outcomes
and forty-six explicitly reused outcomes, with 173 Java tests and the
preceding-candidate lifetime regression failure. This is copied-candidate
qualification, independent of production installation or full G14 acceptance.
`Result<T, ()>` is the reviewed Rust relation-metadata spelling; other error
types and additional standard type/trait families retain separate qualification
work. The runtime's Result and error propagation contracts retain their
existing implementations.

All five probe trees were removed after their children terminated, totaling
1,941,231,071 measured owned bytes, plus 1,253,401 bytes in four temporary
driver files. Six compact receipts retain 9,754,805 bytes of source, commands,
hashes and outcomes. Stable preset outputs and source remain present.

Production installation, eighty-SDK reproduction, generated-consumer artifact
identity and current source-admission controls remain required.
This bounded resolver does not establish arbitrary Rust type/trait evaluation
or complete AP02 grammar and consumer acceptance.

## Fresh copied-candidate SQLite consumers

The [fresh SQLite continuation](parity/ap02_container_entity_types_candidate_sql_repaired_5_0_6.json.gz)
executes six complete original/corrected-candidate generated model pairs and
matches all thirty query, E and JSON outcomes. The model inputs use vector
items for Self and Child, a vector iterator item, a unit-error Result item,
a boxed dereference target and a reference target. The Child selection
exercises the cross-entity target instead of reducing every projection to Self.

Each model executes unselected, loaded, empty, minimally projected and missing
parent queries. The observer compares the complete metadata and structured
runtime outputs, including actual SQLite statements. Fresh Java compilation
uses the exact copied parser and records all actual compiled C++-generator
Java sources. The parser, other sources, dependency JARs, SQLite archive,
runtime libraries and all 11,977 global source inputs remain unchanged.
Both locked reference checkouts remain clean. This is execution of the copied
candidate, with `installed=false` retained in its receipt.

The [first attempt](parity/ap02_container_entity_types_candidate_sql_5_0_6.json.gz)
stopped before Java compilation or business cases because `mvn` was absent
from its PATH. The corrected invocation uses the already installed Maven 3.9.8
directory. The first receipt remains incomplete with zero model cases; it is
not classified as a runtime failure or silently replaced by the successful run.
Both temporary trees were removed. The failed tree's size was not measured;
the successful run removed 531,213,740 measured owned bytes and a 20,755-byte
driver. Its native Rust linker is the pinned Clang driver, preserving the
previously qualified host SQLite linking boundary.

This supplies fresh JSON/SQLite consumer evidence before production
installation. Production source/artifact identity and the current admission
controls remain independent installation obligations. No new full native,
sanitizer, MIPS or physical-router verdict is established by these six models.

The [copied-candidate milestone](parity/ap02_container_entity_types_copied_milestone_5_0_6.json.gz)
binds all eight preceding receipts, preserves the two failed diagnostics and
the missing-Maven attempt, and verifies that the non-projection metadata
parser body is byte-identical. It records 2,472,444,811 measured owned bytes
removed, 1,274,156 measured successful outer-driver bytes removed and
12,278,884 bytes retained in the eight receipts. The failed SQL tree's removed
size remains explicitly unmeasured. Source, stable preset outputs and all
capability statuses are unchanged. The native epoch was still running when
this copied-candidate checkpoint was captured.

## Committed implementation on an isolated branch

The [isolated package verification](parity/ap02_container_entity_types_branch_package_5_0_6.json.gz)
applies the exact reviewed patch and commits implementation
`3187dad4bf3779ea361b250cb77f987296b6919b` on local branch
`cpp_container_projection_c70d8491`. Its tree changes only the parser and six
permanent regression methods. Actual offline Maven packaging succeeds, followed
by fresh compilation and all 173 Java tests. This is committed implementation
verification, with `installed_to_cpp_support=false` retained in the receipt.

The active `cpp_support` checkout remains at
`4a82e0517c6c889c1d9df55fee3e74a7ab685bf9` while the full native source epoch
runs. All 11,977 execution inputs and both locked references remain unchanged.
After checking clean tracked/untracked state, the absence of nested
repositories and retention of the commit on its branch, the owned worktree
was removed without force. Its measured source/build output size was
51,852,083 bytes. Package hashes and terminal commands are retained; disposable
JARs and classes are removed. Merge the retained commit into `cpp_support`
after the frozen epoch terminates, then qualify current production artifacts
and their exact source admission.

The [production installation recipe](proposals/verify_container_type_installation.patch)
is prepared as a separate, syntax-checked patch and passes `git apply --check`.
After installation it must package the current producer, run 173 fresh Java
tests, regenerate all 56 compiler selections, compare complete SDK bytes and
execute each production C++ compiler contract. Original Rust outcomes are
explicitly reused from the sealed matrices. Recipe execution remains pending;
syntax and patch checks are not production acceptance.
