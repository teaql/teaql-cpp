# Compiler scalar mapping and consumer matrix

This work follows the [current completion sequence](current_completion_sequence_5_0_6.md)
and the full P0-P7 goal. The reference remains Rust 5.0.6 at the repository
locks. G13, G14 and G17 remain partial; fourteen capabilities are qualified.

## Source responsibilities

`CppCompilerScalarStorage` currently maps ten canonical Rust scalar identities.
The v4 worker compares actual getter TypeId values in the original compiled
library, then reports semantic identity and primitive storage separately.
`CppCompilerModelConfiguration` checks the complete descriptor/getter inventory
before C++ generation. Each optional mapping must preserve the original
optionality and use the corresponding existing C++ optional carrier.

| Canonical Rust storage | C++ carrier | Existing compiler observation before this continuation |
| --- | --- | --- |
| `u64` | `std::uint64_t` | Required structural IDs and optional foreign keys observed. A configured ordinary scalar has a separately retained original whole-library refusal. |
| `i64` | `std::int64_t` | Required v4 and optional v3 observations. |
| `f32` | `float` | Required and optional v3 observations. |
| `f64` | `double` | Required and optional v3 observations. |
| `bool` | `bool` | Required v4 and optional v3 observations. |
| `alloc::string::String` | `String` | Required v4 and optional v3 observations. |
| `rust_decimal::decimal::Decimal` | `Decimal` | Required v4 observation; the selected retained jobs have no optional witness. |
| `serde_json::value::Value` | `Json` | Optional v3 observation and the approved JSON metadata exception. The required case needs its own complete-library/consumer disposition. |
| `chrono::naive::date::NaiveDate` | `Date` | Required and optional v3 observations. |
| `teaql_core::time::Timestamp` | `Timestamp` | Required and optional v3 observations. |

These observations come from the retained
[scalar execution](parity/compiler_scalar_storage_typeid_5_0_6.json.gz),
[JSON consumers](parity/compiler_json_consumers_current_5_0_6.json.gz),
[host assembly](parity/compiler_host_assembly_5_0_6.json.gz) and
[governed host](parity/compiler_host_governed_complete_5_0_6.json.gz).
A library containing a type does not establish every field's consumer result.
Required IDs, optional foreign keys and configured ordinary scalar fields keep
their distinct roles. Earlier protocols retain their original execution scope.
The [initial finite inventory](parity/compiler_scalar_mapping_inventory_5_0_6.json.gz)
binds all four preceding receipts and the exact canonical mapping source. Eighteen
of its twenty required/optional rows have compiler witnesses; optional Decimal
and required JSON were absent from those selected jobs.

The [finite inventory](parity/compiler_scalar_mapping_inventory_5_0_6.json.gz)
binds the exact producer mapping source and four unchanged committed execution
records. Eighteen of the twenty required/optional rows have a compiler witness
in those selected jobs. Required JSON and optional Decimal have none in that
selection. The two Business ID witnesses remain separate. The inventory makes
no new semantic acceptance claim.

Strong Business IDs form a separate semantic row. The v4 host preserves the
distinct ReceiptNumber/TicketNumber identities while mapping their primitive
storage to String. Its actual Checker, E, query/SQLite/JSON and allocation/Save
consumers are retained in the
[Business ID checkpoint](compiler_business_id_storage_5_0_6.md) and host
assembly. This does not establish arbitrary custom wrapper mappings.

## Temporal and JSON continuation

The [collector](../scripts/verify_compiler_host_temporal_json_5_0_6.py) sends
selected models through ordinary Spring controller assembly: required and
optional Date, Timestamp and JSON, plus optional Decimal. Each job retains its
full original compiler outcome.
Successful jobs reproduce the entire original SDK from the controller's exact
source-envelope bytes and compile the unchanged original/native scalar
consumers against the new libraries. Original failures publish no C++ SDK.

The existing native consumer asserts the exact getter carrier at compilation.
Both consumers exercise real SQLite rows, E evaluation, JSON output, empty
selection and query intent/SQL logs. Optional temporal cases include SQL NULL.
JSON has separate NULL, non-null and empty inputs so the approved original
metadata defect can be reported independently from equal results.

The native consumer disables the default PMR resource, closes each UserContext
and checks released user bytes. The worker and consumer builds have owned
temporary outputs with exit cleanup. The complete original SDK and every
source/reference/native input remain bound to the recorded execution.

The [initial execution](parity/compiler_host_temporal_json_5_0_6.json.gz) completes
all four temporal SDK pairs and their eight shared datasets. The values, E reads,
JSON and query logs match. The record remains incomplete overall: its next native
build fails a probe's getter-type assertion. The case was labelled required JSON,
but its model uses `jsonMe()`, which inherits `optional=true` from the original
built-in `text` type. Both the original compiler witness and C++ actually produce
optional JSON. This is a probe assumption error, without a production type mismatch.

The active required JSON model now uses the existing supported
`jsonMe(optional=false)` configuration. The source contract is visible both in
the locked `model/src/main/resources/models/infra/system-types.xml` and the
retained `tests/generator/checker_clock_composition.xml` fixture. The explicit
optional model retains `jsonMe()?`.

The initial receipt and its collector source are immutable. The current reader
binds its exact bytes and admits the completed temporal prefix only. Removing
the new case-selection operations recovers the entire previous execution body;
temporal inputs and commands stay identical. All input-frame members stay
unchanged except that exact collector transition. The failed later JSON probe
remains part of the receipt. No temporal Rust/native execution is repeated.

The [selected continuation](parity/compiler_host_temporal_json_continuation_5_0_6.json.gz)
reaches the explicit required JSON model. Complete original/C++ generation and
compilation succeed, and the NULL query matches. Its non-null query then fails
in both runtimes: the generic fixture declares `value` without an SQL type, so
both providers return Text. The actual Rust exit 101 and native exit -6 retain
their complete diagnostics and cleanup. This is a failed fixture boundary, with
no successful JSON result or new compatibility exception inferred.

The active fixture now declares JSON and DECIMAL columns for those two kinds.
The declared SQL type determines provider decoding. This changes the new fixture
inputs; no runtime decoder, model source or generated artifact is patched.
Removing only that guarded declaration restores the original temporal dataset
body. The [source images](parity/compiler_host_temporal_json_source_continuation_5_0_6.json.gz)
preserve both preceding collectors and the active replacement.

The [typed-storage continuation](parity/compiler_host_temporal_json_typed_storage_5_0_6.json.gz)
collects the explicit required/optional JSON models and optional Decimal with
those declarations. All three full original/C++ SDK pairs compile and every
shared dataset matches. A manually declared JSON column does not prove the
original schema provider creates the right type from its LargeText metadata.
The approved metadata/input exception retains that separate responsibility.

## Qualified finite result

The [completed matrix](parity/compiler_scalar_mapping_matrix_5_0_6.json.gz)
combines the admitted temporal prefix with the new typed-storage execution:

| Model | Complete original/C++ SDK files | Shared outputs | Typed rows |
| --- | --- | ---: | ---: |
| Required Date | 19 / 39 | 2 | 3 |
| Optional Date | 19 / 39 | 2 | 4 |
| Required Timestamp | 19 / 39 | 2 | 3 |
| Optional Timestamp | 19 / 39 | 2 | 4 |
| Explicit required JSON | 19 / 39 | 3 | 3 |
| Explicit optional JSON | 19 / 39 | 3 | 4 |
| Optional Decimal | 19 / 39 | 2 | 3 |
| Total | Seven complete SDK pairs | 16 | 24 |

These seven original libraries provide 49 actual scalar facts. Every one of
the finite twenty carrier/optional inventory rows now has a compiler witness,
with structural ID/foreign-key roles and earlier protocol scopes retained.
All sixteen complete query/getter/E/JSON/SQL outputs match. Dates include leap
days and a pre-epoch day; timestamps include zero, -1 and millisecond precision;
JSON includes nested objects, strings and NULL; Decimal includes positive,
negative and NULL values. Empty selections are retained throughout.

The [reader gate](parity/compiler_host_temporal_json_validation_5_0_6.json.gz)
passes four ordinary/optimized commands, admitting the exact temporal prefix
and new typed-storage execution. Both Python modes reject fifteen corruptions,
including missing compiler/preparation/model/dataset/SDK members, wrong types
or values, substituted command/input/output, changed observers and Boolean exit
codes. This verification does not repeat the completed business execution.

The first two failed collections retain their source, native diagnostics and
cleanup. The matrix qualifies the named carrier inventory and read consumers.
Remaining Checker/Fix, sample, schema and governed Save combinations retain
their independent exits.

## Development checks and retained outputs

The [first development check](parity/compiler_scalar_mapping_checks_5_0_6.json.gz)
passes formatter write/check for 1,818 handwritten C++ files, locked upstream,
module paths and the generator ledger. Its unrestricted native build then
selects 944 stale tasks. Only its verified owned Ninja child is interrupted;
the observed build exit is 130, the parent records failure and its temporary
directory is removed. The [ownership record](parity/compiler_scalar_mapping_build_scope_5_0_6.json.gz)
identifies that process. The goal remains active and other sessions are untouched.
This attempt is not a passed native gate.

The [focused continuation](parity/compiler_scalar_mapping_focused_checks_5_0_6.json.gz)
builds the three related probes and their fixed-clock dependency. All six actual
CTest entries pass: scalar comparison, dynamic JSON scalars and generated sample
fields, plus their three required source fixtures. Registry/JUnit membership is
exact, with no failures, errors or skips. The final build of those four targets
has no work. The five completed development checks are explicitly reused through
the unchanged typed-execution input frame; all runtime archive/compiler/SQLite
and reference identities remain unchanged.

The [compact milestone](parity/compiler_scalar_mapping_milestone_5_0_6.json.gz)
verifies 23 absent disposable paths and 3,319,058,939 measured bytes removed.
The twelve preceding evidence records occupy 6,680,010 bytes. All eighty retained
SDKs preserve 3,560 code/Assist files; this is a hash check, not regeneration.
Eight completed objects listed in the interrupted build remain in the existing
native preset cache, totaling 300,467,936 current bytes. That is a measured cache
size, not a growth claim. Shared cache eviction is skipped; no extra build
configuration, dependency checkout or deliverable copy is retained.

## Remaining exits

1. Assign the remaining per-field consumer obligations to the finite carrier
   matrix. Compiler presence alone does not establish Checker/Fix, sample,
   schema or persistence behavior.
2. Reconcile preceding String, Boolean, i64/u64, f32/f64, Decimal and Business ID
   consumer evidence through explicit source/artifact identities. Keep actual
   missing representation or runtime behavior as implementation work.
3. Join every applicable mapping to generated query/E, Checker/Fix, sample and
   governed Save consumers, including metadata, Policy and audit/publication.
4. Reconcile remaining SQ/GP requirements and run final P7 integration on the
   final source state. The pending Option-model/column-attribute decisions and
   the three approved exceptions retain their existing dispositions.

This continuation adds no C++ runtime dependency or ownership domain. Complete
configuration mapping, G13/G14/G17 and the full P0-P7 goal stay open.

## Reproduce the recorded reader checks

Use the locked tools and the retained records at this source checkpoint:

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_compiler_host_temporal_json_5_0_6.py --check --record docs/parity/compiler_host_temporal_json_5_0_6.json.gz
python3 scripts/verify_compiler_host_temporal_json_5_0_6.py --check --record docs/parity/compiler_host_temporal_json_typed_storage_5_0_6.json.gz
python3 -O scripts/verify_compiler_host_temporal_json_5_0_6.py --check --record docs/parity/compiler_host_temporal_json_typed_storage_5_0_6.json.gz
```

The first command admits the completed temporal prefix and preserves the failed
later probe. Future source changes need their explicit continuation evidence.
Fresh business collection uses `--execute`, repeated `--case` selections and a
new `--record` path; existing execution and failure records must not be replaced.
