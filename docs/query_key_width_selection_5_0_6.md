# Query-key CLI selection and the 32-bit corpus refresh

Later installed source-binding checkpoint:
[The current query-key comparison](query_keys_cli_source_binding_5_0_6.md)
checks both 5.0.6 corpora, the exact selector repair and current native/QEMU
results: 451/451 at each width, with 46 width-sensitive source outputs.
Fifteen substitution controls and six selected native checks pass. The
preparation and installation history below retains its original time scope.

Installed continuation: [the installed milestone](custom_cli_admission_milestone_5_0_6.md)
records the completed 18-file installation, seven new registrations, installed
17/17 native checks and MIPS query-key 451/451. The pre-repair full native934/934
is a separate successful epoch. Preparation receipts and uninstalled-state
statements below describe their original experiments; their evidence is not
relabeled. The four repairs, new corpus and permanent source gates are now
installed. Complete G13/G14/G17/P0-P7 acceptance remains open.

The custom-dispatcher argument review found a real baseline gap in the query-key
wrapper's four-byte-pointer branch. Its native eight-byte branch selects the
current Rust 5.0.6 corpus correctly. Its four-byte branch still selects a retained
5.0.5 corpus, which the shared comparator rejects before invoking an observer.

A fresh reference-source run and selector repair are prepared in compact
receipts. They have not been installed while the full native epoch keeps its
source inputs frozen. Those preparation experiments do not execute a target
runtime. The separate focused MIPS/QEMU run below now passes all 1,123
observations; physical-device execution remains deferred.

## Exact CLI selection

The static review maps all **197 custom registrations** to argument declarations
in **156 parser-source files**. It finds no absent mandatory flags or mismatched
literal positional choices. One row appears to have unknown `--cpp` and
`--cpp-emulator` flags: `query_keys_rust_parity`.

That row is an intentional two-stage parser. `compare_query_keys.py` consumes
`--pointer-bytes` with `parse_known_args`, then forwards the remaining arguments
to `compare_values.main`. Controlled execution of the unchanged Python code
confirms the following routes:

| Pointer bytes | Selected corpus | Current result before runtime execution |
| --- | --- | --- |
| 8 | `query_keys_5_0_6_cases.json.gz` | Admitted; 451 inputs reach the controlled observer |
| 4 | `query_keys_cases_32.json.gz` | Rejected because the corpus baseline is 5.0.5 |
| 16 | None | Rejected by the width argument's choices |

Only the final observer function is replaced by an explicit test spy. The real
wrapper, shared argument parser, baseline check and corpus admission execute.
This demonstrates CLI selection and admission, not C++ query-key correctness.
The static review remains a locator rather than complete argparse, dynamic
branch or semantic verification for all other custom registrations.

## Fresh source observations

The runtime reference is `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and the
generator reference is `45e1572c4956ca963410d8ea436ead3c8857087a`.

The new source run preserves all **451 original inputs** and leaves the old
corpus unchanged. An owned library target points directly at the unchanged
original query-key driver. Its original main function is retained but unused;
a small owned entry point calls the existing `observe_target(input, 32)`.
The original build script and private cache/pagination key methods are unchanged.

Cargo uses `--offline --locked`, actual Debug assertions and a private target
directory. Metadata binds the observer library and original build script to
their copied source paths and all six TeaQL packages to the full **610-file**
reference archive. The replay copies **10 project inputs** and verifies their
hashes, dependency identities and the complete archive after execution.

All **451 results differ** from the historical 5.0.5 expectations. The prepared
new corpus is `tests/parity/query_keys_32_5_0_6_cases.json.gz`; its full data and
the one-line selector replacement are retained in the source receipt. It is
not an overwrite or relabeling of the historical corpus.

The source observer uses its existing identity shells and prepares 32-bit
`usize` values on the host before calling the original algorithms. This is
**host Rust with a declared input-width adaptation**, not execution of a Rust
MIPS binary or the complete production repository/Context. The separate current
C++ MIPS probe now matches all 451 prepared width observations. Installing the
new corpus and running the repaired CLI remain separate, pending actions.

## Prepared selector repair

An owned mini-repository contains the historical corpus, active 64-bit corpus,
proposed new 32-bit corpus and unchanged shared comparator. The original selector
and the candidate run through six controlled scenarios each: explicit 64-bit,
default width, 32-bit, invalid width, missing C++ argument and unknown option.

All **12 CLI runs** have the expected results. The original 32-bit route fails
its baseline guard. The candidate's **6/6 scenarios** select the intended current
corpus or reject the invalid arguments. Every admitted scenario forwards all
451 inputs. The final observer remains controlled; no runtime executable is
invoked by this selector experiment.

A permanent regression is also prepared in
`docs/parity/query_key_width_cli_regression_ready_5_0_6.json.gz`. It runs the
actual two-stage CLI through ten scenarios: default and explicit 64-bit width,
two 32-bit argument spellings, emulator forwarding at both widths, invalid or
missing width, missing C++ executable argument and an unknown option. The
original selector fails exactly the three admitted 32-bit scenarios. The
candidate passes **10/10** controls; all **20 original/candidate CLI runs** are
retained. The observer is explicitly controlled, so these tests verify routing,
argument admission and cardinality rather than runtime algorithms.

After the active native epoch terminates, verify the recorded preconditions,
install the separate new corpus and selector change, then run the actual native
query-key regression and repaired focused MIPS/QEMU CLI check. The focused MIPS
build/execution epoch has already terminated successfully. This source-only
preparation does not close SQ02, the current integration gate or P0-P7. Capability
states remain **14 qualified / 3 partial**.

## Completed focused MIPS execution

The committed `mips` preset and pinned OpenWrt GCC 14.3.0 toolchain successfully
built `teaql_query_keys_probe` in the existing stable output directory. The
configuration selects 74Kc soft-float code and four-byte pointers. After the
linker exited successfully, the controller verified an ELF32, big-endian MIPS
header and executed the probe under QEMU's 74Kf model.

| Matrix | Actual matching target observations |
| --- | ---: |
| Original 451 inputs with fresh 5.0.6 width-prepared expectations | 451/451 |
| Existing readonly-schema and purpose metadata inputs | 672/672 |
| Total | 1,123/1,123 |

`docs/parity/query_key_32_mips_execution_5_0_6.json.gz` is terminal and complete,
with `parity_passed=true`. It retains every command, the build output, compiler
information, ELF header, binary hash/size and both complete raw runtime streams.
The probe binary and all frozen source/reference identities remain unchanged
after execution. This is actual C++ target execution, independent of the earlier
controlled CLI tests. It does not run the still-uninstalled selector repair.

Only the query-key target and these matrices were executed; the full MIPS suite
and physical router were not. The width matrix's reference remains host Rust
with explicit 32-bit input preparation, not an actual Rust MIPS execution.
The native whole-suite epoch remains independently active and keeps source
inputs frozen.

The first controller stopped after successful compiler/version and CMake
configuration checks because it expected `CMAKE_CXX_COMPILER` in the cache.
CMake stores that value in its versioned compiler-information file for this
toolchain. The corrected controller reads and hashes that file, verifies
four-byte pointers, and proceeds to compilation. The initial record is preserved
as `query_key_32_mips_compiler_info_check_failure_5_0_6.json.gz`; it is a
verification-tool failure, not a C++ compiler failure.

## Evidence and cleanup

- `docs/parity/custom_dispatcher_cli_selection_review_5_0_6.json.gz`: all commands,
  parser declarations, helper-source hashes and static limitations.
- `docs/parity/query_key_cli_selection_diagnostic_5_0_6.json.gz`: three executed
  original CLI routes with the explicit controlled observer.
- `docs/parity/query_key_32_source_refresh_5_0_6.json.gz`: complete source recipe,
  manifest/lock, archive, compiler artifact, raw results and proposed corpus.
- `docs/parity/query_key_32_selector_repair_ready_5_0_6.json.gz`: exact selector
  preconditions, proposed corpus checksum and all twelve selector runs.

The permanent CLI regression preparation removed **304,819 bytes** of owned
mini-repository outputs plus **11,922 bytes** of inner trace data after its
children exited. Its source and registration remain unapplied until the frozen
epochs finish.

The source replay removed **221,396,818 bytes** of owned outputs. The selection
diagnostic and repair preparation removed **3,104** and **302,467 bytes**. Their
compact receipts retain **871,502**, **1,529,379** and **872,019 bytes**,
respectively. All child processes exited before cleanup; shared caches and the
active native build directory are preserved. The MIPS controller removed its
empty owned scratch directory and retained its 1,193,471-byte execution receipt;
its stable preset output remains available. No worktree corpus, selector,
production implementation or frozen execution input has changed.
