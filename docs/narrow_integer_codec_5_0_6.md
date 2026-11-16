# Narrow integer entity decoding, Rust 5.0.6

## Confirmed kernel gap

The direct derive experiment compiles and executes the unmodified locked
`teaql_macros::TeaqlEntity` implementation with four required and four optional
fields: i16, i32, u16 and u32. Across 376 inputs, the pre-repair C++
GeneratedScalarCodec differs on 272 integer cases. It rejects valid integer
values and supplies generic invalid-value errors where the original derive
provides checked width/range diagnostics.

The C++ Value representation already preserves signed and unsigned inputs in
I64/U64 carriers. The missing responsibility is the conversion into the model's
declared narrow C++ type. Neither a new Value variant nor a new allocation
domain is required.

## Implementation plan

1. Extend the kernel GeneratedScalarCodec for exactly std::int16_t,
   std::int32_t, std::uint16_t and std::uint32_t. Check representability before
   casting either I64 or U64, with the original source/target range messages.
   Preserve the existing 64-bit conversion branches and their Decimal rules.
2. Retain missing/untyped NULL defaults, optional NULL, explicit TypedNull
   rejection and rejection of non-integer inputs. Continue using the caller's
   existing Memory resource for results and diagnostics.
3. Rebuild and execute both observers against all 376 inputs, comparing complete
   values and EntityError entity/message fields. Exercise min/max boundaries,
   adjacent out-of-range values, negative-to-unsigned conversion and I64/U64
   extremes. Keep the pre-repair observations separate from repaired evidence.
4. Run affected native contracts, exact legacy-source preservation and ordinary/
   optimized receipt rejection checks. Format handwritten C++ with the pinned
   tool and remove owned disposable outputs after retaining compact evidence.
   Do not promote G13/G14/G17 or claim complete P0-P7 acceptance.

## Distinct generator boundary

The complete generator experiments do not establish an accepted narrow-integer
configuration. The first custom declaration uses an absent parent type and both
independent evaluators reject it. After selecting the registered integer parent,
all six complete original libraries fail compilation: optional configured
carriers conflict with String-based generated update code, and the required
case also conflicts with generated defaults. These are retained original
refusals; they are not C++ mappings or newly approved compatibility exceptions.

The kernel experiment uses ordinary primitive field tokens in hand-written
derive fixtures. Its first probe forwarded a Rust `:ty` fragment through a
macro, introducing an opaque type group that the reference macro did not
recognize. The corrected probe forwards token trees, preserving ordinary
primitive/Option syntax for the authentic derive. The initial compiler failure
is retained; no reference source or generated SDK is edited.

## Executed repair

The [completed diagnosis](parity/narrow_integer_codec_diagnosis_current_5_0_6.json.gz)
contains 272 differences: 112 valid integer values were rejected and 160
out-of-range inputs received different errors. The other 104 missing/NULL and
non-integer outcomes already matched. The earlier probe compiler failure is
retained separately in `narrow_integer_codec_diagnosis_5_0_6.json.gz`.

The kernel now checks the four narrow integer targets with C++20
`std::in_range` before casting I64/U64 input. The existing 64-bit and all other
conversion branches remain byte-identical. The
[legacy source proof](parity/narrow_integer_codec_legacy_source_5_0_6.json.gz)
removes only the new, disjoint constexpr branch and utility include and recovers
the exact preceding header.

The [repaired actual execution](parity/narrow_integer_codec_current_5_0_6.json.gz)
rebuilds the original derive and native observer and matches all 376 complete
value/error observations. Both required and optional i16/i32/u16/u32 fields
retain the original entity and error strings. The native observer closes each
UserContext and verifies that its arena blocks are released. Its boundary
corpus is extracted from those actual original observations, rather than
calculated from C++ behavior.

The permanent `narrow_integer_codec_rust_parity` CTest target passes its complete
376-case comparison in the
[native seed](parity/narrow_integer_codec_native_seed_5_0_6.json.gz). The
comparator binds complete corpus bytes, original fixture bytes, exact input
membership, the baseline and the original execution receipt. Rebuilding the
affected runtime archive leaves every recorded native runtime/compiler/SQLite
input hash unchanged.

## Source admission and regression qualification

The [source admission module](../scripts/narrow_integer_source_admission_5_0_6.py)
binds the actual execution, exact legacy-byte proof, installed CTest and six
original generator refusals. Directed source transitions preserve the executed
case generation, commands and comparison bodies. The preceding Business ID
module delegates only its four source-matching functions; its existing controls
and other responsibilities remain unchanged.

The [first compatibility run](parity/narrow_integer_compatibility_5_0_6.json.gz)
passes 51 controls per Python mode. A separate
[diagnosis](parity/narrow_integer_reader_false_acceptance_5_0_6.json.gz)
then reproduces three reader defects: an empty dependency inventory, a substituted
linker and a substituted native test command are incorrectly accepted. Their
repair requires the exact actual dependency lock and linker identity, and the
complete native CTest command. The prior reader bytes and results remain intact.

The [repaired compatibility run](parity/narrow_integer_compatibility_repaired_5_0_6.json.gz)
accepts the completed execution and rejects these corruptions under ordinary
and optimized Python:

| Source/receipt responsibility | Rejected controls per mode |
| --- | ---: |
| Narrow-integer source admission | 13 |
| Actual derive/native execution receipt | 18 |
| Preceding Business ID admission | 12 |
| Preceding scalar admission | 13 |
| Total | 56 |

The three independently reproduced reader regressions also reject in each mode.
These checks reuse the completed original/native execution; they do not claim
another Rust build or another 376-case execution.

The [affected native gate](parity/narrow_integer_native_5_0_6.json.gz)
passes all seven registered tests and required fixtures, with exact registry/
JUnit membership and no failures, errors or skips. The selected final build has
no work, and all recorded runtime archive/compiler/SQLite hashes stay unchanged.
The [retention check](parity/narrow_integer_retention_5_0_6.json.gz) verifies
unchanged bytes for all eighty SDKs, their eighty manifests and 3,560 code/Assist
files. It also verifies the clean, unchanged producer and recorded C++ inputs;
it is a hash check, not fresh SDK regeneration.

The [final checks](parity/narrow_integer_checks_5_0_6.json.gz) pass the pinned
formatter write/check for 1,817 handwritten C++ files, the locked upstream and
module-path verifiers, and Git whitespace validation. The recorded execution
inputs and references remain unchanged. All twenty-two recorded disposable
paths are absent; the earlier executions measured 1,707,123,186 bytes removed.
Compact evidence and the permanent source/corpus fixtures are retained.

The complete generator refusal experiments remain in
`compiler_integer_carrier_diagnosis_declared_5_0_6.json.gz`. This closes the
bounded kernel repair and its verification boundary. Generated narrow-integer
SDK support, complete configuration mappings, pending identity/column decisions,
G13/G14/G17 and full P0-P7 remain open. No compatibility exception or capability
status is added. No sanitizer, MIPS or physical-device execution is claimed.
