# Nested metadata path identity against Rust 5.0.6

The shared Java parser now preserves rooted paths and raw identifier spelling
when interpreting nested metadata. Type-path normalization retains its existing
behavior. This is a bounded AP02/G14 correction; complete grammar, protected
execution and P0-P7 acceptance remain open.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source responsibilities

The unchanged `teaql-macros/src/attr.rs` dispatches metadata through
`Path::is_ident`. The locked Syn 2.0.118 nested-meta parser accepts an optional
leading `::` and keyword identifiers. A rooted or multisegment path does not
match a single ordinary key. Proc-macro2 1.0.106 also retains raw identifier
identity: `r#indexed_layout` does not equal `indexed_layout`.

| ID | Required behavior |
| --- | --- |
| NM01 | Parse rooted nested paths without treating them as recognized ordinary keys. An unknown flag allows later callbacks to execute. |
| NM02 | Preserve `r#` spelling during metadata key dispatch. Raw spellings of recognized keys remain unknown paths. |
| NM03 | Preserve callback boundaries. Unknown assignments or groups stop the attribute after earlier effects; a reverse relation is committed only after its complete callback succeeds. Apply the same path rules inside reverse relations. |
| NM04 | Keep metadata identity separate from type normalization. Existing `typePath` callers continue to normalize raw type identifiers and reject rooted types under their existing contract. |

The previous helper stopped parsing at rooted flags and stripped `r#` before
key dispatch. The [original-source diagnostic](parity/audit_rooted_metadata_path_diagnostic_5_0_6.json.gz)
records **106 differences in 349 cases**, including incorrect limits, entity
overrides, indexed layout and reverse-relation multiplicity. For example,
`r#audit_value_max_len=7` must stop as an unknown assignment; it must not
overwrite the preceding limit and continue to later assignments.

The correction shares the cursor's path reader with an explicit metadata mode.
Only the three nested metadata entry points use that mode. No generated C++
method acquires another parser, dependency or ownership mechanism.

## Permanent regression and execution

The [isolated candidate](parity/audit_nested_metadata_path_candidate_5_0_6.json.gz)
matches all 349 retained original observations. The new source-owned Java
resource preserves those exact observations and their source-receipt hash;
the regular source and AST verifiers include it in their complete replay.

The [installed source replay](parity/audit_nested_metadata_path_installed_source_5_0_6.json.gz)
passes **64,267/64,267** cases: **48,965 accepted metadata effects** and
**15,302 checked refusals**, with **142/142 focused Java tests**.
The [AST replay](parity/audit_nested_metadata_path_grammar_5_0_6.json.gz)
also passes, including **270 retained source witnesses**. Refusal does not mean
identical Rust/Java error text or an identical error stage.

Four focused Java tests cover rooted flags, raw keys, nested relation callbacks
and the separate type-path contract. A fifth test replays the complete new
corpus. The [regression control](parity/audit_nested_metadata_path_regression_controls_5_0_6.json.gz)
passes **22/22** tests with the current helper. Substituting the preceding
helper fails exactly the rooted, raw, relation and corpus tests; the existing
type-path contract continues to pass.

The [80-SDK reproduction](parity/audit_nested_metadata_path_sdk_reproduction_5_0_6.json.gz)
preserves all **3,560 generated files** and **2,467 protected artifacts**.
The metadata value change is the shared parser fingerprint. The reproduction
tool also sorts the file-hash map in **41 manifests**; the
[milestone receipt](parity/audit_nested_metadata_path_milestone_5_0_6.json.gz)
reconstructs each preceding serialized manifest and verifies its frozen hash,
allowing only that ordering change and the parser fingerprint. The
[native and source guards](parity/audit_nested_metadata_path_native_guards_5_0_6.json.gz)
pass two affected generated API/wire tests and the upstream, module-path,
generator and custom-source checks; the native build reports no work.
This is not another full native, imported-module, sanitizer or MIPS run.

## Generated caller verification

The [first caller attempt](parity/audit_nested_metadata_path_callers_5_0_6.json.gz)
is incomplete. After three original generated caller workspaces compiled and executed,
the fourth C++ generation failed with `NoSuchMethodError` for
`ObjectDescriptor.getPredefinedFields`. Its cached model and naming classes
belonged to incompatible source epochs; this is not an accepted caller result
or a failure of the repaired metadata dispatch.

The compiler verifier now compiles the producer's complete model and generator
Java sources together into its owned output directory, as it already does for
the original generator. Its existing source-overlay checks and independent
original/producer output directories remain in use. The
[coherent caller replay](parity/audit_nested_metadata_path_coherent_callers_5_0_6.json.gz)
passes **8/8 original/native pairs**, comparing **24 entity descriptors** and
**112 field-policy classifications**. Every original and current generation,
compilation and execution succeeds. Native execution disables default PMR
allocation and destroys each UserContext, checking that live user bytes return
to zero. The failed attempt remains immutable. These observations do not
execute SQL or establish arbitrary protected-publication behavior.

## Remaining grammar review

The [locked owner inventory](parity/locked_parser_owner_inventory_5_0_6.json.gz)
records **1,064 function/method definitions** and **222 opaque item macros**
across **21 selected source files**. All selected Syn/proc-macro2 source members
match their checksum-locked crate archives. Spans, source hashes, module/impl
owners, calls and branch counts make further source review reproducible.

Its **292 parser locators** are a heuristic subset. They do not establish
typed reachability, evaluate conditional compilation, expand macros or cover
every parser responsibility. Neither the inventory nor this finite path
corpus establishes complete AP02 grammar acceptance.

G13/G14/G17 remain partial, with **14 qualified / 3 partial** capabilities.
Other model configurations, remaining protected consumers and the complete
current integration retain their independent exits. The approved JSON,
quoted-FK and invalid wire-profile exceptions are unchanged; the separate
column-attribute decision remains pending.

## Input and artifact lifecycle

The milestone checks **1,814 unchanged C++ inputs** against the preceding
installed checkpoint. Its exact input delta is four producer paths, two
verification scripts, 80 manifests and the G14 evidence ledger. Capability
statuses and all source dispositions remain unchanged: **122 accepted / zero
pending / two external exclusions**.

The seven successful owned preparations and verification runs remove
**330,816,325 bytes** of temporary compiler targets, classes, copied sources,
generated SDKs and executables after consuming their results. Their compressed
receipts total **2,846,012 bytes**, excluding the separate milestone,
installation/status deltas and failed-attempt diagnostic. The failed caller
attempt also removes its owned directory; its removed byte count was not
measured. Stable preset outputs and shared caches remain available.
