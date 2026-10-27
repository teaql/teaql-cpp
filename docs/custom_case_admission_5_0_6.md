# Nonempty corpus admission in custom comparators, Rust 5.0.6

Installed continuation: [the installed milestone](custom_cli_admission_milestone_5_0_6.md)
records the completed 18-file installation, seven new registrations, installed
17/17 native checks and MIPS query-key 451/451. The pre-repair full native934/934
is a separate successful epoch. Preparation receipts and uninstalled-state
statements below describe their original experiments; their evidence is not
relabeled. The four repairs, new corpus and permanent source gates are now
installed. Complete G13/G14/G17/P0-P7 acceptance remains open.

Actual CLI replay confirms **18 zero-observation false acceptances** across
six custom comparators. Their isolated candidates pass **60/60 controls**.
The prepared guards and permanent regression remain unapplied while the full
native epoch holds its execution inputs frozen.

The observers in this experiment are explicitly controlled saved-output stubs.
This verifies Python CLI admission, not new Rust/C++ execution or source parity.
G13/G14/G17 and P0-P7 remain incomplete; capability status stays
**14 qualified / 3 partial**.

## Confirmed behavior

Each original entry point accepts `cases=[]`, `cases={}` and `cases=""` as an
empty input matrix. It then succeeds after comparing no observations. These are
three distinct actual false acceptances per entry point, not a static inference.
The graph-limit script even prints `Matched 11 JSON graph boundary/view cases`
when the controlled observer receives zero inputs.

| Comparator | Rows in its unchanged complete corpus |
| --- | ---: |
| Audit events | 1,427 |
| In-memory engine | 6,532 |
| Log manager | 94 |
| JSON graph limits | 11 |
| Generated dynamic JSON | 1,181 |
| Generated expression diagnostics | 196 |
| Total | 9,441 |

The shared default comparator already rejects empty/non-list case collections.
These custom entry points implement their own corpus loading and bypass that
check. The issue does not imply a defect in the runtime features they exercise.

## Prepared guards and regression

Each candidate adds the same explicit nonempty-list check before constructing
its input matrix or launching the target observer. The shared helper, existing
comparison algorithms, adapters, normalization and runtime code remain unchanged.
Invalid containers produce the specific English diagnostic:
`Comparison corpus cases must be a nonempty list`.

The permanent regression executes ten scenarios for each of the six entry points:

- The unchanged complete corpus.
- Empty list, object and string collections.
- NULL, Boolean and numeric collections.
- A NULL record, missing input and missing expected output.

All six complete matrices pass for both the original and candidate. Their
**9,441 corpus rows** remain intact; no smaller positive matrix replaces them.
The audit comparator retains its existing width acceptance/filtering stages,
so this row total is not a count of newly executed runtime outputs.

The original comparator set accepts all 18 empty collections. NULL/Boolean/
numeric collections already fail, but without the required explicit early guard;
some first launch a pointer-width probe. The candidate rejects all six malformed
container forms per entry point with the intended diagnostic and **no observer
invocation**. Malformed nonempty records retain checked rejections. All
**60/60 candidate controls** pass, with **120 original/candidate CLI runs**
retained in the preparation receipt.

## Executed boundary and provenance

The owned mini-repositories contain unchanged comparator/helper sources, the
baseline lock, all six original corpora, their SQL fixtures, three complete
original generated SDKs (**111 files**), provenance manifests, models and extra
expression sources. The actual provenance and fixture checks execute. Only the
copied case collection and the reviewed candidate guard differ during controls.

Controlled executables return retained observations through the scripts' actual
subprocess paths. They preserve target arguments, emulate the declared 64-bit
width, and use the unchanged audit count helper for its count-validation stage.
Dynamic JSON and expression diagnostic outputs use the scripts' actual existing
adaptation functions. The log-manager stub emits saved stdout, stderr and files
inside the comparator's owned per-fixture directories. All subprocesses exit
before their outputs are consumed or temporary directories reclaimed.

These stubs test admission and existing plumbing. They do not qualify target
arithmetic, audit formatting, log publication, source algorithms or generated
runtime behavior. The complete original and candidate source bytes, case
variants, observer code, commands, trace records, exit codes and raw diagnostics
are retained. No reference source, worktree corpus, generated SDK or frozen
execution input changes.

## Application and remaining work

`docs/parity/custom_case_admission_repair_ready_5_0_6.json.gz` retains exact
precondition hashes, all six patches, the complete permanent regression and its
CMake registration. The proposed source is
`tests/parity/verify_custom_case_admission_5_0_6.py`; its CTest name is
`parity_custom_case_admission`. `worktree_patch_applied=false` remains accurate.

After the native epoch terminates, verify the preconditions, apply these guards
and the regression, and run the six actual native comparisons plus their new
admission test. This review covers these six entry points; the complete set of
197 custom registrations, protected/source provenance, other selection paths
and all phase exits retain their independent requirements. The earlier shared
admission and resource-lifetime repairs retain their separate evidence/counts.

## Cleanup

Both regression processes finish before cleanup. Preparation removes
**1,868,411 bytes** of outer outputs and **67,467,133 bytes** of inner fixtures,
controlled outputs and traces. The compact receipt retains **901,540 bytes**.
The log-manager's individual fixture directories are additionally reclaimed by
its unchanged context managers and are not included in those final-tree byte
measurements. Stable preset outputs and shared caches remain in place. No
sanitizer, MIPS or physical-device rerun is added by this Python-only preparation.
