# Compiler-resolved identity and optional foreign-key roles

This continues the [eighteen ordinary-field matrix](compiler_scalar_family_5_0_6.md)
under the unchanged Rust 5.0.6 locks. Required entity IDs and optional foreign
keys have distinct storage responsibilities. The ordinary-field matrix used a
required Context Root relationship; its successful operations do not establish
NULL clearing on an optional relationship.

## Source contract and execution plan

The locked `RustFieldDescriptorNaming.rustStorageType` gives object relationships
`u64` or `Option<u64>` according to their optionality. `rustStorageFieldName`
retains the canonical `_id` member independently of the physical SQL column.
The compiler worker observes the getter's actual TypeId, and
`CppCompilerModelConfiguration` checks its complete descriptor membership and
nullable/id/version flags before `CppCompilerScalarStorage` accepts a carrier.
The application model and E facade retain their existing strong entity types.

1. Compile a complete unchanged original model with an optional Gateway
   relationship and required Reading identity through the ordinary v4 Spring
   host/controller. Independently reproduce every original SDK file and retain
   the published complete C++ SDK inventory. Preserve an actual original
   compiler refusal if the model cannot compile.
2. Compile the existing read observers with explicit test-only access-path
   adapters for ID and optional foreign-key getters and E. Observe actual
   SQLite rows with a small ID, a value above 32 bits, the maximum signed
   SQLite integer, NULL and a wide foreign key, plus an empty selection. Do not
   infer unsigned SQLite storage beyond its actual signed-integer boundary.
3. Adapt only the existing governed observer's test operands: assign and clear
   the optional relationship through generated setters, provide a wide target
   row, and keep the fixture's Reading allocation floor at six independently
   of bootstrap floor calls. Exercise NULL-to-NULL, value-to-NULL,
   NULL-to-wide-value and wide-to-small-value transitions through creation,
   update, mixed pending graph Save, whole-plan denial, approval, transport
   rollback, stale root/child versions and committed audit-delivery failure.
   Preserve the original Policy, audit, trace, schema and database observations.
4. Compare complete raw original/native results and independently check stored
   IDs, nullable columns, outcomes and transaction states. Verify exact source,
   library, toolchain, dependency and SDK identities; distinguish execution from
   receipt-reader checks. Keep G13/G14/G17 partial until their full exits hold.

The collection uses owned temporary observer sources and existing collector
mechanics. It captures the complete adapter source and every exact replacement;
no generated SDK, runtime or producer source is patched. C++ probes disable the
default PMR resource, finish operations before UserContext close and check that
all user bytes are released. One-off compiler/source outputs are reclaimed
after their children exit; compact commands, sources and outcomes remain.

The separately retained configured ordinary-`u64` original compiler refusal
remains a refusal. Structural ID/foreign-key support cannot accept that model.
Regular Business IDs retain their distinct semantic identities and String
storage through the [explicit unchanged-source reuse](parity/compiler_scalar_business_id_reuse_5_0_6.json.gz).
Pending Option-model and column-attribute choices and the three approved
compatibility exceptions keep their existing scopes.

## Executed role and governed results

The [actual collection](parity/compiler_optional_identity_roles_5_0_6.json.gz)
successfully runs the ordinary v4 host/controller, unchanged original compiler,
independent original regeneration and both native observers. All eleven recorded
build/run commands exit zero. The independent original SDK reproduces all
nineteen files; the complete published C++ SDK has thirty-nine files. All seven
same-library scalar facts agree with their descriptor membership, including:

| Role | Original getter | C++ getter | Compiled property |
| --- | --- | --- | --- |
| Reading identity | `u64` | `std::uint64_t` | U64, required, ID |
| Reading Gateway foreign key | `Option<u64>` | `std::optional<std::uint64_t>` | U64, nullable, canonical `gateway_id`, physical `gateway` |

Both complete read outputs match. Three typed rows retain IDs 7, 4,294,967,297
and 9,223,372,036,854,775,807. Their foreign keys are 1, NULL and 4,294,967,297.
Getter, E, JSON, generated SQL, intent and counts agree, including the empty
selection. These reads cover the signed SQLite INTEGER boundary, not arbitrary
unsigned 64-bit SQLite storage.

All forty governed outputs also match. Ten operation modes each retain four
operand pairs. Creation consumes the initial operand; the other modes stage
the initial and replacement operands. Explicit NULL remains NULL in the
immutable Policy plan and safe audit rather than becoming zero or an omitted
change. Successful scalar updates and mixed pending graph Saves persist the
replacement; denial, trigger failures and version conflicts retain the original
foreign key. The post-commit sink failure retains committed values and versions
and reports the committed error. No audit is delivered for rollback or denial.

Actual bootstrap is repeatable: it creates Gateway 1 and no Reading. A separate
wide Gateway row is a test operand. The ordinary nullable relationship produces
no Context Root Fix evidence. Both native observers disable default PMR and
release their UserContext blocks after each input. No runtime, producer or
retained generated source change was needed for these results.

## Reader qualification and preserved failures

The [first reader](parity/compiler_optional_identity_validation_5_0_6.json.gz)
rejects the completed collection because it reserializes inputs from a
key-sorted receipt rather than preserving the actual command's source-defined
member order. The [repaired reader](parity/compiler_optional_identity_validation_repaired_5_0_6.json.gz)
reconstructs those bytes from the independent input definition. All four
ordinary/optimized CLI/imported runs pass; business execution is not repeated.

The [independent helper diagnosis](parity/compiler_optional_identity_reader_false_acceptance_5_0_6.json.gz)
then reproduces two metadata false acceptances in each Python mode: a changed
adapter with a correspondingly changed hash, and an empty base-source inventory.
The final reader binds the complete immutable collection image after its
independent semantic checks. Direct read/state/plan/audit helpers retain their
own negative controls. Earlier reader successes keep their original boundaries;
they do not become final source-binding acceptance.

The [final validation](parity/compiler_optional_identity_validation_final_5_0_6.json.gz)
passes all four actual CLI/imported commands under ordinary and optimized
Python. Each mode rejects twenty-four receipt corruptions and five direct
read/state/plan/audit helper corruptions. Input validation uses unconditional
checks. This is a reader repair and verification, not another Rust or native
business execution.

## Finite role closure and retained outputs

The [milestone](parity/compiler_identity_roles_milestone_5_0_6.json.gz) joins
the eighteen ordinary scalar forms to the two structural u64 roles. All twenty
rows of the ten-carrier/optional inventory have named compiler/consumer
dispositions. That finite table does not accept arbitrary configured types or
wrappers. An original compiler refusal remains separate, and a valid original
type without a faithful C++ representation remains an implementation gap.

All 12,124 preceding execution inputs and the native runtime/compiler/SQLite
artifacts remain unchanged. Pinned formatting checks all 1,820 handwritten
repository C++ files; the temporary adapters were also formatted with 18.1.8.
Upstream locks, module paths, the generator ledger and whitespace pass. All
eighty retained SDK manifests and 3,560 generated code/Assist files retain their
hashes; this check is not SDK regeneration. The two new native observers were
actually built and run; no existing CTest suite or module/sanitizer/MIPS/device
profile is repeated or relabeled.

Eight exact recorded disposable paths are absent. Six distinct byte
measurements total **614,026,719 bytes reclaimed**. Captured temporary source
directories are also absent without adding unmeasured bytes to that total.
Five compact execution/reader records occupy 4,185,987 bytes before the
milestone. Shared caches and stable build presets remain intact.

Recheck the immutable final reader with an owned temporary source directory:

```sh
source scripts/toolchains/activate.sh
python3 - <<'PY'
import gzip, hashlib, json, pathlib, subprocess, sys, tempfile
receipt = pathlib.Path("docs/parity/compiler_optional_identity_validation_final_5_0_6.json.gz")
blob = receipt.read_bytes()
if hashlib.sha256(blob).hexdigest() != "b9f271d7792f4065730487c2d48efc3de37452c59b6571aa8aa38ca330e5f895":
    raise RuntimeError("Unexpected final reader receipt")
record = json.loads(gzip.decompress(blob))
with tempfile.TemporaryDirectory(prefix="teaql_identity_reader_recheck_") as directory:
    reader = pathlib.Path(directory) / "reader.py"
    reader.write_text(record["validation_source"])
    for flags in ([], ["-O"]):
        result = subprocess.run([sys.executable, *flags, str(reader)], check=True,
                                capture_output=True, text=True, timeout=180)
        outcome = json.loads(result.stdout)
        print(outcome["optimized"], len(outcome["receipt_controls"]),
              len(outcome["direct_controls"]))
PY
```

Complete compiler/configuration routing and redundant interpretation, pending
E03/E04 decisions, remaining source/consumer/artifact joins and final P7
integration retain their own exits. G13/G14/G17 remain partial. No dependency,
ownership domain, compatibility exception or capability promotion is added.
