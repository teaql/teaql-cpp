# Source-shaped scalar sample assignment

The runtime now separates conversion into native model storage from retaining
the original mutation operand. Generated sample recipes delegate incompatible
operand/storage combinations to this shared mechanism. Public typed setters,
Context lifetimes, query execution arguments and persistence layers are unchanged.

The reference remains Rust 5.0.6 `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`.

## Reproduced problems

`bool(isNumber=true)` selects the original numeric sample branch. Rust retains
the I64 mutation operand but cannot convert it into Boolean storage. Required
getters retain `false`; optional getters retain their prior empty state. The
previous C++ generator used `static_cast<bool>`, producing `true` instead.
[Actual sample execution](parity/sample_operand_bool_samples_initial_5_0_6.json.gz)
records **24 differing field results** across ordinary/inherited and
required/optional models.

Other configurations, including integer/decimal/float fields with `isString=true`
and date fields with `isNumber=true`, did not compile as a public C++ SDK.
The [complete original/C++ reproduction](parity/sample_operand_cross_initial_5_0_6.json.gz)
retains the generated sources, successful original entity compilation and C++
compiler diagnostics. This is a runtime adaptation requirement, not a reason to
weaken the generated public typed interface.

The previous JSON sample branch also retained Text mutations without updating
native JSON storage. The locked source setter converts that native view to a
JSON string. Numeric operands produce native JSON numbers. Both views must
coexist with the unchanged original mutation operand.

## Runtime contract

`detail::EntityRuntimeAccess::update_scalar_value` uses the existing source-shaped
Value conversion methods. Successful conversion updates native storage; failed
conversion keeps its previous value. The mutation ledger always receives the
original Value, including its kind and unnormalized text. Text native storage
uses the established trimming rule; original text is retained separately.

Untyped NULL clears optional storage. Typed NULL remains a distinct operand:
ordinary optional scalars retain storage, while optional JSON obtains a present
JSON null. Required JSON converts every Value through the existing JSON view.
Changing the mutation ledger later must not discard successfully converted
native storage.

The adapter rejects a foreign operand, key, or allocator-bearing native storage
before mutation. Plain primitive references do not carry allocation identity;
the generated befriended recipe supplies its own model member. Explicit copies
into the destination Context permit transfer. Nested strings/JSON allocate in
that destination. No singleton, new ownership domain or implicit lifetime
extension is introduced.

`GeneratedSampleValues::assign_operand` delegates conversion/ledger work and
marks only the assigned field loaded. Existing runtime function bodies remain
byte-identical after removing the newly inserted helper blocks. This is a kernel
mechanism followed by thin generator adapters.

## Executed qualification

- [Native kernel](parity/sample_operand_kernel_native_final_5_0_6.json.gz),
  [ASan/UBSan kernel](parity/sample_operand_kernel_sanitize_5_0_6.json.gz) and
  [module-import kernel](parity/sample_operand_kernel_modules_final_5_0_6.json.gz)
  exercise conversion, storage retention after ledger clear, NULL distinctions,
  nested allocation, foreign-domain refusal, loaded state and actual Context
  destruction with the default PMR resource disabled.
- [Mixed scalar descriptors](parity/sample_operand_cross_fixed_5_0_6.json.gz)
  and [actual scalar execution](parity/sample_operand_cross_samples_final_5_0_6.json.gz)
  retain exact original/C++ effects and **144 field outcomes**.
- [Raw JSON descriptors](parity/sample_operand_json_fixed_5_0_6.json.gz)
  preserve **eight explicit approved type differences** across four callers.
  The [approved JSON exception](json_payload_compatibility_exception_5_0_6.md)
  keeps C++ JSON descriptors; this is not an exact descriptor-match claim.
  [Actual JSON scalar execution](parity/sample_operand_json_samples_5_0_6.json.gz)
  compares **288 field outcomes** exactly, including original operand kind/value,
  native getters and field load state.
- [Java regression](parity/sample_operand_java_5_0_6.json.gz) passes **141 tests**,
  including the 63,918 retained metadata parser contracts and the new source
  configurations. Original parser results reuse unchanged locked identities;
  there is no new full Rust parser run.
- [SDK scope controls](parity/sample_operand_reproduction_controls_5_0_6.json.gz)
  regenerate both exact preceding sample files with the captured old producer,
  verify their original hashes and reject ten unrelated edits. Only JSON sample
  delegation changes are allowed; all other implementation bodies remain exact.
- [Relevant native regressions](parity/sample_operand_native_5_0_6.json.gz)
  pass **6/6**, including the actual SQLite sample-field and clock-composition
  comparisons, original operands, entity runtime and Context sample state.
- [All SDKs](parity/sample_operand_sdk_reproduction_5_0_6.json.gz) reproduce
  **80 workspaces / 3,560 files**. Only the sample assignment in
  `generated_sample_fields_runtime` and `generated_checker_clock_composition`
  changes to a shared JSON delegate. Models, queries, metadata, application
  signatures and the remaining SDK bodies retain their previous bytes.
- [Pinned format/check](parity/sample_operand_format_5_0_6.json.gz) passes
  without further C++ changes. There is no new full native, imported or
  sanitizer-suite acceptance claim.

Scalar execution uses unchanged generated Rust statements/blocks and actual C++
recipes. It is a deterministic field-level gate; wall-clock sample statements,
relationship selection and whole governed Save are separate responsibilities.

## Verification protocol

The sample observer now returns failure when its post-run input guard detects
drift. A [protocol-only negative control](parity/sample_operand_input_guard_control_5_0_6.json.gz)
mocks commands with retained valid results, changes the reported controller hash
and verifies refusal. It claims no compiler or runtime execution. The incomplete
earlier cross-sample receipt is retained; the final stable run repeats execution.

All one-off source compilation runs use owned temporary directories and remove
their binaries/targets after retaining compact sources, commands and results.
Stable native, module and sanitizer preset outputs remain in their existing
directories. Final leak scanning is disabled; no MIPS/device run is claimed.
All ten recorded task-owned compiler/SDK probe directories have been removed.
Measured removals total **1,059,700,658 bytes**; two failed probes and the regular
generation/protocol-control contexts were removed without size measurements.

The four changed preceding C++ inputs are the two runtime headers and two
generated sample headers. A new kernel test brings the active C++ input count
to 1,814. The other 1,809 preceding C++ inputs remain byte-identical. Historical
native893 evidence retains its original full input epoch.

G13/G14/G17 and P0–P7 remain open. These field consumers do not qualify complete
delegated grammar, every model configuration, source provenance or protected
publication. The core source corpus count remains 142 / 155,380 observations.
