# Resolved insert/update preparation contracts

Reference: Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Scope: `prepare_insert_command` and `prepare_update_command` in
`teaql-runtime/src/data_service/resolved.rs`, covering RQ07 and the update
preparation portion of RQ08. Both frontend comparisons pass all 1,690 cases;
installed native checks pass 5/5 and module checks pass 4/4. No production implementation
or generator output changes in this continuation.

## Actual source and observed responsibilities

The fixture is appended to the complete locked runtime through
`ReferenceRuntimeView`, preserving every original production and test body.
It invokes the actual methods directly. The C++ counterpart invokes the same
public preparation methods on `EntityDataService`. Both executors reject any
query or mutation call: preparation cannot accidentally earn persistence evidence.

The matrix has four independent groups:

| Inputs | Responsibility |
| --- | --- |
| 384 | Insert/update, Parent/Child command entities, four behavior modes, four RequestPolicy modes, real bootstrap on/off and three Checker modes |
| 1,210 | Five metadata shapes, eleven ID values, eleven version values and ID allocator success/failure |
| 8 | Absent, observing, fixing or rejecting Checker, with required name absent/present |
| 88 | Update expected version absent, negative, zero or maximum; old snapshot absent/present; eleven input value shapes |

Every callback records its full incoming command, including trace, values and
the update key, expected version and old snapshot. The original input command
must remain unchanged after either success or failure. The final result, exact
error, callback order and allocator counter are compared without normalization.

The main controls establish these distinctions:

- Mutation behavior is selected using the repository entity, even for a
  different command entity. It runs before RequestPolicy. A denied hook stops
  later callbacks and allocation.
- Behavior and Policy may rewrite command entity, scalar values and trace. Update
  hooks additionally rewrite identity/version; each later callback sees the
  preceding changes. Old snapshots remain independent.
- Insert requires the final entity metadata after both callbacks. Missing ID,
  untyped NULL and signed/unsigned integer zero request a new ID; explicit
  typed NULL, negative IDs, text, floating zero and booleans retain their source
  behavior. Version initialization has its own corresponding source rule.
- Metadata may use custom ID/version names, omit either property or be absent.
  Target and Policy entities use different names, so final-entity routing cannot
  be inferred from a fixed `id`/`version` convention.
- The allocator receives the final command entity. Allocation errors precede
  version initialization and Checker execution; the counter is not advanced
  by failure.
- Insert marks the checked values Create before Checker/Fix, then clears the
  reserved marker from successful output. A preexisting Update marker on the
  caller's input remains unchanged. Fix runs before required-property validation.
- Metadata includes timestamp fields, but these preparation methods do not
  fabricate values for them. Graph timestamp preparation has a separate role.
- Update preparation clones and runs its behavior/policy hooks without insert
  ID/version initialization, Checker execution, version increment or required
  metadata lookup. It accepts an absent expected version here; execution owns
  its later requirements.
- Both fixtures enter bootstrap mode through a registered callback under actual
  `ensure_schema`. Behavior is suppressed; RequestPolicy and insert Checker/ID
  preparation remain active.

All C++ fixture state is Context-owned and the default PMR resource is disabled.
There is no new allocation domain or dependency.

## Thin command factories

The four resolved-service mutation factories directly delegate to the core
constructors with the repository entity and supplied identity/version. Their
C++ counterparts add the explicit destination allocator and forward those same
arguments. They invoke no behavior, policy, Checker or provider. The complete
[accepted mutation model review](core_mutation_source_review_5_0_6.md),
MU05/MU06/MU09/MU10, provides the constructor/field/builder obligations; the
resolved-source body review establishes this transparent delegation rather than
adding tests that merely repeat four one-line wrappers. This does not establish
execution or graph Save through those commands.

## Verification boundary

The source-only receipt is
[resolved_mutation_preparation_source_5_0_6.json.gz](parity/resolved_mutation_preparation_source_5_0_6.json.gz).
It records one added caller test and the original runtime/dependency provenance.
The [installed frontend receipt](parity/resolved_mutation_preparation_installed_5_0_6.json.gz)
joins actual source and both 1,690-case comparisons, exact native5/module4
CTest selections, canonical imported consumers and final no-work builds. It
also reruns the preceding 1,036-case select preparation comparator. Pinned
format apply/check passes for 1,722 handwritten C++ files. No sanitizer or MIPS
profile is repeated for these fixture-only changes.

Reproduce with a fresh receipt destination:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_resolved_mutation_preparation_5_0_6.py \
    --record /tmp/resolved_mutation_preparation_fresh.json.gz \
    --cpp build_native_current/teaql_resolved_mutation_preparation_probe \
    --module build_modules_native/modules/teaql_module_resolved_mutation_preparation_probe \
    --check-gates
```

The controls exercise scalar preparation, not generated graph Save or database
execution. Delete/recover orchestration, event delivery,
transaction behavior and the complete resolved-service source retain separate
requirements. Source and capability verdicts do not advance from this witness.
