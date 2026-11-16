# Module source-fixture audit and completion plan

The complete imported Python registration audit identifies 390 missing
source-fixture bindings and three outdated script selections. The script
selections are repaired in this checkpoint. The following audit preserves that
checkpoint's scope. The [subsequent shared adapter](module_source_contracts_5_0_6.md)
installs these bindings and completes one additional partial binding. Its
64-test source selection and two native-default guards pass. Its fresh complete
module integration subsequently passes 785/785 with stable source/reference and
executable identities. The original failed epoch and its preset/provenance
repairs remain separately recorded; the audit below retains its original scope.

The baseline remains Rust 5.0.6 runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`. Runtime parent
`514f38f6cd7a467ea58d090b7ddebe79a1c91326` and producer
`ec8eac64f0b69808b0017cf72d74e4aef1523e1d` define the inspected source state.

## Complete registration inventory

The [ordinary audit](parity/module_source_fixture_gap_audit_5_0_6.json.gz)
and [optimized audit](parity/module_source_fixture_gap_audit_optimized_5_0_6.json.gz)
retain the actual 1,101-test native and 722-test module registries. All 403
Python module entries have explicit native counterparts, including ten named
aliases. Each row retains commands, script hashes, native responsibilities,
required source groups and existing module bindings.

| Imported Python responsibility | Entries |
| --- | ---: |
| Direct comparisons | 241 |
| Named custom comparisons | 135 |
| Comparisons outside the parity directory | 13 |
| Source/control/invariant entries | 14 |
| Total | 403 |

Direct-name pairing first exposes 384 missing fixture bindings. Resolving the
ten aliases increases the complete count to **390**, across **61** required
source groups. The transitive fixture closure adds `generator_child_checks`,
giving **62** source groups. `executed_helper_validation` is already included
in the direct set. These are dependency and provenance obligations; this audit
does not establish execution of all module comparisons.

Each audit mode rejects eight inventory corruptions. Complete rows and aliases
remain in the compressed receipts rather than being reduced to a sample list.

## Three repaired entries

Two example comparisons selected `compare_runtime_examples.py` with its
historical 5.0.5 default corpus. Actual invocations in both audit modes fail
with the baseline-refusal diagnostic before executing the example programs.
They now select the native `compare_runtime_examples_5_0_6.py` wrapper, which
checks the current corpus/provenance authority and source hashes.

The clock boundary entry now selects `run_clock_boundary_5_0_6.py`. This is
the same wrapper used by native testing; it disables disposable core dumps
before running the expected clock-failure cases.

The [completed execution](parity/module_cli_entry_repairs_repaired_5_0_6.json.gz)
builds the six required targets and passes **3/3 actual tests in each Python
mode**. Each example comparison matches two SQLite scenarios; the clock test
verifies 36 outcomes. This gives forty scenario outcomes per mode. The five
example/clock program hashes and shared clock-library hash remain unchanged
through execution, and the final scoped build reports no work.

Only these three Python script selections change. Every audited module Python
script now matches its named native counterpart. Full argument and source-fixture
binding qualification remains part of the adapter work below. All 1,101 native
commands and properties remain unchanged; there is no new whole native or module
execution claim.

The separate [ordinary admission](parity/module_cli_entry_admission_5_0_6.json.gz)
and [optimized admission](parity/module_cli_entry_admission_optimized_5_0_6.json.gz)
each reject sixteen corruptions. They enforce the exact three-line CMake change,
source/reference identity, complete JUnit/scenario output, executable identity
and the still-open fixture inventory. The
[initial collector failure](parity/module_cli_entry_repairs_5_0_6.json.gz)
retains the successful build and incorrect inventory key lookup. The repaired
collector uses the audit's `module` key and then executes the complete tests;
the initial failure is not accepted as runtime execution.

## Shared adapter implementation sequence

1. Generate a module source-contract manifest from the authoritative native
   registrations and complete module inventory. Keep explicit aliases, comparison
   programs, corpus selections, executable mappings, fixture owners and relevant
   properties. Reuse canonical module source contracts for program aliases.
   Validate complete arguments and declared equivalences such as an empty host
   emulator argument; script-name equality alone is insufficient.
2. Add a compact test-only CMake registration helper and unconditional manifest
   verifier. They must bind the actual module commands and fixture graph. Keep
   native fixture declarations authoritative, preserve their source checks and
   record generated input identities. Generation is a development step; generated
   manifest data must be regenerated rather than patched by hand.
3. Separate source validation from frontend registration validation. Thirty
   direct source commands pass an optional `--ctest-build-dir`; their inspected
   parsers have neither a required option nor a non-null default. Retain their
   complete source-only checks while replacing the native command checks with
   actual module checks. The transitive generator-child guard separately checks
   native parent names, commands and fixture labels; give it an explicit frontend
   adaptation and preserve all existing child/source controls. Module testing
   must work without an already configured native build directory.
4. Register the complete 62-group closure, reuse the existing composite source
   fixture, and apply all 390 missing bindings systematically. Preserve required
   parent fixtures and relevant environment, timeout and serialization properties.
   Verify valid and corrupted bindings under ordinary and optimized Python,
   execute the affected source/consumer gates, and use a broad module milestone
   when the full adapter is installed. Native full-suite, sanitizer and MIPS runs
   retain their change-specific policies.

The exit requires every existing Python module entry and every newly introduced
source/control entry to have an explicit disposition, with no omitted aliases,
partial argument matches, dropped parent guards or implicit native-cache
dependency. It does not change the Rust grammar, metadata or compatibility
requirements. The subsequent qualified adapter closes these missing bindings
and the additional partial binding for its recorded source checkpoint; see the
linked implementation and complete execution evidence.

## Artifact lifecycle and goal scope

The successful run removes its 1,656,426-byte owned log/state/JUnit directory
after its children exit. The initial small failed-run directory is also removed;
its size was not measured. Controller/checker scripts use owned temporary
directories with exit cleanup. Stable preset outputs and compact evidence are
retained; shared caches and existing source/data are preserved.

No C++ implementation, generated SDK, module export or producer source changes.
This advances E02/E05 in the [goal exit audit](current_goal_exit_audit_5_0_6.md).
Complete G13/G14/G17 and P0-P7 remain active at **14 qualified / 3 partial**.
