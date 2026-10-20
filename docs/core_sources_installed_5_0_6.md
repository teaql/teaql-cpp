# Installed core source cohort: Rust 5.0.6

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Status: the five complete source files are accepted at the
[installed source checkpoint](source_cohort_acceptance_5_0_6.md), following
the complete 784-test native epoch and installed scoped gates. This is not
complete core-library reexport, capability or P0-P7 acceptance.

## Source responsibilities and installation

The existing complete body reviews remain authoritative:

| Source | Review | Lines |
| --- | --- | ---: |
| `teaql-core/src/value.rs` | [VL01-VL21](core_value_source_review_5_0_6.md) | 463 |
| `teaql-core/src/list.rs` | [SL01-SL25](core_smart_list_source_review_5_0_6.md) | 506 |
| `teaql-core/src/request.rs` | [RQ01-RQ20](core_request_source_review_5_0_6.md) | 624 |
| `teaql-core/src/dynamic_search.rs` | [DS01-DS19](core_dynamic_search_source_review_5_0_6.md) | 297 |
| `teaql-core/tests/dynamic_search.rs` | [Complete source tests](core_dynamic_search_source_review_5_0_6.md) | 221 |

The [staged archive](parity/core_source_staged_reconstruction_5_0_6.json.gz)
restores twelve complete file contents from retained evidence after temporary
working files expired. The [installation record](parity/core_source_installation_5_0_6.json.gz)
checks every original and staged hash before applying them. The clone corpus
preserves the original 32 inputs and expected outcomes; its newly explicit
provenance container has its own hash rather than claiming the expired file's
byte identity.

No production core, generator or generated SDK implementation changes. The
cohort installs missing canonical module consumers and comparison registrations,
a checked source contract, a reference-locked Rust observer workspace, the
retained clone observer and a permanent reproduction command. Eighteen new
consumers explicitly import and link runtime without SQLite or its provider.
Existing consumer bindings remain unchanged.

## Executed scoped gates

The [terminal source receipt](parity/core_sources_installed_5_0_6.json.gz)
records thirteen original/native/imported matrices with **5,071 exact outcomes
per frontend**. They cover Value construction/conversion/formatting/ownership,
DataType diagnostics, SmartList state and generic capabilities, complete request
state/helper conversions, dynamic search and default/clone/post-drop behavior.
The shared Value probe serves two distinct input families; these counts are
observations, not unique API declarations.

All **87** registry identities retain the exact reference versions/checksums.
The actual original tests pass: Value **13**, SmartList **2**, dynamic search
**8**. The original test bodies and observer drivers remain unchanged.

Native **24/24** and imported **24/24** pass with no failures, errors or skips.
Each frontend executes ten complete contract units and fourteen comparator/
subprocess checks. The actual warning-canary child must omit untrusted field
paths and secret values. The clone observer disables default PMR allocation,
destroys its UserContext and checks nested Global allocation explicitly.

All eighteen new imported link commands exclude SQLite and its provider.
Canonical consumer validation retains complete original bodies, and selected
final builds report no work. All captured execution inputs and both reference
identities remain unchanged throughout the source gate.

## Reproduction and remaining acceptance

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_core_sources_5_0_6.py \
    --record /tmp/teaql_core_sources_replay.json.gz
```

Use a fresh receipt path. `--families` selects named subsets and `--profiles`
selects native or module execution; a subset does not qualify omitted families.
The checked-in [contract](parity/core_source_contract_5_0_6.json) guards exact
source, observer and corpus hashes, cardinalities and required unit/test names.

The required complete native integration now passes 784/784 with no failures,
errors or skips, unchanged frozen execution inputs/references and a final
no-work build. The failed 778/782 epoch and its separate four-failure repair
remain preserved. The installed source audit accepts these five files together
with ten separately reviewed responsibilities, making the overall source count
82 accepted, 40 pending and two external exclusions. Broader query/save, model,
privacy, source-grammar and user behavior decisions retain their own gates.
The native receipt is a preceding frozen checkpoint, not a relabeled run after
every follow-up installation. No new sanitizer, MIPS or physical-device result
is claimed here.
