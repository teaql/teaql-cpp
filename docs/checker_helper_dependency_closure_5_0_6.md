# Checker helper dependency closure and configuration gap

G02/G03 remain partial. This checkpoint closes the current-reference replay of
the existing basic Checker and ordinary Business-ID helper observers and records
a newly reproduced generated Checker compilation gap. That initial gap is now
repaired in [the string-clock checkpoint](checker_clock_text_5_0_6.md); the
source behavior and initial compiler results below remain historical evidence.

## Current-reference helper replay

`scripts/verify_checker_helpers_source_closure_5_0_6.py` builds two independent
observer packages with `cargo build --offline --locked`. Their build scripts
extract the existing observer functions from `rust_field_layout/src/main.rs`.
The verifier compares the actual Cargo build-script output with those function
bodies byte for byte before executing the binaries. The historical observer,
its manifest and the original generated libraries remain unchanged.

All registry versions and checksums must belong to the locked Rust 5.0.6
Cargo.lock. The basic helper package uses the complete retained School library;
the Business-ID package uses the complete frozen producer's Business-ID crate.

| Matrix | Live outcomes | Contract |
| --- | ---: | --- |
| Basic Checker helpers and entity annotations | 274 | 220 required/optional/text/Unicode-length findings and 54 annotation operations retain the original observations. |
| Ordinary generated Business IDs | 1044 | Allocation, validation, roots, missing services, profiles, immutable updates and evidence retain the original observations. Only the already declared resource type-name diagnostic adaptation applies to C++. |

The [dependency closure record](parity/checker_helpers_dependency_closure_5_0_6.json)
contains terminal build/comparison results, compiled-body hashes and input hashes.
It adds current-lock execution evidence without rewriting the historical corpora.

```sh
set -e
source scripts/toolchains/activate.sh
cmake --build --preset native --target teaql_loaded_snapshot_probe
python3 scripts/verify_checker_helpers_source_closure_5_0_6.py
```

## Generated `uniStr()` Checker gap

The locked producer's system-types.xml defines `uniStr` as a string with a
`createFunction` attribute. Its checker template treats every such attribute as
a clock fix. The [minimal model](../tests/generator/checker_unique_string.xml)
was rendered through the complete original Rust producer and the independent
C++ producer using the existing TypeConfigurationProbe.

The complete unchanged Rust crate compiles against the current reference
dependencies. Its actual NoteChecker produces these observations:

| Status | Text getter | Changed code is Timestamp | Findings |
| --- | --- | --- | ---: |
| Create | `existing code` | Yes | 0 |
| Update | `existing code` | No | 0 |
| Unknown | `existing code` | No | 0 |

Rust's generated string setter accepts `Into<Value>`: it retains the previous
stored string when the operand is a Timestamp, stages that Timestamp in the
mutation ledger, and marks the field loaded. A create fix consequently retains
both the text getter and the differently typed mutation operand.

C++ generation succeeds, but including its complete generated domain fails:
`GeneratedFieldChecks::clock` calls `entity.updateCode(time)` while updateCode
accepts `std::string_view`. The native compiler reports no viable conversion
from Timestamp to string_view. This is a C++ gap; the original Rust artifact
does not have this compilation failure.

The [initial gap archive](parity/checker_unique_string_initial_gap_5_0_6.json.gz)
preserves all generated files, model identity, the unchanged Rust observer calls,
its dependency lock, source results and complete native compiler diagnostics.
Each of its 34 files has a SHA-256 hash. The original source worktrees remain
clean. No generated file was patched to make either compilation outcome occur.

The repair must first provide the shared runtime operand/stored-value mechanics,
then emit the thin clock-fix adapter. It must preserve the source Timestamp
operand, unchanged stored text, availability and ordered fix evidence. A normal
string creation operation alone would not prove this Checker contract. Optional
and configured non-timestamp clock fields need explicit source dispositions
during the remaining G03 branch review.

## Integration checkpoint

After successful selected builds, native passes 13/13, named modules 5/5 and
ASan/UBSan 5/5 with leak scanning disabled. These gates cover the existing
Checker, optional/Business-ID, ordinary/inherited Behavior, protected deletion/
recovery and application customization paths. They do not include the failing
new `uniStr()` configuration. Twenty retained workspaces/240 files and 44
contract SDKs strictly reproduce; pinned formatting checks 1377 handwritten
C++ files. See the [terminal record](parity/checker_helper_dependency_terminal_5_0_6.json).

G02/G03 acceptance and P1/P7 remain open. The runtime/model/cache, privacy,
durable provider, assembly and intent/trace obligations retain their existing
owners and are not retired by this helper checkpoint. No MIPS or physical-device
execution was performed.
