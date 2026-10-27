# Visibility and parenthesized type children, Rust 5.0.6

The shared generator parser now preserves visibility lookahead, ordinary type
dispatch and parenthesized child-buffer boundaries. This corrects metadata
that was lost or incorrectly accepted after ignored expressions. It is a bounded
AP02/G14 correction; full grammar, remaining protected execution and P0-P7
acceptance remain open.

References remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Source contracts

The [reviewed source owners](parity/audit_visibility_source_owners_5_0_6.json.gz)
match the checksum-locked Syn 2.0.118 archive. Four owners in `restriction.rs`,
`ty.rs` and `path.rs` define the following responsibilities. The earlier selected
owner inventory did not include `restriction.rs`; its locators were not complete
grammar coverage.

| ID | Required behavior |
| --- | --- |
| VT01 | After `pub`, consume `(crate)`, `(self)` or `(super)` as restricted visibility only when that child buffer contains exactly the keyword. Otherwise leave the parentheses for the field type, including `pub ()` and `pub (crate::T, U)`. |
| VT02 | `pub(in path)` commits its child buffer after a valid module path. Remaining child tokens become an ignored enclosing parse error under the original attribute callback behavior. Preserve later metadata effects. |
| VT03 | Ordinary type dispatch excludes bare `try`. Preserve it where the source accepts it: raw identifiers, rooted or subsequent path segments and trait-bound paths. Expression-path parsing retains its separate rules. |
| VT04 | A single parenthesized type can leave child tokens that the enclosing attribute callbacks ignore. Complete type strings, including `reverse_relation.json_type`, must reject those tokens. Tuple elements continue to require their full comma-delimited structure. |
| VT05 | A parenthesized leading lifetime dispatches to a trait object rather than tuple parsing. `('a + T, extra)` therefore cannot become an accepted complete tuple type. |

For example, `{struct S(pub (T)); 5}` must consume the ignored expression before
processing later `entity`, audit-limit and indexed-layout settings. Previously
the parser treated `(T)` as an invalid visibility and stopped those callbacks.
In contrast, a malformed complete `json_type` must stop its reverse-relation
callback without committing the relation or applying later settings.

The [expanded original-source diagnostic](parity/audit_visibility_parenthesized_diagnostic_5_0_6.json.gz)
records **2,259 differences in 6,920 cases** with the preceding helper. The
unchanged original container parser, full 610-file source archive, actual locked
dependencies, raw observations and controller are retained. Earlier diagnostic
and candidate attempts remain separate, including the attempts that exposed the
bare-`try` and lifetime-leading child cases.

## Installed verification

The [final isolated candidate](parity/audit_visibility_final_candidate_5_0_6.json.gz)
matches **6,920/6,920** original observations. The source-owned Java resource
retains the exact inputs and expectations with the original receipt hash.
The regular verifier includes it without changing earlier corpora.

The [installed source replay](parity/audit_visibility_installed_source_5_0_6.json.gz)
passes **71,187/71,187** metadata cases: **54,017 accepted effects** and
**17,170 checked refusals**, plus **147/147 focused Java tests**. The
[AST replay](parity/audit_visibility_grammar_5_0_6.json.gz) passes the same
metadata matrix and **270 retained witnesses**. These finite observations do
not establish complete Rust grammar coverage. Refusal requires a checked
rejection, not identical Rust/Java error messages or stages.

Four focused tests cover the repaired responsibilities; a fifth replays the
complete new corpus. The [regression control](parity/audit_visibility_regression_controls_5_0_6.json.gz)
passes **27/27** tests with the current helper. Substituting the preceding helper
fails exactly those five new tests.

The [generated caller gate](parity/audit_visibility_callers_5_0_6.json.gz)
passes **8/8 original/native pairs**, comparing **24 entity descriptors** and
**112 field-policy classifications**. Complete original and current generator
sources compile in separate owned outputs. Each generated Rust caller and the
native caller build and execute successfully. Native execution disables default
PMR allocation, destroys each UserContext and verifies that live user bytes return
to zero. This gate does not execute SQL or qualify every protected consumer.

The [SDK reproduction](parity/audit_visibility_sdk_reproduction_5_0_6.json.gz)
preserves all **80 SDKs / 3,560 generated files** and **2,467 protected artifacts**.
Only the shared parser fingerprint changes in their manifests. The
[milestone](parity/audit_visibility_milestone_5_0_6.json.gz) reconstructs every
preceding serialized manifest and verifies its frozen hash with that single
fingerprint restored. No generated source was edited by hand.

The [native and source guards](parity/audit_visibility_native_guards_5_0_6.json.gz)
pass both affected generated API/wire tests, upstream and module-path checks,
generator provenance and the three retained custom-source validators. The native
build reports no work; **949** is the registered test count, not a full-suite
execution. No new sanitizer, imported-module, MIPS or physical-router run is
claimed.

## Scope and artifact lifecycle

All **1,814 C++ inputs** retain the preceding source-program checkpoint bytes.
The exact input delta consists of four producer paths, the source-verifier
registration, 80 SDK manifests and the G14 evidence ledger. Existing source
dispositions and capability statuses remain **122 accepted / zero pending /
two external exclusions** and **14 qualified / three partial** respectively.

Fourteen owned preparations and verification runs remove **516,817,247 bytes**
of disposable classes, copied sources, generated workspaces, targets and native
executables after consuming their results. This includes incomplete candidates;
their compact diagnostics remain. Stable preset outputs and shared caches are
retained. Existing JSON, quoted-FK and invalid wire-profile exceptions remain
unchanged; the separate column-attribute decision remains pending.
