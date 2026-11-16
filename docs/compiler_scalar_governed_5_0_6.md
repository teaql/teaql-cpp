# Generated scalar schema and governed persistence

This continues the [finite scalar matrix](compiler_scalar_mapping_matrix_5_0_6.md)
under the full [P0-P7 goal](cpp_generator_parity_plan.md). Rust runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a` remain the reference. G13, G14 and
G17 stay partial until their complete exits are satisfied.

## Plan and required evidence

1. Reproduce the complete original/C++ SDKs for the seven qualified scalar
   models from their exact source envelopes and compiler documents. Require
   equality with every retained SDK file hash. Explicitly reuse the completed
   type/descriptor compiler jobs; compile the new business observers separately.
   No reference or generated source is patched.
2. Start with an empty SQLite database. Run the registered schema provider and
   generated bootstrap twice. Observe actual PRAGMA column/foreign-key facts,
   generated root rows and repeat-run state. Preserve the approved original
   JSON LargeText metadata discrepancy separately from ordinary parity.
3. Execute typed creation and update, then a mixed Gateway/Reading graph Save.
   Observe Context-based Fix evidence, actual IDs, whole immutable Policy
   operation plans, safe audit values and their canonical trace chains, SQL
   intent/parameters and authoritative stored values/versions.
4. Exercise Policy denial and exact approval, root/child write failures,
   root/child optimistic conflicts and audit delivery failure after commit.
   Retain original JSON failures as explicit outcomes where the approved
   metadata defect reaches persistence or subsequent authoritative loading.
   Ordinary CRUD does not substitute for the two-object graph cases.
5. Compare complete shared observations, qualify active readers and corruption
   controls under ordinary and optimized Python, run affected native targets
   and preserve source/artifact continuity. Update the capability ledger without
   promoting an incomplete capability. Remove owned temporary outputs after
   their children exit; use only the stable affected native build targets.

The [collector](../scripts/verify_compiler_scalar_governed_5_0_6.py) consumes
committed compiler receipts. Fresh generation over those exact documents must
produce the entire original nineteen-file SDK and C++ thirty-nine-file SDK
again. This is source/artifact identity reuse, rather than another worker
compiler execution or another run of the preceding read consumers.

The [native observer](../tests/parity/compiler_scalar_governed_probe.cpp) uses
Context-bound Q and generated model setters, the registered schema provider,
generated audited Save and the shared graph runtime. It disables default PMR,
owns SQLite and callback resources through UserContext, completes each Save
before close and checks released user bytes. The
[original observer](../tests/parity/compiler_scalar_governed_probe.rs) uses the
same generated operations against the locked source runtime. Policies and sinks
are trusted fixture capabilities; their installation does not model application
authorization or permit request data to replace trusted capabilities.

## Initial observer compilation

The [first Date attempt](parity/compiler_scalar_governed_date_initial_5_0_6.json.gz)
reproduces both complete SDKs. Its new Rust observer fails because the Entity
and LedgerEntity traits were not imported. The reference SDK itself compiles;
the five E0599 messages belong to the fixture. Its source, compiler output and
cleanup remain retained. No schema, Save or native result is inferred from it.

The corrected observer imports the two existing traits. The
[trait continuation](parity/compiler_scalar_governed_date_traits_5_0_6.json.gz)
compiles the original and native observers. Its original execution then refuses
the fixture's ID generator: bootstrap must reserve fixed ID floor 1, while the
test capability implemented generation only. That initial execution produces no
business observation. Its exit 101 and cleanup remain retained; no native result
is inferred from the unexecuted native binary.

The fixture now implements the actual reservation contract in both languages.
Counters use the shared canonical ID-space names, reserve at least the requested
floor and generate increasing IDs above it. The C++ counters and strings belong
to UserContext. This changes the supplied fixture capability, not the original
or C++ runtime ID abstractions.

The [reserved-floor Date execution](parity/compiler_scalar_governed_date_floor_5_0_6.json.gz)
completes ten original/native observations. Every field matches, including
schema/foreign-key facts, both bootstrap snapshots, all Fix evidence, complete
Policy operations, safe event fields/trace chains, stored values/versions and
business SQL logs. Creation, update, mixed graph changes, denial, exact approval,
both write-failure positions, both version conflicts and post-commit audit
delivery failure retain their actual outcomes.

The [remaining execution](parity/compiler_scalar_governed_remaining_5_0_6.json.gz)
first exposes another observer argument error. Locked optional non-Timestamp
setters take `impl Into<Value>`; passing `Option<NaiveDate>` directly does not
compile. This is a fixture error, rather than a failed runtime operation.

The [argument continuation](parity/compiler_scalar_governed_optional_arguments_5_0_6.json.gz)
adapts exactly the three observer setter arguments for optional Date, Decimal
and JSON: `None` becomes `Value::Null`, and a present value uses `Value::from`.
The generated libraries and both stored observer templates remain unchanged.
Each adapted observer source is retained. Timestamp keeps its actual dedicated
`Option<Timestamp>` signature. The continuation also adds explicit NULL
creation, scalar clearing and graph clearing for every optional model.

## Actual business results

All seven whole SDK pairs reproduce exactly: nineteen original and thirty-nine
C++ files per pair. These runs reuse the preceding compiler type/descriptor
jobs explicitly, then freshly compile and execute the new Rust/native business
observers. No reference source or generated SDK file is changed.
The collector's row flag `original_execution_reused` refers to that preceding
compiler job. Separate successful `original_build` and `original_run` commands
are required for every new business observer; the flag does not substitute for
those executions.

| Model | Complete matching observations | Separate JSON exception observations |
| --- | ---: | ---: |
| Required Date | 10 | 0 |
| Optional Date | 13 | 0 |
| Required Timestamp | 10 | 0 |
| Optional Timestamp | 13 | 0 |
| Optional Decimal | 13 | 0 |
| Required JSON | 0 | 10 |
| Optional JSON | 0 | 13 |
| Total | 59 | 23 |

The fifty-nine complete ordinary outputs match every shared field, including
actual SQLite column/FK metadata, two generated bootstrap snapshots, Context
Fix evidence, IDs, complete immutable Policy operations, safe audit field values
and full trace chains, actual stored values/versions and business SQL intent/
parameters. NULL creation and clearing retain actual NULL rather than an omitted
field. Root and child write failures leave both writes rolled back; optimistic
conflicts retain the adversarial version; sink delivery failure leaves the
committed graph and returns a committed-operation error.

The twenty-three JSON observations remain the previously approved JSON metadata
compatibility exception. Original generated metadata calls the property
`LargeText`, producing SQLite `TEXT`; C++ uses `JSON`. Original non-NULL creation
reports an invalid `Text` value after the row has already been stored. A seeded
non-NULL row likewise remains stored, but its authoritative typed reload fails;
the original update/graph cases stop at that load. These are actual error/state
observations, not Policy, rollback or conflict acceptance for the original JSON
path. In particular, an error here does not prove that the transaction rolled
back. Optional NULL creation succeeds in both runtimes, with the schema
declaration discrepancy still retained.

C++ completes the JSON scalar/graph operations, denial, exact approval, both
rollback positions, both version conflicts, post-commit audit failure and
optional NULL clearing. The reader checks these results explicitly and binds
the complete original/native exception observations. It neither suppresses
arbitrary JSON differences nor counts these twenty-three observations as
matching Rust parity.

## Reader and source continuation

The collector now has separate `--execute` and `--check` entries. Completed
business execution is reused through exact source images; adding the reader
does not repeat the compilation or database runs. The previous temporal reader
retains its exact command, input, execution and fifteen corruption-control
bodies. Only its source admission is extended to the named fixture/reader
additions; unknown additions and unqualified hashes reject.

The initial reader attempts retain their failures. The first source reader
misread the nested `initial`/`untyped` temporal transition record. The collector
reader then imported `tomllib`, unavailable in the pinned Python 3.10, and its
first input-byte check re-serialized a JSON-sorted receipt dictionary rather
than the original insertion-ordered `inputs(case)` output. The corrected reader
uses existing manifest/lock bytes and reconstructs the exact executed inputs
from the unchanged input function. No execution body, SDK, native archive or
business observation changes during these admission repairs.

The [repaired validation](parity/compiler_scalar_governed_validation_repaired_5_0_6.json.gz)
passes ten actual ordinary/optimized reader commands with unchanged source,
native archives and reference frames. Each Python mode rejects seventy bad
inputs: five source controls, twenty Date controls, twenty remaining-model
controls, fifteen retained temporal controls and ten directly imported
observation controls. The imported tests check schema, bootstrap, rollback,
whole-plan membership, audit, trace, Boolean versions, intent, Fix and committed
error semantics without relying on an earlier output-parser rejection.

The [development checks](parity/compiler_scalar_governed_development_5_0_6.json.gz)
record pinned formatter write/check for 1,819 handwritten C++ files, the exact
upstream lock, module paths and generator ledger. All
[eight selected native tests](parity/compiler_scalar_governed_native_5_0_6.json.gz)
and required source fixtures pass with exact registry/JUnit membership, zero
failures/errors/skips and a final no-work build of the six selected targets.
No complete native, module, sanitizer, MIPS or physical-device epoch is claimed.

The [retention check](parity/compiler_scalar_governed_retention_5_0_6.json.gz)
preserves all eighty SDK manifests and their 3,560 files. This is a hash check,
rather than regeneration. Five scalar-probe directories are absent after
1,503,663,165 measured bytes of disposable outputs were removed. The five raw
execution/failure records occupy 5,568,319 bytes. The selected CTest's owned
JUnit directory is also reclaimed, with its XML retained in the native receipt.
Shared caches and the stable preset build directory remain untouched. The
[milestone](parity/compiler_scalar_governed_milestone_5_0_6.json.gz) binds the
compact evidence and the unchanged preceding producer/C++ source inputs.

Complete per-field Checker/sample consumers, remaining ordinary scalar joins,
the pending E03/E04 decisions, SQ/GP reconciliation and final P7 integration
remain independent exits. This checkpoint does not promote G13/G14/G17 or add
a compatibility exception, runtime dependency or ownership domain.

Recheck the retained business/source results without recompiling the observers:

```sh
source scripts/toolchains/activate.sh
python3 scripts/compiler_scalar_governed_source_admission_5_0_6.py --check
python3 scripts/verify_compiler_scalar_governed_5_0_6.py --check --record docs/parity/compiler_scalar_governed_date_floor_5_0_6.json.gz
python3 scripts/verify_compiler_scalar_governed_5_0_6.py --check --record docs/parity/compiler_scalar_governed_optional_arguments_5_0_6.json.gz
python3 -O scripts/verify_compiler_scalar_governed_5_0_6.py --check --record docs/parity/compiler_scalar_governed_optional_arguments_5_0_6.json.gz
```

Fresh collection uses `--execute`, explicit repeated `--case` selections and a
new `--record` destination. Existing success and failure records must be kept.
