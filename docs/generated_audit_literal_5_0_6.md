# Generated audit-length literals: Rust 5.0.6

Status: the sixteen initially reproduced valid-input differences are repaired.
The subsequent [nested metadata correction](audit_nested_metadata_5_0_6.md)
also repairs the recorded repeated settings, later overflow, raw mask override
and numeric/comment rejection cases. Complete G14/P7 qualification remains open,
including broader container/expression boundaries and the unanswered wire-naming
behavior decision. The earlier checkpoint below retains its original scope.

The references remain Rust `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`
and generator `45e1572c4956ca963410d8ea436ead3c8857087a`. The
[initial parser comparison](parity/generated_audit_literal_parser_initial_5_0_6.json.gz)
executes the complete unchanged original `teaql-macros/src/attr.rs`. It exposes
sixteen differences in 59 inputs, rather than inferring behavior from template
text or expected Rust primitive types.

## Shared generator repair

`CppFieldLayoutMetadata.auditValueMaxLen` now delegates to one Java literal
adapter. It preserves integer radix, separators, suffixes, unsigned width and
decoded-string behavior, and adds the original literal boundaries:

- Unicode identifier suffixes follow the locked `unicode-ident` 1.0.24 data,
  including combining continuations and Unicode 17 letters absent from the host
  JDK's identifier classification.
- Decimal exponents such as `1e2` and `1e_2` do not become a limit of one.
  Suffixes such as `f32`, `f64`, `f16` and `f128` remain integer metadata because
  that is the observed original `syn` behavior.
- Lexer whitespace, ordinary/nested comments and outer literal attributes retain
  the literal value. Parenthesized, unary, binary, cast, range and postfix
  expressions do not become literal limits.
- Ordinary quoted values reuse the existing Rust-string decoder. Raw values
  retain their payload. Decoded whitespace, embedded NULL and unsigned overflow
  keep their original no-limit result.

The original nested-meta parser ignores a trailing-token error after its callback
records a value. Controls such as `17 trailing` preserve that result; the adapter
does not silently replace this boundary with a whole-string integer parser.
This is not a claim to implement the complete Rust expression/token grammar.

The adapter and tables run in the existing Java generator. They add no C++
runtime dependency, ownership domain, public API or runtime privacy algorithm.
The Unicode tables contain 691 start ranges and 806 continuation ranges; binary
search avoids depending on the host JDK's Unicode version. Original dependency
license notices are included in the generator resources.

## Earlier literal checkpoint

The [installed-adapter comparison](parity/generated_audit_literal_parser_repaired_5_0_6.json.gz)
retains all 59 initial inputs and compares 445 total stimuli against the complete
original parser. All **444 valid-source inputs match**. The remaining stimulus,
`0x11a` followed by a combining character, is rejected by the original lexer but
produced no limit in the C++ generator at this checkpoint. It remained an explicit
source-rejection discrepancy, rather than a passed comparison or permission to
weaken the G14 scope.

The [separate remaining-boundary observations](parity/generated_audit_literal_remaining_5_0_6.json.gz)
also retain repeated `audit_value_max_len`/`audit_value_max_len_int` overrides,
an overflowing later override and unterminated comments. The original parser's
later assignment or lexical rejection was not reproduced by that adapter.
Those source-observed differences required the subsequent nested metadata
correction; the 444 matching inputs did not qualify them.

Both the original Unicode helper and the actual installed Java helper enumerate
all **1,114,112 code point positions**, including invalid scalar positions, for
each of XID_Start, XID_Continue and lexer whitespace. The resulting ranges match
exactly. Source identities, original parser/driver, dependencies, Java sources
and outputs are retained in the receipt.

The selected Java gate passes **79/79** tests: the complete C++ generation test
class, field-layout and JSON-input tests, and four new source-derived literal
contract tests. The
[table extraction receipt](parity/audit_literal_xid_tables_5_0_6.json.gz)
also records the unchanged runtime lock and exact registry identities.

Reproduce the source-data check with the activated toolchain and existing Rust
oracle dependencies:

```sh
source scripts/toolchains/activate.sh
python3 scripts/verify_audit_literal_xid_5_0_6.py
```

`--write` regenerates only the Java table arrays from that source. Rust remains
verification tooling; ordinary generation and deployed applications do not invoke
it.

The [complete generated/compiler comparison](parity/generated_privacy_literal_repaired_5_0_6.json.gz)
passes **10/10** Rust/C++ pairs: two ASCII controls, four Unicode suffixes, an
exponent, a commented integer, Unicode whitespace and an outer literal attribute.
Every original and C++ compilation exits zero. It compares **30** descriptors
and **140** policy tuples, including authentic identities, masks and declared
lengths. Original producer/entity/derive paths are unchanged; the earlier six
original outputs remain available in the initial receipt.

The 26 added literal controls are now available to the existing compiled SDK
auditor. Select named cases instead of implying that every parser stimulus has
already passed a complete generated/compiler run:

```sh
python3 scripts/audit_generated_privacy_contracts_5_0_6.py \
  --cases unicode_integer_suffix unicode_string_suffix unicode_raw_suffix \
          combining_suffix exponent comment_before source_whitespace_nel \
          literal_attribute \
  --require-match --output /tmp/audit_literal_compiled_replay.json.gz
```

The [terminal validation](parity/generated_audit_literal_validation_5_0_6.json.gz)
also preserves **24** ordinary generated workspaces/**1,583** files and **50**
complete contract SDK reproductions. The strict contract replay reports zero
generated code or Assist changes. Ordinary manifests and contract provenance
now fingerprint both shared literal helpers. All six original compiled fixtures'
source files and results remain byte-identical to the initial failure receipt.
Every runtime/SDK C++ file from the frozen native epoch remains unchanged except
the independently repaired test assertion. Native rebuild reports no work.
Neither these comparisons nor the Java controls close AP02/AP03 or promote G14.
