# Generated Checker and behavior assembly for Rust 5.0.6

Status: supported-model implementation and focused native/lifetime/module/
reproduction gates passed. Complete G02/G03/G12/G15 and P1
remain open. Sources are the frozen producer's checker/index.stg,
behavior/index.stg and runtime/index.stg, plus runtime 5.0.6.

## Generated responsibilities

Each supported entity emits CheckerLogic, NoopChecker, a TypedChecker wrapper and
an EntityDataServiceBehavior extension class. Logic callbacks and convenience
validation methods use the application camelCase naming convention. Behavior SPI
hooks and framework checker protocols retain runtime snake_case names.

GeneratedCheckerLogic implements required/optional/text and Unicode scalar string
length checks. GeneratedFieldChecks implements predefined loaded-update rejection,
trusted Context-root assignment/validation, create/update clock fixes and evidence.
Wrappers run generated fixes, then every predefined field's update-load check, then
business logic, preserving the source order. updateTime declares both create and
update functions; on create both run and both append evidence, as the source does.

Logic objects must be managed in an explicit Context. Wrappers borrow their
registered owner through ResourceBorrow, including their concrete registered slot
when logic is accessed through a polymorphic base. No singleton or implicit shared
owner is introduced. Both direct wrapper and generic typed adapter callbacks use
runtime quiescence; cleanup cannot free a callback's referenced Context storage.

moduleWithCheckers, moduleWithBehaviors and moduleWithBehaviorsAndCheckers construct
the existing RuntimeModule with model descriptors, graph decoders and the selected
registrations. setup now applies the combined module before SQLite execution.
Custom logic lives outside regenerated files and is registered through the same
typed adapter. Assist explains these extension points and the source rule order.

## Integration corrections needed by real registration

Graph decoder registration publishes relation payloads in the immutable runtime
graph. Generated getters now resolve those views. Const list access borrows the
frozen view; mutable access explicitly copies it into the model's own Context.
Forward object access honors explicit local values, relation views, named load
availability and identity graph lookup. It performs no implicit SQL.

ColumnLayout now preserves separate query-only relation names. RelationShapeCache
is bounded to one explicit hydration pass and shares transitions from an existing
shape without retaining values. CompactRow loaded-state projection includes these
names; foreign shape copies and column insertion/removal preserve them. Shape
interning distinguishes identical SQL columns with different relation availability.
Embedded graph flattening retains CompactRow metadata instead of converting every
row back through a plain map. Compact flat hydration carries mutable result shapes
through descendant plans so selected nested relations remain visibly loaded.

Generated AuditedSaveExt entries route through the ledger save and its existing
graph fix scope. The later request-intent/trace checkpoint corrected the earlier
blanket scope for ordinary handwritten entities, whose plain source entry preserves
its separate preflight responsibility. See
[request_intent_trace_5_0_6.md](request_intent_trace_5_0_6.md). Nested adapters for
the same User operation borrow the active scope when no
explicit time override is requested. They retain one operation clock and evidence
collection; independent operations still own distinct Context-managed states.

## Evidence and remaining scope

A third generated model fixture contains Tenant and Note with a Context-root field,
createTime and updateTime. It exercises custom logic, root mismatch, sparse update
rejection, clock ordering and exact fix evidence multiplicity. A real audited SQLite
save assigns the trusted root and common timestamps and retains all four create
fix records. Existing graph/query examples run with generated registrations.

Final focused native passes 7/7: generated API and Checker contracts, runnable
example, core snapshot contracts/source corpus, graph-fix and checker Context
contracts. Sixteen additional direct Rust row scenarios observe named relation
shape reuse and indexed availability across narrow/wide layouts. The snapshot/row
corpus now has 427 inputs. The source oracle remains dev-only and locked; C++
production dependencies remain unchanged. All three generated fixture sets now
contain eleven files, including checkers.hpp and provenance with the metadata
adapter's implementation hash.

This checkpoint does not remove inheritance/constant/business-ID guards or waive
their generated checker responsibilities. Complete source-based Checker/behavior
matrix, user extension qualification, readonly/durable carriers, guarded model
configurations and full governance/provenance integration remain pending. Native
fixtures and source-correlated mechanics do not by themselves close those ledgers.

Final targeted ASan/UBSan passes 6/6 with leak scanning disabled. Named-module
import passes 1/1; all 1,318 catalog exports and seven canonical facade archives
verify. Selected Java regressions pass 9/9. All three eleven-file fixture sets
reproduce, and one positive/nine negative compiler contracts pass. Existing native
relation_hydration_contracts additionally passes 1/1 after the metadata mutation
protocol change. P1 is still active; these checks do not qualify every source
model/configuration or close any top-level capability.

## Direct generated-source helpers and entity annotations

The source oracle now compiles the release's unmodified retained narrow generated
library, with its TEAQL dependencies patched to the frozen 5.0.6 worktree. It calls
SchoolCheckerLogic helpers directly. The C++ observer calls GeneratedCheckerLogic;
220 inputs compare complete diagnostic objects in camel, snake and Pascal naming
profiles. Cases include optional absence, required booleans, Unicode whitespace,
Unicode scalar lengths, combining marks and long strings. JSON-lines parsing
splits on LF only so literal Unicode line separators remain inside JSON strings.

Another 54 direct EntityRuntimeState input sequences distinguish per-entity
annotations from the root request reason. Generated model setComment/getComment
now delegate to the entity-keyed annotation map. Root set_comment/get_comment
remain the separate request-level API. On composition, source entity annotations
overwrite matching keys, different entity types with the same ID remain separate,
and the receiving root reason stays unchanged, including when absent or empty.
clear_committed clears both annotations and the root reason. Runtime composition
also preserves the receiving complete trace-chain entry instead of concatenating
complete chains, following the frozen source. That latter change has source-review
evidence; these 54 public calls do not establish complete trace-lineage parity.

The combined 274-input corpus is checker_provenance_5_0_6_cases.json. It records
the runtime commit and hashes of the generated helper and entity runtime sources.
Native contracts additionally copy long annotations from User Context into
Global storage, destroy the original User Context, then read the surviving copy.
These cases qualify a helper/composition subset; full G02/G17 governance and
provenance remain open.

This additional checkpoint passes focused native 4/4 and ASan/UBSan 4/4 with
leak scanning disabled. Module import passes 1/1; the 1,318 exported names and
seven facade archives verify. Selected Java regressions pass 9/9 and each of the
three eleven-file generated fixture sets reproduces. No MIPS or physical-device
run was needed for this source/helper change. One positive and nine negative
generated API compiler contracts also pass.
