# Nested item checkpoint against Rust 5.0.6

The current installed generator parser matches the original Rust metadata
effects for all **2,143** cases in the previously uninstalled nested-item
checkpoint. This is a finite AP02/G14 grammar gate. Complete delegated grammar,
all modeled configurations, protected consumers and P0-P7 remain open.

The reference commits are runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Retained boundary

The [historical candidate](parity/audit_nested_item_rules_candidate_5_0_6.json.gz)
was an isolated Java source copy, not an installed producer change. Its cases
cover nested declarations, receiver position and references, variadic
arguments, generic whitespace, alias `where` clauses, imports, visibility and
module names. Later parser work changed the installed source independently.
The current parser matches the retained original outcomes for every case; this
does not retroactively install the historical candidate source or relabel its
execution as a current run.

The permanent source-derived corpus is stored in the independent producer workspace as
`generator/src/test/resources/cpp_nested_item_checkpoint_metadata_5_0_6.json.gz`.
It retains the historical receipt hash, exact inputs and complete original
metadata objects, including partial effects before ignored parse errors.
The focused Java corpus test and the full source verifier both consume it.

## Current execution

The [fresh original/current source gate](parity/audit_nested_item_checkpoint_source_5_0_6.json.gz)
builds the complete unchanged Rust container-attribute parser with the locked
dependencies, checks all retained expectations against fresh original outputs,
then compiles the installed Java sources and executes their metadata probe.
Across the expanded **73,330** cases, all complete accepted metadata effects
and checked refusals agree: **56,113 accepted**, **17,217 refused**, and **zero
differences**. The new 2,143 cases are part of this total, not an additional
whole-grammar claim. All **148/148** focused Java tests pass. The source verifier
records unchanged input hashes, successful exits for every child command and
removal of its **48,914,882-byte** owned build directory.

Only a regression resource, one Java test and the source-verifier registration
were added. The parser implementation, C++ runtime, generated SDKs and module
exports were not changed by this checkpoint. The prior native **1,058/1,058**
run retains its separate input identity; this source gate is not another native,
sanitizer, MIPS or physical-router run.

The scoped command is:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_source_5_0_6.py \
  --record docs/parity/audit_nested_item_checkpoint_source_5_0_6.json.gz
```

The quoted-column compatibility decision remains pending. G13/G14/G17 remain
partial at **14 qualified / 3 partial** capabilities.

The later [delegated/nested diagnostic continuation](audit_delegated_nested_checkpoint_5_0_6.md)
adds 6,016 different historical cases to the permanent corpus, with its own
fresh source receipt. The 73,330-case result above retains its original input
identity.
