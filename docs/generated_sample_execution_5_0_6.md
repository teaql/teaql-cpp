# Generated sample execution against Rust 5.0.6

Status: partial G12/P6 implementation. Complete model configurations and the final
semantic matrix remain open.

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Producer reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
The source is the unchanged `rust-lib-core/sample_data/index.stg` and retained
relations/inheritance generated crates, verified by their provenance hashes.

## Runtime and generated interface

`generateSampleData(user_context, plan)` returns a lazy Context-owned operation.
The shared runtime copies step descriptors, then runs model recipes sequentially.
Roots load first, followed by constants and business models; each class retains
source depth/name ordering. Reference query errors default to an empty pool.
Required dependency checks use the template's centralized `required` naming
property (absence of `optional`), rather than the unrelated explicit `required`
attribute getter. Missing dependencies produce the exact source skip record.

Generated SQLite runtime setup now installs the existing scoped SQLite ID-space
generator for ordinary models as well as constant configurations. The initial
comparison exposed the previously used Global counter (IDs starting at 1001)
in place of the source per-model SQLite spaces (IDs starting at 1). The explicit
test runtime configures the same provider service, without ID normalization.

The existing fanout and reference helpers remain authoritative. Recipes create
through the bound Q facade and save through `AuditedSaveExt` with `Init Sample
Data`. Counts change only after successful saves. Generated references are added
only for models with reverse relations. Each call adds another business batch.
The recipe friend permits internal Context-root assignments without exposing
those setters to application callers.

`GeneratedSampleValues` supplies numeric, text and wall-clock defaults to thin
recipes, including modules consumers. Numeric filling retains the source I64
ledger operand even when the generated member has a Decimal or floating type.
Temporal values use the wall clock; seed alone is not temporal determinism.

Managed state borrows reject stack states and detect explicit state destruction
while work is pending. Sequence state belongs to UserContext. Wake propagation
and cancellation compose existing operations; child work quiesces before arena
reclamation. A Global report copy must be explicit.

## Source observations and scoped integration

The frozen original entry provides 18 SQLite observations over 26 invocations,
three scales, seeds zero/seven, present/missing roots and repeated calls. They
produce 6,880 persisted business rows. The corpus compares full reports, skipped
reasons, ordered IDs, names, foreign keys and versions without normalization.
Both probes use the same explicit test tables; this is sample execution evidence,
not successful automatic schema qualification.

The C++ probe explicitly allows 1 GiB for its test UserContext and aggregate live
User storage. Production defaults remain 64 MiB. Medium batches and repeated
Small batches retain enough Context-owned operation state to exceed the default;
callers must choose a suitable plan and allocation budget. This checkpoint does
not claim router performance or reduced per-save retention.

The native integration exercises two real audited Tiny batches, typed readback,
null default PMR, and an explicit Global report copy after actual User deletion.
Deferred reference-query and sequence tests exercise pending/wake completion,
failed-load fallback, explicit cancellation, actual Context deletion, unstarted
later steps, stack descriptor copying, stack-state rejection, and state destruction
while a child query is pending.

## Remaining differences and qualification

The original relations automatic bootstrap fails with a SQLite foreign-key
mismatch. The original inherited sample entry succeeds with 20 rows each for
School, Academy and ResearchAcademy. The initial C++ trial omitted ancestor
fields because the implementation workspace's older shared naming helper used
own fields. Recipes now use the existing C++ adapter of the locked producer's
complete predefined fields. Native real audited save/readback passes for all
three inherited business models, including ancestor text/root/integer fields and
the source I64 numeric operand behind Decimal storage. Full inherited row/trace
and configuration qualification remains open. Source observations are retained
in `parity/generated_sample_source_observations_5_0_6.json`; the original succeeds
and must not be described as a source failure or scope exclusion.

Further gates remain for scalar/numeric/temporal/JSON/optional configurations,
constant/candidate/inherited reference topology, further save failure/async
adapters, sample progress log behavior and complete model-aware
Assist/extension instructions. Keep G12 and P6 partial until these are qualified.

## Completed checkpoint gates

Native integration/state/related generated API, Checker and Business-ID contracts:
6/6. Targeted ASan/UBSan: 2/2, with leak scanning disabled. Module lookup and
Context-root generated integration: 2/2. Java CppGenerationTest: 26/26. Compiler
contracts: one positive sample call and two invalid argument rejections. All 15
C++ artifact sets / 180 files reproduce. The pinned formatter checks 951 C++
files. Source locks, capability ledger, module exports and path checks pass.
The full 18-case replay passes against live original calls, without output normalization.

The inherited field follow-up also passes the native execution contract and the
expanded ASan/UBSan execution contract. Its generation assertion requires parent
text/numeric/object defaults in ResearchAcademy recipes. Complete inherited
source row/trace comparisons remain a distinct pending qualification.

The later [failure/pending-save checkpoint](generated_sample_failures_5_0_6.md)
adds 28 original observations and real delegated saver suspension/cleanup tests.
It leaves broader configuration and complete G12 qualification open.
