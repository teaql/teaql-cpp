# Generated Behavior hooks and business customization: Rust 5.0.6

The complete original Note, Invoice and Priority domains execute their generated
module factories and actual registered Behavior callbacks. All **864/864**
original/native observations match without normalization.

## Scope of the source comparison

The matrix combines four module modes, default/recording/denying behaviors, all
six hooks, initial/replaced custom registration and ordinary or long Unicode/
embedded-NUL text. Hooks are before_select, before_insert, before_update,
before_delete, before_recover and relation_loads.

Original generated Behavior structs implement the runtime trait with no overrides.
The C++ generated classes inherit the corresponding permissive runtime defaults.
The Rust custom fixture composes the original struct; C++ subclasses the generated
class. Both delegate to the original defaults before recording and modifying
the supplied command. That is a language binding adaptation, not a replacement
of the source Behavior algorithm.

The complete output preserves registry presence, command Debug before its return,
exact failure text, relation paths and callback order. Replaced registrations use
the new behavior. Absent default registrations remain absent in metadata-only and
checker-only modes. A denying relation_loads fixture still returns its paths:
that source hook returns a vector rather than a fallible mutation result.

These are direct registered-hook contracts. No SQL or authorization decision is
executed by this observer. Protected CRUD dispatch, mutation governance and audit
delivery retain their separate runtime/provider evidence and requirements.

## Application-owned extension example

[custom_checker_behavior.cpp](../examples/custom_checker_behavior.cpp) keeps its
ProductRules and ProductOrder classes outside regenerated domain headers.
ProductRules overrides the generated typed callback and rejects blank names.
ProductOrder overrides before_select and adds name ordering.
The program registers both through a RuntimeModule, uses real audited Save and
reads through generated Q/E with SQLite. Applications still supply authorization
policies. before_select also applies to authoritative Save readback: an initial
example predicate excluded a newly persisted object and caused graph Save to
fail. The final example orders rows without excluding persisted objects.

The initialization order matters: configureRuntime installs defaults. The example
configures first, applies its custom module, then initializes schema and performs
business work. Generated Assist now describes this order and warns that calling
configureRuntime/setup again replaces those registrations. Java regression checks
the emitted guidance; regenerated files are not edited by hand.

Native observers disable default PMR, copy results into Global, destroy stack PMR
values, delete UserContext and serialize only the retained output. Native5,
module2 and ASan/UBSan2 pass; Java generation/metadata tests pass 45/45. Twenty
workspaces/240 files and 44 contract SDKs strictly reproduce. Thirty SDKs update
only their genuinely generated Assist; every SDK code byte stays identical.
The [terminal record](parity/generated_behavior_hooks_5_0_6.json) and
[source closure](parity/checker_behavior_source_closure_5_0_6.json) retain results
and fingerprints. The closure replays 3096 current-reference outcomes through
unchanged existing Checker flow, SQLite commit and ordinary assembly observers,
plus the new Behavior observer. Historical manifests remain unchanged. Sanitizer
leak scanning stays disabled; no MIPS or physical-device result is implied.

The [G02/G03 requirements](checker_behavior_requirements_5_0_6.md) distinguish
this source callback evidence from the remaining committed-save and integration
obligations. Complete G02/G03/P1 acceptance is not claimed by the new corpus.
