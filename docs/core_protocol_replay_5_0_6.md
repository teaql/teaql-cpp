# Historical core protocol replay

Status: ten original-call families reproduce against actual Rust 5.0.6. Nine
complete families match native C++; the structural row formatter has a diagnosed
source mismatch. Active CTest selections remain historical during the running
complete native test run. Complete G13/G15/G16/P7 remain open.

Reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`. Every original input and
its order are preserved. Driver calls/logic are unchanged; only the relative
shared fixture import changes where needed in the active observer workspace.
Old drivers and corpora remain intact. New corpora record source/historical
hashes and full outputs without normalization, then reproduce in a second run.

| Family | Original observations | Native result |
| --- | ---: | --- |
| Core Timestamp traits and descriptor builders | 259 | Pass |
| Base entity dynamic operations | 1,481 | Pass |
| Entity registration/default/override/failure ordering | 42 | Pass |
| Entity diagnostics | 289 | Pass |
| Eval/load-state protocol | 1,136 | Pass |
| Internal ID/default/bootstrap protocol | 171 | Pass |
| Model/row/list diagnostics | 169 | Fail |
| Mutation diagnostics | 120 | Pass |
| Query diagnostics | 120 | Pass |
| Business ID core rules/errors/allocation protocol | 1,283 | Pass |

All **5,070** Rust observations reproduce. Nine complete native families match
**4,901/4,901**; the [terminal family record](parity/core_protocol_replay_5_0_6.json)
preserves the remaining failure. These are core public-call fixtures, not the
complete generated model/configuration matrix. The original internal-ID driver
observes wall-clock validity through boolean predicates; this checkpoint does
not claim equal wall-clock IDs or deterministic time values.

## Row diagnostic defect

All **169/169** model diagnostic inputs differ. Source CompactRow Debug now
shows a CompactRowLayout with names, relation names, readonly definitions,
primary loaded-state cache and other-type cache, plus the row's optional loaded
read metadata. C++ still shows the historical names vector and native values
only. The discrepancy propagates to every enclosing SmartList/facet formatter.

The [difference inventory](parity/model_debug_source_gap_5_0_6.json) retains every
failed index/format path and one complete compact example. Existing ColumnLayout
already implements the responsibility and caches; the formatter must reflect
them rather than fabricate new ownership, locks or caches for diagnostic text.
Rust Arc/Weak and mutex implementation details require explicit C++ Context
adaptations for initialized caches. The recorded 169 inputs are uninitialized
ordinary layouts, with no schemas or read sidecars; they do not qualify those
broader metadata/loaded-state formatting configurations.

Keep current full-run inputs unchanged until the last compiler check exits.
Then implement the shared formatter, qualify ordinary and nonempty metadata/cache
states with source/native evidence, and update the active selection only after
actual agreement. Do not replace the source output with the old formatter text.
This is distinct from the separately diagnosed raw LIKE provenance correction.

## Reproduction

After the ten separately named Rust binaries are built with the locked observer
manifest, original/native replay uses:

```sh
python3 tests/parity/replay_historical_inputs_5_0_6.py \
  --family core_api --family base_entity_api --family entity_registration_api \
  --family entity_debug --family eval --family internal_id --family model_debug \
  --family mutation_debug --family query_debug --family business_id \
  --rust-dir /tmp/teaql_cpp_field_layout_rust_506/debug \
  --cpp-dir build_native_current --check
```

The aggregate command currently fails on model_debug; individual families can
be replayed separately to retain all outcomes. New versioned comparators exist,
but current CMake selections are unchanged. No production C++, generator,
dependency, sanitizer/module/MIPS or physical-device change is inferred.
