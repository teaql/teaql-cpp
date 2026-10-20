# Inheritance variants: Rust 5.0.6

Current status: G04/P2 is qualified with documented language/ownership adaptations;
normal inheritance generation is enabled. See [the final acceptance](inheritance_acceptance_5_0_6.md).
The checkpoint status and guard statements below are retained historical evidence.

Status: partial G01/G03/G04/G13 evidence. The branched scalar/clock fixture is
source-qualified within its observed contracts. Inherited Business-ID generation
has C++ integration evidence and an original-producer compilation defect; it is
not source-qualified. The default inheritance guard remains.

Runtime: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer: `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Branched configuration

`inheritance_variants.xml` declares Document, a fieldless IntermediateRecord,
Invoice, sibling Receipt and third-level SpecialInvoice. Receipt compatibly
redeclares name. All descendants inherit the Tenant Context root, boolean active,
optional note and create/update-time fields. The original complete generated
crate compiles unchanged; its 43 files plus provenance reproduce.

**375 matching original/C++ observations** cover 120 constructions, 120 SQLite
reads and 135 audited saves. Construction observes ordered projections and
recursive descendant trees, including sibling ordering and the empty intermediate.
Typed loading preserves source overwrite order: the later sibling's common
fields win, while earlier descendant extras remain in the ancestor's dynamic
record. This is source behavior, not a C++ language inheritance/casting rule.

The save matrix includes **21 commits**, **48 optimistic conflicts**, **45 Check
failures** and **21 Graph failures**. It compares inherited false/NULL values,
required/length/sparse checks, timestamp fixes, ordered fix field paths, all five
physical model tables and pending ledger fields. Failed database states remain
unchanged immediately before saving, including separately injected conflicts.
Source/result identities remain separate from these same-model scenarios.

Volatile clock epochs are deliberately projected as whether created_at changed
from 111 and updated_at from 222. Persistent non-clock values and complete Check
diagnostics remain observed; conflict and injected-trigger markers are explicit.
The corpus is not evidence of equal clock epochs between processes or identical
operational timings. Original values are not normalized to force comparisons.

The native application test checks recursive structure, later-sibling precedence,
descendant dynamic retention, explicit Global copying, expired E access after
actual UserContext deletion and destination allocator ownership with default PMR
disabled.

## Inherited Business-ID defect and C++ repair

The separate `inheritance_business_ids.xml` adds a Business-ID field to that
ancestor. The frozen producer emits descendant getters referencing DocumentNumber
without defining/importing it, and descendant checkers reference a missing
DocumentDocumentNumberBusinessIdSlot. Complete original compilation produces
**20 E0425/E0433 errors**. The untouched 44-file fixture, dedicated Cargo compile
reproducer and
[structured failure record](parity/inherited_business_source_failure_5_0_6.json)
preserve that evidence. Run:

```sh
python3 scripts/verify_inherited_business_source_failure.py --check
```

The invalid original crate is isolated from the shared oracle Cargo dependencies.
It is not patched, and its failure is not interpreted as an application-level
runtime rejection. This remains a source qualification gap requiring an explicit
source-contract disposition before full inheritance acceptance.

The C++ generator previously reported a type-name collision because it processed
the same inherited FieldDescriptor repeatedly. Validation and type emission now
track descriptor identity: one inherited field gets one shared type definition;
distinct field/type collisions retain their existing rejection. A Java regression
asserts one definition and inherited typed accessors. C++ integration creates an
Invoice with the inherited number through SQLite Business-ID infrastructure,
Tenant fixes and common graph clock fixes. That successful C++ save is integration
evidence, not a paired original inherited Business-ID observation.

There are now ten C++ fixture sets (110 generated files) and two additional
original 44-file fixture sets. Earlier artifact counts remain historical. Other
P0-P7 capabilities and the remaining parent-input/stream gates stay open.

Selected native source/integration checks pass 2/2; the final selection with
existing Business-ID source/integration regressions passes 4/4. Targeted sanitizer lifetime
and C++ Business-ID integration passes 1/1 with leak scanning disabled; module
integration passes 1/1. Selected Java regressions pass 15/15, including the new
single-type emission control. All ten C++ artifact sets reproduce 110 files, and
both new original fixture sets reproduce 44 files each. Formatting checks 855
C++ files; active source/path integrity passes. MIPS was not repeated.
