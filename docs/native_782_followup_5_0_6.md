# Follow-up to the failed 782-test native epoch

Runtime reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.

The recorded [full native epoch](native_integration_5_0_6.md) remains a failed
**778/782** run. This follow-up repairs its four failing checks and records
separate scoped execution. It does not qualify a new full-suite epoch, promote
source dispositions or complete any remaining capability or P0-P7 exit.

## Entity and Eval manifest provenance

Both comparators rejected the stored hash of
`tests/parity/rust_core_primitives_reference_5_0_6/Cargo.toml`. Its only change
appended the independent `meta_api_reference_5_0_6` binary. Removing those exact
bytes reproduces the previous manifest hash
`3bbddd629ee449e2084c56919dc540626fb56bc29452212508a2d5271eb796e0`.
The dependency lock, original observers and production source anchors are
unchanged.

The [requalification receipt](parity/native_782_core_manifest_requalification_5_0_6.json.gz)
preserves both old compressed corpora, proves the exact manifest-only change,
checks reference registry identities and replays the unchanged comparators
against provisional corpora before replacing the authoritative anchors. Actual
original Rust, native C++ and imported C++ match all 230 Entity observations and
18 Eval scenarios covering 180 mutations. Eval's existing initialized-diagnostic
and four-format snapshot checks remain active. Inputs, expected outcomes and
every other source hash remain unchanged.

The first helper invocation incorrectly looked for imported executables at the
build root after their successful build. They live in `build_modules_native/modules`.
Its [failed receipt](parity/native_782_core_manifest_initial_path_failure_5_0_6.json.gz)
is retained; no authoritative corpus was changed by that invocation.

To repeat the guarded replay, use a fresh receipt path:

```bash
source scripts/toolchains/activate.sh
python3 scripts/requalify_core_manifest_5_0_6.py \
    --record /tmp/teaql_core_manifest_replay.json.gz
```

## Business-ID fixture independence

The lookup comparator previously read an ignored
`build_inherited_business_binding_bridge_5_0_6` directory without declaring a
preparation step. A clean checkout therefore failed before comparing results.
It now invokes the existing unchanged bridge builder in an isolated temporary
directory. The complete derived provenance must still equal the retained corpus,
and `q.rs` must remain byte-identical to the original generated source.

The original and adapted file hashes, model provenance, observer/dependency
checks and builder hash remain guarded. The 528 expected lookup outcomes and
their corpus are unchanged. Both generated-factory and runtime-composition
paths match all 528 observations on native and imported C++ frontends. The
persistent bridge directory is absent before and after these checks. This
retains the explicit adapted-source qualification; it does not convert the
inherited fixture into untouched producer acceptance.

## Guarded and ordinary mutation provenance

The integration test still expected guarded readback to inherit an attached
sidecar. The locked `teaql-sql/src/mutation_execution.rs` guarded adapter sets
`inherited_redactions: None`; the shared C++ implementation already preserves
that behavior. The test now requires the current masked binding to be present
and the inherited secret to be absent.

A positive ordinary-readback failure control requires both the current binding
and inherited secret, preserving coverage of ordinary propagation. Both controls
use real SQLite writes and a transport read failure through the shared executor.
No production mutation, SQL, privacy or generator implementation is changed.
See the [complete mutation review](sql_mutation_execution_source_review_5_0_6.md)
for the distinct source responsibilities.

## Scoped terminal results

The [targeted receipt](parity/native_782_targeted_followup_5_0_6.json.gz)
contains complete JUnit results: **native 9/9** and **imported 9/9** pass, with
no failures, errors or skips. The selection includes the four repaired checks,
their Context contracts, the original SQL mutation test port and the mutation
execution source comparator. Separate runtime-composition lookup checks also
pass. Selected final builds report no work; exact reference identities remain
unchanged.

Memory and production execution behavior are unchanged. No sanitizer, MIPS,
physical-device or new full-suite result is claimed by this follow-up. The next
qualification batch remains the permanent transaction Send entry and core
source/module registration work described in the parity plan.
