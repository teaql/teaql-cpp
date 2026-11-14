# Inherited-business provenance under optimized Python

This is a bounded SQ02/G13 repair for the adapted inherited-business source
driver and its existing negative-control entry. It preserves the original
Rust 5.0.6 compiler failure, the adapted-source scope, both retained 60/20-case
corpora, and all generated C++ behavior. It does not turn the broken original
SDK into an accepted artifact or close SQ02, G13, G14, G17, or P0-P7.

The [pre-edit witness](parity/inherited_business_optimized_preedit_5_0_6.json.gz)
pins the original driver and control sources and the unchanged execution and
contract receipts. It changes only `complete` from `true` to `false` in a
task-owned copy of the saved execution. Ordinary Python rejects the incomplete
record. `python3 -O` removes the driver's assertions, exits successfully and
prints the ordinary verification message. This is a false acceptance of saved
evidence, not a new Rust or C++ execution result.

The [source transition](parity/inherited_business_assert_rewrite_5_0_6.json.gz)
replaces exactly **100** direct driver assertions and **2** control-entry
assertions with unconditional checks. Whole-AST comparison verifies the
assertion-only rewrite. Five declared driver edits then admit the historical
support hashes and exact old/current source modes, check the input-recipe hash
unconditionally, bind the saved runtime-driver hash to its source epoch, and
refuse optimized archive replay while the shared archive runner retains
removable assertions. The old execution and registry bytes are not rewritten.
The [transition helper](../scripts/inherited_business_source_transition_5_0_6.py)
pins the manifest and pre-edit witness by SHA-256 and rejects mixed source
epochs or unrelated source changes.

The [permanent control](../tests/parity/verify_inherited_business_assert_rewrite_5_0_6.py)
checks six source-transition corruptions. Under ordinary and optimized Python,
it accepts the unchanged historical record, rejects the incomplete copy, and
runs all **35** existing evidence controls. It also checks that optimized
archive replay refuses before creating an output. Its
[control receipt](parity/inherited_business_assert_rewrite_controls_5_0_6.json.gz)
records the outcomes and removal of its task-owned incomplete record.

Selected native CTests pass **5/5** normally and **5/5** with
`PYTHONOPTIMIZE=1`: source contract, two real inherited-business comparisons,
the existing 35-control entry, and the new transition control. The current
registry has **1,084** tests and **602** registered Python entry scripts;
**80** retain **833** direct `ast.Assert` nodes. Imported helpers and the
other registered comparators still require SQ02 review. No full 1,084-test
native epoch has run for the current source state. Production C++, generator,
generated SDKs, locked references, and stable preset outputs are unchanged.
