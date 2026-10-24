# Postfix and attribute metadata grammar (Rust 5.0.6)

Status: the verified correction is installed after the frozen native epoch
terminated successfully. The prototype sections below retain their original
uninstalled checkpoint scope. Complete AP02 and P0–P7 remain open.

## Installed source, callers and artifacts

The preceding [full native epoch](parity/audit_expression_full_native_terminal_5_0_6.json.gz)
passes **877/877**, including 196 generated API compiler contracts, with zero
failures/errors/skips and unchanged 11,413 inputs/references. Its owning wrapper
reaps the child and removes 1,115,108 temporary log/state/JUnit bytes. This is the
frozen C++ checkpoint before the later syntax-parser installation.

The producer installs the exact verified candidate with SHA-256
`d1d6cf3e17ea515ccc7d482a775a5abfa4f770fe1fd5d3d264ac6591ba8849a1`.
The new `cpp_expression_syntax_metadata_5_0_6.json.gz` corpus retains all 3,571
unchanged source observations. The two earlier corpora remain immutable. One
additional permanent corpus test uses the existing full-metadata comparator.

The [installed source gate](parity/audit_syntax_installed_source_5_0_6.json.gz)
reexecutes unchanged Rust and freshly compiled Java over **8,252/8,252** inputs:
8,150 complete accepted metadata effects and 102 checked source refusals. All
**114/114** Java tests pass. The
[previous installed-parser control](parity/audit_syntax_previous_installed_parser_control_5_0_6.json.gz)
reconstructs and hash-checks the exact parser used by the successful native
snapshot, then freshly compiles it with the new tests. It preserves all original
outputs, reproduces **1,694** differences and fails the added corpus test.

The [fresh generated caller gate](parity/audit_syntax_generated_compiler_5_0_6.json.gz)
compiles all 261 original generator/model Java sources and **24** original/C++
model pairs. All **72 entity descriptors / 336 property-policy tuples** match,
including valid grouped casts, invalid members/overflow, numeric field labels,
boxed patterns and both valid late recovery and rejected attributes. Native
execution disables default PMR, actually deletes each UserContext and requires
zero remaining User bytes before proceeding.

The [current artifact gate](parity/audit_syntax_artifact_closure_5_0_6.json.gz)
regenerates all 25 example, 50 contract and five independent SDK destinations,
then repeats all three strict checks. Every implementation byte stays identical;
exactly 80 provenance records change, each permitting only the literal-parser
source hash update. Original references and historical SDKs stay unchanged.
Source, compiler and artifact temporary directories are removed after terminal
execution. No C++ runtime function, module export, ownership or dependency changes.

The [complete runtime source cohort](runtime_core_source_closure_5_0_6.md)
independently advances source counts to **122 accepted / 0 pending /
2 excluded_external**. Generated behavior decisions, full grammar/provenance
and capability/phase exits remain separate; **11 capabilities are qualified and
6 partial**. The earlier prototype counts and `installed=false` receipts below
remain historical, rather than being rewritten after installation.

## Prototype checkpoint

The references remain runtime `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and
generator `45e1572c4956ca963410d8ea436ead3c8857087a`. Every observer executes the
unchanged complete attribute parser with the original locked `syn` 2.0.118.

## Cast boundaries

The [897-case original/current probe](parity/audit_expression_cast_postfix_probe_5_0_6.json.gz)
finds **306** differences. It combines thirteen type forms with member/method/
await/index/call/try suffixes and valid infix controls, both directly and after
parenthesizing the cast, in three expression contexts. Every difference is an
unparenthesized cast followed by a prohibited suffix. Rust retains the earlier
limit 17; Java incorrectly continues to the later limit 29 and table settings.

The original `check_cast` rejects these suffixes after a type. The candidate
adds that check after the existing type parser; grouped casts keep their legal
postfix path. The [isolated replay](parity/audit_expression_cast_postfix_candidate_5_0_6.json.gz)
matches **897/897** fresh original outcomes. Its
[existing-corpus gate](parity/audit_expression_cast_existing_corpora_candidate_5_0_6.json.gz)
also matches **4,681/4,681** and passes **113/113** Java tests.

## Member and pattern boundaries

The [initial member probe](parity/audit_expression_member_probe_5_0_6.json.gz)
finds **1,026/1,554** differing cases. The
[expanded probe](parity/audit_expression_member_complete_probe_5_0_6.json.gz)
adds explicit boxed-pattern, raw-identifier and numeric-token controls and finds
**1,068/1,650** differences. These are cases arising from shared parser gaps,
not 1,068 independent runtime defects.

The source Member/Index and struct-pattern parsers require these distinctions:

- Reserved words and `_` cannot become ordinary named members. Postfix `await`
  has its separate expression branch; valid raw identifiers remain valid.
- Tuple indices are unsuffixed **u32** literals. Valid underscores and nondecimal
  integer forms must parse; overflowing indices and suffixes must reject.
  This is a source syntax bound independent of host pointer width.
- Expression trailers can split a float token into multiple tuple indices and
  retain its trailing dot. Struct field labels require a single integer token.
- Struct patterns support the source's `box`, `ref` and `mut` prefix order.
  Prefixes do not admit a numeric member, reserved name or explicit field alias.

The candidate reuses the existing identifier and numeric token readers, adds
bounded index validation and preserves the distinct pattern/expression paths.
Its [member replay](parity/audit_expression_member_candidate_5_0_6.json.gz)
matches **1,650/1,650**, including the existing checked raw-identifier refusals.
The [same-candidate regression gate](parity/audit_expression_member_existing_corpora_candidate_5_0_6.json.gz)
passes all **4,681** earlier observations and **113** Java tests.

## Combined proposal and installation gate

The [combined replay](parity/audit_expression_postfix_combined_candidate_5_0_6.json.gz)
recompiles the same candidate and matches **2,547/2,547** fresh original outcomes.
Every original input/output agrees with the preceding probes. Complete candidate
source and hashes are retained; temporary overlay/class/Cargo directories are
removed after their processes exit. No existing expected result is rewritten.

The [regression proposal](parity/audit_postfix_regression_proposal_5_0_6.json.gz)
retains those source observations for a future permanent corpus. Installation
must wait for the frozen native epoch to terminate. It then requires permanent
Java/source regression, fresh generated descriptor/privacy callers, regenerated
artifacts and strict reproduction checks. The current producer still has the
documented postfix discrepancies; this prototype is not relabeled as its result.
No runtime C++, module export, ownership or dependency changes are proposed.

## Outermost attribute paths and recovery

The [1,024-case attribute probe](parity/audit_expression_outer_attributes_probe_5_0_6.json.gz)
finds **320** more current-producer differences. Root expressions treated their
attributes as opaque bracket groups, admitting empty/invalid paths, reserved
keywords and attributes nested directly inside name-value payloads. The actual
source requires an attribute path distinct from a nested-meta path: `unsafe`
has a special outermost branch; ordinary qualified paths retain their own
identifier rules. Arbitrary-keyword nested-meta parsing remains separate.

Recovery order also matters. The locked source can parse a Meta value while
leaving tokens in its bracket buffer. Its outer nested-meta caller ignores the
eventual error after applying later settings. Thus retained examples such as
`#[foo extra]`, `#[foo=1,]` and `#[unsafe::foo]` continue to the later assignment.
The candidate preserves those observed effects instead of demanding an earlier
EOF. Invalid actual paths and directly nested attributes still stop parsing.

The [attribute candidate](parity/audit_expression_outer_attributes_candidate_5_0_6.json.gz)
matches **1,024/1,024** fresh original results. Its
[same-source existing regression](parity/audit_expression_outer_attributes_existing_candidate_5_0_6.json.gz)
passes **4,681/4,681** and **113/113** Java tests. This includes the postfix/member
correction; the installed producer remains unchanged.

The [final combined candidate](parity/audit_expression_syntax_combined_candidate_5_0_6.json.gz)
matches **3,571/3,571** fresh original results across postfix, member and
attribute groups. Every original observation agrees with its preceding receipt.
Together with the existing regression, the same candidate source passes **8,252**
observations in the two declared gates. This is not an installed permanent
8,252-case gate or a complete AP02 verdict.

The [latest regression proposal](parity/audit_expression_syntax_regression_proposal_5_0_6.json.gz)
supersedes the earlier 2,547-case proposal for the next installation. It retains
all 3,571 source expectations, the complete candidate source receipt and matching
hashes. The candidate source is available as `candidate_parser.source` in the
combined receipt. Installation, permanent corpus/test updates, fresh generated
callers and strict regeneration remain deferred until the native input freeze
ends. Every temporary overlay and compiler directory has been removed.

An initial overlay-controller indentation error occurred before any compiler
ran. The corrected controller was syntax-checked before execution; the failed
owned directory was removed. This setup failure is separate from parser results.

At the prototype checkpoint, source totals were **118 accepted / 4 pending_review /
2 excluded_external**, with **11 qualified / 6 partial** capabilities. The
installed/source-cohort sections above record the later decisions. P0–P7 stays open.
