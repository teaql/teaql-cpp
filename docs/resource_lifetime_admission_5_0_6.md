# Resource lifetime comparator admission, Rust 5.0.6

Installed continuation: [the installed milestone](custom_cli_admission_milestone_5_0_6.md)
records the completed 18-file installation, seven new registrations, installed
17/17 native checks and MIPS query-key 451/451. The pre-repair full native934/934
is a separate successful epoch. Preparation receipts and uninstalled-state
statements below describe their original experiments; their evidence is not
relabeled. The four repairs, new corpus and permanent source gates are now
installed. Complete G13/G14/G17/P0-P7 acceptance remains open.

Controlled execution of the actual resource-binding comparator exposes **21
false acceptances**. An isolated repair passes all **26 admission controls**.
The repair and permanent regression are prepared but remain unapplied while
the full native epoch holds its execution inputs frozen.

These are comparator defects, not evidence that the current runtime loses
resources or permits expired borrows. The experiment replays retained outputs
through executable stubs; it executes neither Rust nor C++ runtime code.
Capability status remains **14 qualified / 3 partial**.

## Confirmed gaps

`compare_resource_bindings_5_0_6.py` already checks all 384 semantic observations
with the shared type-preserving comparator. It separately checks the declared
Rust replacement-drop/C++ arena lifetime adaptation, using ordinary Python
equality and truthiness for some fields. Those additional checks have three
independent gaps:

- Boolean lifecycle flags accept some numeric or string substitutions. Drop
  counters also accept Boolean or floating values equal to the expected integer.
- `captured` and `old_borrow_live_before_close` are compared only to each other.
  Both can be falsely cleared when a typed insertion requires capture, or both
  falsely set when no typed insertion occurs.
- The observer decoder discards blank LF records instead of rejecting them.
  A complete result stream with an extra blank record still passes.

The expected capture follows the actual native probe: its first `typed` action
retains a borrow; `named`, `other` and `check` actions do not. The probe accesses
that retained resource before deleting the UserContext, then checks rejected
access and copying after deletion. The comparator must require that requested
capture, rather than merely agreement between two reported flags.

## Permanent controlled regression

| Control family | Scenarios |
| --- | ---: |
| Numeric/string substitutions in lifecycle flags | 10 |
| Lost and fabricated capture | 2 |
| Original/native drop-counter type substitutions | 4 |
| Created/destroyed-total type substitutions | 4 |
| Extra blank LF record | 1 |
| Valid retained profiles | 1 |
| Missing/extra output record | 2 |
| Incorrect drop count and permitted expired access | 2 |
| Total | 26 |

Each CLI run consumes the complete **384-input** matrix. The regression uses the
actual comparator and shared helper, the root baseline lock, all **25 original
SDK files**, all **60 native generated files**, both provenance manifests and
the original model. These files are copied unchanged into an owned mini-repository.
The stub combines retained original semantic results with the independently
retained native ownership profile. All fixture/provenance checks execute.

Every mutation is explicit and retained. Four original/native total-counter
controls alter both the copied expected corpus and emitted semantic data to
expose the independent integer contract; two original drop controls alter only
the copied expected ownership profile. These are deliberate saved-corpus
corruptions, not altered reference sources or newly observed source outcomes.
The source worktree, retained corpora and generated SDKs remain unchanged.

Against the original comparator, the valid profile and four pre-existing
rejection controls behave correctly, while all **21 newly exposed corruptions
pass incorrectly**. Against the candidate, all **26/26 controls** have their
expected outcomes. The **52 actual CLI runs** retain exit codes, stdout, stderr,
mutations and the exact controlled observer source. Expected rejections must
contain their specific diagnostic, rather than merely exit with any error.

## Prepared repair and application gate

The candidate preserves the comparator's existing ownership adaptation and
semantic projection. Its changes are:

1. Decode output with the existing LF-framed `json_lines` helper.
2. Compare all declared drop and final destruction counters with `same`.
3. Require actual Boolean lifecycle flags, and derive both capture flags from
   the presence of a `typed` action in the request.

`docs/parity/resource_lifetime_admission_repair_ready_5_0_6.json.gz` retains the
complete patch, permanent regression source, CMake registration, precondition
hashes and original/candidate execution receipts. The proposed regression is
`tests/parity/verify_resource_lifetime_admission_5_0_6.py`; its CTest name is
`parity_resource_lifetime_admission`. `worktree_patch_applied=false` remains
accurate until the native epoch releases its inputs.

After that epoch terminates, verify the recorded preconditions, apply the
prepared changes, and run the permanent regression together with the actual
native resource-binding comparison and existing comparator controls. This
experiment establishes admission behavior only. Fresh source/runtime binding,
complete custom/protected qualification, G13/G14/G17 and every P0-P7 exit retain
their independent requirements.

## Lifecycle and retained evidence

Both original and candidate regressions finish and reap their child processes
before removing their owned files. Preparation removed **1,677,549 bytes** of
outer outputs and **15,573,541 bytes** of inner fixtures, stubs and diagnostics.
The compact preparation receipt retains **882,499 bytes**.
The complete native epoch's **11,639 input hashes** remain unchanged. Stable
preset directories and shared caches are preserved; no sanitizer, MIPS or
physical-device rerun is introduced by this comparator-only preparation.
