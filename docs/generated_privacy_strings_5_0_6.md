# Audit length string literals: Rust 5.0.6

Status: both original/native gaps are repaired. The ten added string cases
match actual original derives and native execution. The combined replay of all
33 cases and current workspace/SDK reproduction gates pass.
G14/P1/P7 remain open.

The locked original derive in teaql-macros/src/attr.rs accepts a LitStr, obtains
its semantic value and parses usize. The previous C++ adapter inspected the
literal's unprocessed text. Ordinary quoted digits worked, while valid escapes
and raw string syntax silently omitted the declared limit.

The [initial comparison](parity/generated_privacy_string_initial_5_0_6.json.gz)
contains complete original and C++ generated trees and successful compilations.
For both `"\x31\x37"` and `r"17"`, the actual original Item descriptor has
Some(17), while the old C++ descriptor has no limit. That is a metadata behavior
gap; source file inventories or textual attribute checks would not detect it.

The C++ adapter reuses CppJsonInputMetadata.decodeLiteral for ordinary quoted
strings. Raw strings retain their payload, including backslashes. Optional literal
suffixes remain separate from the semantic value. The decoded value is never
trimmed: whitespace, embedded NULL, invalid digits and usize overflow preserve
the original parse-to-None behavior. Existing integer radix/underscore/suffix and
target-width handling remains unchanged. No runtime privacy algorithm, Context
allocation or dependency is added.

The ten configurations include ASCII byte and Unicode escapes, embedded NULL,
quoted whitespace, plain and hash-delimited raw strings, raw backslashes,
overflow and ordinary/raw suffixes. The observer imports complete original
Platform/Item/Child entity files unchanged, executes the original derives and
field-policy classification, and compares actual compiled C++ descriptors.
The output retains table/service, masks/declaration state, limits and property/
column/policy tuples without normalization.

```sh
set -e
source scripts/toolchains/activate.sh
python3 scripts/audit_generated_privacy_contracts_5_0_6.py \
  --cases quoted_hex_escape quoted_unicode_escape quoted_null quoted_whitespace \
          raw_string raw_hashed_string raw_invalid_backslash raw_quoted_overflow \
          quoted_suffix raw_suffix \
  --require-match --output /tmp/privacy_string_replay.json.gz
```

Use a fresh output path; retained evidence is never overwritten. Dependencies
and registry checksums must match the reference Cargo.lock. See the earlier
[compiled metadata review](generated_privacy_contracts_5_0_6.md) for the prior
23 cases, original model-mask semantics and target-size boundary.

## Verified repair and current reproduction

The [repaired comparison](parity/generated_privacy_string_repaired_5_0_6.json.gz)
contains **10/10** matching compiled pairs, **30** actual entity descriptors and
**140** field-policy tuples. Both initial cases retain all 26 original files
byte for byte and the same original outputs. The only behavior change is in the
C++ adapter's interpretation of audit-length string literals.

Java generator/field-layout/JSON-input tests pass **53/53**. All **23 workspaces /
276 files** reproduce at the current producer implementation. All **44** complete
contract SDKs strictly reproduce; the regeneration report confirms no generated
code or guidance byte changed. The unchanged earlier metadata corpus also
replays **12 models / 48 entity records**. The pinned formatter checks **1407**
handwritten C++ files. Rust and generator references remain clean and locked.

The [combined replay](parity/generated_privacy_string_full_replay_5_0_6.json.gz)
passes **33/33** actual compiled pairs, **99** descriptors and **462** policy tuples.
It replays the earlier 23 cases and the ten added string cases together. All
original files and outputs for the 23 prior cases and two initial failing cases
remain unchanged. These are source comparisons, not a claim of complete runtime
privacy acceptance.

The [terminal record](parity/generated_privacy_strings_5_0_6.json),
[validation logs](parity/generated_privacy_strings_validation_5_0_6.json.gz) and
[Java results](parity/generated_privacy_strings_java_5_0_6.json) retain the
actual outcomes and reproduction scope.

No runtime, public module interface or lifetime algorithm changed in this
repair. The original/native observer compiles and executes the complete C++
facade for each input. No additional sanitizer, module, MIPS or physical-device
run is claimed for this checkpoint.

This checkpoint does not accept arbitrary Rust expressions, invalid lexical
syntax, all generated model configurations or complete governed/protected-I/O
privacy integration. Those responsibilities remain under G14 and its existing
runtime/integration owners. Generation remains a development step; ordinary
native applications do not depend on Java or the source oracle.
