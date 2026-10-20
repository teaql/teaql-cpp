# Character and byte-character metadata lexing: Rust 5.0.6

Runtime reference: `fe82a18982415f1746df9a2cf7c42f5e9abdc6d3`.
Generator reference: `45e1572c4956ca963410d8ea436ead3c8857087a`.
This is a bounded AP02/G14 correction. It does not qualify the complete Rust
expression, type, pattern, body or lexical grammar.

## Original difference and responsibility

The [unchanged original/current witness](parity/character_lexical_source_witness_5_0_6.json.gz)
executes 25 inputs and reproduces 13 differences. Empty/multiple-character
literals, invalid escapes, surrogate/out-of-range Unicode values and non-ASCII
byte characters reach subsequent metadata effects in C++, whereas Rust rejects
the complete token stream before applying any effects.

The exact reference uses `proc-macro2 1.0.106`. Its `parse.rs` separates
`character`, `byte`, `literal_suffix`, `backslash_x_char`, `backslash_x_byte` and
`backslash_u`. Character hex escapes are ASCII; byte escapes span all 256 values.
Unicode escapes permit one to six hex digits and internal/trailing underscores,
and require a valid Unicode scalar. Byte characters do not accept Unicode
escapes. One literal payload and its closing quote are required.

The candidate follows these source responsibilities and retains literal
suffixes. Whole-stream validation runs before metadata interpretation. Lifetime
tokens stay distinct from character literals, and complete identifiers prevent
a final `b` from becoming an accidental byte prefix. Expression, closure-default
and pattern parsing share the same character handling. No expression is evaluated.

## Staged evidence

The [first candidate](parity/character_parser_candidate_5_0_6.json.gz) repairs
the lexical differences but incorrectly rejects 16 valid token streams when a
lifetime appears where expression parsing should simply stop. A separate
[lifetime correction](parity/character_parser_candidate_lifetimes_5_0_6.json.gz)
matches 2,003/2,003 controls. The
[expanded replay](parity/character_parser_candidate_boundaries_5_0_6.json.gz)
matches **3,147/3,147**, including 1,763 unchanged retained controls and 1,384
new witness, ASCII/byte/Unicode/escape/suffix/boundary controls. These include
literal, macro, closure body, const-default and pattern positions.

Three new handwritten Java tests retain all six existing complete test classes.
The [corrected test replay](parity/character_parser_candidate_java_replay_5_0_6.json.gz)
passes **107/107**. Earlier unsuccessful harness attempts are preserved: a
missing staged test file and a run from the wrong resource directory. The latter
runs all 107 tests but cannot locate three existing fixtures; running from the
generator directory resolves those fixture failures without source changes.

The [staged patch](parity/character_parser_staged_patch_5_0_6.json.gz) retains
both complete original/candidate Java files, their hashes and diffs, and all
1,384 new inputs. Installation and actual-producer qualification are separate
from these staged results.

## Installed producer

The [installation receipt](parity/character_parser_installation_5_0_6.json.gz)
checks both original/candidate file hashes before installing the two Java files
and retained input family. Actual Maven executes all six complete classes and
passes **107/107**; the
[installed source comparison](parity/character_parser_installed_comparison_5_0_6.json.gz)
matches **3,147/3,147**. Commands, complete output and actual Surefire reports are
retained in the [installed gate record](parity/character_parser_installed_gates_5_0_6.json.gz).
The [SDK reproduction](parity/character_parser_sdk_refresh_5_0_6.json.gz)
passes for all **78** active SDKs. Every existing code/guidance/data byte remains
unchanged, as do all **2,201** protected artifacts; only producer fingerprints
refresh. The installed gate finishes successfully, including generator provenance
verification. Compiled consumer and full native gates remain separate requirements.

The additional [compiler driver](../scripts/audit_character_metadata_contracts_5_0_6.py)
generates seven complete original/C++ models for Unicode characters, character
and byte escapes, byte patterns and const defaults. Each must retain the later
limit of 21. Its [compiler gate](parity/character_parser_compiler_gates_5_0_6.json.gz)
also replays the eleven existing container descriptor and ten typed-JSON cases,
checking frozen execution inputs, both references and all six runtime archives.
An incomplete gate record is not a passing result.

That compiler gate is now terminal and passes **28/28**: seven new character
models, eleven retained container descriptor cases and ten typed-JSON/compiler
cases, including the retained unqualified-foreign-type rejection. All seven
character cases produce the actual original limit of 21. Every captured execution
input, both references and all six native archives remain unchanged throughout.
The separately recorded [full native epoch](native_integration_5_0_6.md) has
started; it is not a completed integration result.

## Acceptance boundary

The candidate changes only the independent generator's metadata parser and its
handwritten tests. No runtime ownership, SQL, privacy algorithm, generated API or
runtime dependency changes. The complete current native gate, source promotion,
G14/G13/G17 qualification and P0-P7 completion remain open. No sanitizer, MIPS or
physical-device result is claimed.

## Reproduction

Use fresh output paths and the locked toolchain:

```bash
source scripts/toolchains/activate.sh
metadata_args=()
for family in operands grammar_next paths delimiter_literals patterns pattern_boundaries closures characters; do
    metadata_args+=(--extra-cases "tests/parity/audit_expression_${family}_5_0_6_inputs.json")
done
python3 scripts/verify_container_metadata_5_0_6.py "${metadata_args[@]}" \
    --require-match --output /tmp/teaql_character_metadata_replay.json.gz
```

This executes the original source and installed producer over all 3,147 inputs.
SDK regeneration, compiler/JSON and native integration are additional gates,
not implied by this parser comparison.
