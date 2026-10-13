# Ordinary generated runtime assembly against Rust 5.0.6

Status: partial G12/P6 implementation and qualification. Runtime:
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`; producer:
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Assembly and initialization

C++ now emits the ordinary metadata-only `module(user_context)` alongside
`moduleWithCheckers`, `moduleWithBehaviors` and `moduleWithBehaviorsAndCheckers`.
All four register the generated schema bootstrap when the model has exactly one
ordinary Domain Root. Previously the bootstrap recipes and callback were emitted
only under constant qualification, leaving ordinary setup without its source
root preparation responsibility.

`configureRuntime(user_context, filename)` binds SQLite transport, dialect,
scoped persistent IDs and the distinct generated application executor without
creating schema. `setup` delegates to it and waits for `ensure_schema`, retaining
its C++ convenience contract. The bootstrap runs existing shared asynchronous
model/schema operations and audited saves; generated code remains field recipes.
A fresh root has fixed ID 1, an installed active-root binding and reserved ID
floor. Existing roots retain application-owned values; initialization does not
reconcile their scalar defaults as though they were constants.

The root recipe preserves source Text seed operands for Decimal/float/double
when the original generic setter receives a quoted string. It marks availability
through the existing indexed state without inventing a successful typed numeric
parse. This preserves the possibility of source-shaped typed-check failure.
The normal application update APIs remain strongly typed.

Generated Assist now describes the metadata-only factory and all three
checker/behavior factories, the separate configuration/schema operations,
automatic root preparation, preservation on repeated initialization and failure
propagation. It explicitly tells applications to reuse the prepared root.
These guidance changes passed all 27 generator regression tests and were
regenerated across the 18 model workspaces; broader model-aware Assist remains
subject to G12 qualification.

The constant-only no-root guard remains explicit. Multiple ordinary roots still
lack a unique source Domain Root and do not receive a generated bootstrap; this
checkpoint does not qualify those original configurations or remove earlier
inheritance/constant generation guards.

## Paired ordinary-module observations

The oracle executes unchanged original module factories, configures their
actual SQLite provider and distinct domain executor, then calls `ensure_schema`.
The C++ probe applies each generated module with the same real provider stack.
All **72** observations match without normalizing outcomes. The matrix covers
four factory modes, Checker and Behavior registration, descriptor availability,
Tenant/Note and inherited Platform roots, one/two/three initialization calls,
application edits between calls, active roots, full root rows and exact errors.

The original one-to-one relations fixture fails automatic bootstrap with a
SQLite foreign-key mismatch. C++ now reaches and preserves that failure through
ordinary generated assembly rather than omitting the root write. Its exact
error and empty persisted root rows remain part of the paired corpus.

## Fixture and integration maintenance

Controlled inheritance fixtures now update the automatically prepared root's
name/version instead of inserting a duplicate ID 1. Their frozen Rust drivers
already prepare that root through generated initialization. Sample/module
integration consumes the prepared active root rather than creating a second
unrelated Context root.

The optional-scalar and custom Checker-flow fixtures use an explicit manual
RuntimeModule/provider helper without generated bootstrap, matching their
unchanged Rust contexts built from metadata and custom registries. This helper
is test infrastructure and does not substitute for application initialization.
The generated ordinary assembly is exercised separately through its public
module and schema paths.

A broad native rebuild also exposed two independent integration failures:
the diagnostic provenance type collided with the existing templated core
ExpressionRoot, and a dynamic-list vector assignment required assignable entities.
The internal diagnostic type is now ExpressionRootProvenance. Dynamic decoding
uses the existing allocator-aware SmartList vector-move constructor, supporting
constructible handwritten entities while preserving their original example APIs.

Historical ported C++ fixtures also require explicit mutation intent and
query comment/purpose before constructing current requests. Their adapters now
use the immutable request intent API. These maintenance changes do not relabel
the frozen 5.0.5 Rust fixtures as 5.0.6 comparisons or establish new source
qualification. The unchanged original references remain the oracle authority.

The maintained SQLite entity-service test now covers explicit intent at direct,
prepared and schema-bootstrap entry points. Its real pagination case verifies
both purpose identities: a changed purpose performs offset execution and
registers a new cursor; the original purpose reuses its seek cursor. The source
normalization in `teaql-runtime/src/data_service/resolved.rs` clears comment and
trace, but retains purpose. Source cursor registration overwrites the transient
cache-miss observation with `OFFSET_FALLBACK:FIRST_PAGE` after a successful put.
The same-purpose cross-Context aggregation-cache fixture follows
`data_service/helpers.rs`, which also retains purpose while canonicalizing the
query. Structured mutation target IDs are checked independently of the canonical
request comment. No runtime fallback for missing intent was introduced.

## Remaining scope

Current checkpoint gates: generator regression **27/27**; regenerated artifacts
and reproduction check **18 workspaces / 216 files**; targeted native CTest
**6/6** (ordinary assembly, sample fields, sample execution, optional types,
Checker flow and runtime namespace); targeted ASan/UBSan **3/3** (ordinary
assembly, sample execution and optional types), with leak scanning disabled;
module imports **3/3** (generated Context root, sample fields and optional types).
Maintained native SQL/provider/entity-service regression passes **9/9**, with
historical/source qualification limits as described above.
These gates were rerun after the diagnostic provenance rename and dynamic-list
constructor fix. Native validation also corrected the manual fixture helper's
provider include, which the earlier module-only check could not detect.

Full root seed/configuration, temporal/JSON/Business-ID combinations, complete
model-aware runtime customization/Assist responsibilities and sample progress
logs remain open. Full capability/P7 acceptance is not established by these
ordinary-module observations. The broad native integration gate is recorded
only after its final build/test result is available.

The complete native build now exits successfully with `ninja: no work to do`
after repairing ten stale request-interface/helper-name compilation targets.
Full CTest is a separate gate and still exposes historical baseline rejection
and unported native/example contracts. It must not be described as green.

Further generated Checker, Business-ID, return-type and inheritance fixtures
consume the root installed by `setup` instead of creating/binding a second root.
The manual shared-provider inheritance fixture installs scoped SQLite IDs before
schema bootstrap and updates its existing root seed. Its stream lease,
actual Context deletion, expression expiration and explicit Global model-copy
checks continue to pass. Six corresponding generated native contracts pass
after the fixture repairs; this does not change the inheritance/default-generation
guard or qualify the original inherited Business-ID compiler failure.
