# Delegated grammar and nested-signature checkpoints, Rust 5.0.6

Two previously isolated diagnostics are now permanent source-derived metadata
regressions. Their **1,043** delegated expression/type cases and **4,973**
nested signature/pattern cases retain the exact original Rust inputs and
complete observed effects. This is finite AP02/G14 evidence, not complete Rust
grammar or protected-consumer acceptance.

The reference commits are runtime
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source-derived inputs

The earlier [delegated grammar diagnostic](audit_delegated_grammar_diagnostic_5_0_6.md)
contains 801 accepted effects and 242 checked refusals. The separate
[nested signature/pattern diagnostic](audit_nested_signature_diagnostic_5_0_6.md)
contains 3,301 accepted effects and 1,672 checked refusals. The new permanent
resources in the independent producer workspace are
`generator/src/test/resources/cpp_delegated_grammar_metadata_5_0_6.json.gz`
and `generator/src/test/resources/cpp_nested_signature_metadata_5_0_6.json.gz`.
Each stores its original receipt hash and one complete expected metadata object
per input. Both the focused Java corpus test and the full source verifier
consume them.

The preceding [nested-item checkpoint](audit_nested_item_checkpoint_5_0_6.md)
remains separately fingerprinted. No historical receipt is edited or counted as
a new execution.

## Fresh installed-source gate

The [new source receipt](parity/audit_delegated_nested_installed_source_5_0_6.json.gz)
builds and executes the complete unchanged Rust container-attribute parser
under the locked dependencies, checks every retained original expectation,
then compiles and executes the installed Java parser and all focused tests.
Across the expanded **79,346** cases, the implementations agree on **60,215
complete accepted effects** and **19,131 checked refusals**, with **zero
differences**. All **150/150** Java tests pass. Every child command exits
successfully, all **65** input hashes remain unchanged, and the task-owned
**48,915,236-byte** build directory is removed.

Only two regression resources, two focused Java test entries and two source
verifier registrations were added in this continuation. The parser body,
C++ runtime, generated SDKs and module exports remain unchanged. The prior
native **1,058/1,058** pass belongs to a different input epoch; this gate adds
no native, sanitizer, MIPS or physical-router execution.

The scoped command is:

```bash
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_metadata_source_5_0_6.py \
  --record docs/parity/audit_delegated_nested_installed_source_5_0_6.json.gz
```

Complete delegated child-rule coverage, other modeled configurations, actual
protected consumers, the quoted-column compatibility decision and P0-P7 remain
open. G13/G14/G17 stay partial at **14 qualified / 3 partial** capabilities.

The later [three-domain generated caller gate](audit_delegated_nested_callers_5_0_6.md)
checks nine actual descriptors and 42 field-privacy classifications against
fresh original and current generated entities. It remains separate from this
metadata-source execution and from governed SQL or sink behavior.
