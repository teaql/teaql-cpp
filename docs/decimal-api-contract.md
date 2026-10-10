# Supported Decimal API contract

Baseline: TEAQL Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
reexporting the locked `rust_decimal` 1.42.1 `Decimal`. The workspace requests
`serde`; the dependency's default features are `serde` and `std`. Optional maths,
database, macro, ndarray, rkyv and alternate serde representations are not enabled
by this contract. This boundary follows the pinned feature configuration, rather
than removing capabilities because their tests are inconvenient.

This is a responsibility mapping and work queue. It is not a complete acceptance
verdict. Source inspection includes `decimal.rs`, `error.rs` and the enabled
default branches of `serde.rs`; arithmetic implementation and formatting evidence
are recorded separately. A method declaration or a large corpus cannot replace
the corresponding behavior, ownership and composition review.

The final enabled Decimal source contract is subsequently accepted as part of
the value.rs/time.rs group after the complete body/default review and current
integration below. Final no-work native verification passes 303/303 in 74.81
seconds with complete dependency records. The 21-check combined model selection
and separate 13-check scalar/memory selection pass on sanitizer/MIPS. Later
historical paragraphs retain the failures and gates as they stood during work;
see value-api-acceptance.md for the final grouped verdict and exact limitations.

## Pointer-width and constant-evaluation follow-up, 2026-10-08

The earlier numeric matrix exercises fixed-width integer conversions, while
isize/usize have only a small native boundary assertion. A separate test-only
oracle now invokes the unchanged locked Decimal ToPrimitive, TryFrom,
FromPrimitive and From implementations in actual 64-bit native Rust and actual
32-bit ARMv7 musl Rust under QEMU. Its isolated Cargo manifest/lock use only
versions and checksums already present in the existing oracle. No C++ production
dependency is introduced, and this ARM execution is not Rust execution on MIPS.

For each word width, 238 inputs retain optional extraction, full TryFrom error
Display/Debug, pointer-sized input rejection, construction, raw sign/scale/word
state and unchanged caller values. Inputs cover both zero signs, fractional
truncation, 32/64-bit integer boundaries, maximum coefficients and retained
extended scales. Recorded observations are checked twice against each actual
Rust executable. Native C++ matches all 238 live 64-bit observations; MIPS C++
matches all 238 live 32-bit observations without output normalization. The new
pointer comparison is registered in CMake.

A separate compile fixture reproduces missing C++ constant evaluation for the
source's allocation-free const-fn entries. from_parts, scale, zero/sign reads,
mantissa, unpack, min/max bounds and sixteen-byte serialization now support
constexpr use. Construction and serialization move into the header with their
existing runtime behavior, invalid-scale optional rejection and negative-zero
normalization unchanged. Both actual Rust word-width builds compile original
constant construction/read/serialize assertions; native assertions additionally
check all bytes, bounds, raw fields and portable signed mantissa.

Explicit-domain Result/Error factories retain their existing scoped allocation
adaptation; this repair does not make Context-owning result carriers compile-time
objects. No arithmetic, rounding policy, object representation, ownership,
generator or production dependency changes. Fresh Decimal platform selections
and the full current native consumer rebuild are running. The previous 302/302
checkpoint precedes these header changes and is historical evidence. Complete
Value/time/Decimal and stage acceptance remain open.

Reproduce the actual source word-width observations with the existing installed
ARMv7 musl Rust standard library, cross linker and QEMU:

```sh
CARGO_TARGET_DIR="$PWD/build/rust-decimal-pointer" cargo build \
  --manifest-path tests/parity/rust-decimal-pointer/Cargo.toml --locked --offline
CARGO_TARGET_DIR="$PWD/build/rust-decimal-pointer" \
  CARGO_TARGET_ARMV7_UNKNOWN_LINUX_MUSLEABIHF_LINKER=arm-linux-gnueabihf-gcc \
  cargo build --manifest-path tests/parity/rust-decimal-pointer/Cargo.toml \
  --target armv7-unknown-linux-musleabihf --locked --offline
python3 tests/parity/generate_decimal_pointer_scope.py \
  --rust64 build/rust-decimal-pointer/debug/teaql-cpp-decimal-pointer-oracle \
  --rust32 build/rust-decimal-pointer/armv7-unknown-linux-musleabihf/debug/teaql-cpp-decimal-pointer-oracle
```

The comparison runner selects the actual native C++ pointer width and refuses
a mismatched live Rust width. For MIPS, pass its QEMU prefix plus the 32-bit
oracle and `--rust-emulator=/usr/bin/qemu-arm`. Existing arithmetic, conversion,
foundation and formatting corpora remain unchanged.

Fresh Decimal/auxiliary selections pass 13/13 under ASan/UBSan with leak
scanning disabled (64.18 seconds) and 13/13 on OpenWrt MIPS/QEMU
(35.42 seconds). The selected final refreshes report no work, with nonempty
dependency records for all 147 sanitizer and 324 MIPS compiled objects. A final
full-byte static assertion is additionally compiled in the native Decimal
contract; its affected target refresh is tracked separately. Full native
consumer integration remains running before complete source/stage acceptance.

The final full-byte assertion rebuilds and its affected Decimal contract passes
1/1 under ASan/UBSan with leak scanning disabled (0.03 seconds) and 1/1 on
MIPS/QEMU (0.13 seconds). Formatting checks all 609 handwritten C++ files;
baseline, inventory and whitespace checks pass. The latest source code still
requires terminal full native consumer/regression acceptance.

## Public responsibility mapping

The final enabled-source review includes decimal.rs public factories/constants/
traits, arithmetic_impls.rs checked/saturating/reference/inverse forwarding,
all five assignment implementations and both value/reference Sum/Product folds,
error.rs variants/conversions/diagnostics, and serde.rs default visitor and
serialize_str branches. Source feature guards exclude maths, database, macro,
alternate serde and unrelated ecosystem adapters at the pinned configuration.
Inherited num-traits identities/casts and standard Ord defaults are reviewed
separately, rather than assumed from implementing the named parent traits.

The C++ result/domain and throwing-panic adaptations remain explicit. No source
variant, raw sign/scale, mutation boundary, default, formatter or binary/JSON
protocol is removed. Context-owning typed error factories use their selected
allocator; allocation-free scalar copies/constants do not extend Context data.
This review is complete at the responsibility level, but current consumer and
composition acceptance still requires the fresh final-state platform gates.

| Original items | C++ mapping and required behavior |
| --- | --- |
| MIN, MAX, ZERO, ONE, NEGATIVE_ONE, TWO, TEN, ONE_HUNDRED, ONE_THOUSAND, MAX_SCALE | Same named scalar constants. Maths constants are feature gated in the dependency. |
| new, try_new | `from_i64_with_scale` and explicitly scoped `try_new`; invalid scale remains a failure. |
| from_i128_with_scale, try_from_i128_with_scale | Same responsibilities using portable Int128, checked 96-bit coefficient and scale. |
| from_parts | Array-of-three-word factory; preserve word order, scale validation and negative-zero normalization. C++ represents invalid scale with an empty optional rather than a Rust panic. |
| from_scientific, from_scientific_lossy | Separate typed-result factories; preserve exponent parsing, intermediate overflow and rounding boundaries. |
| from_str_radix, from_str_exact, FromStr | Distinct radix/exact/default parsing. Default parsing retains its scientific fallback and typed error; `parse` supplies an optional adapter. |
| array_string | Owning fixed-capacity DecimalInlineString. No Context allocation or heap string substitutes for its inline storage. |
| scale, mantissa, unpack | Complete raw scalar observations, including retained extended scales and negative zero. |
| is_zero, is_integer, is_negative, is_positive, is_sign_negative, is_sign_positive | Separate zero/integrality/sign responsibilities; negative zero still has a negative sign. |
| set_sign, set_sign_positive, set_sign_negative, set_scale, rescale | Keep sign setters and checked/raw scale changes distinct; rescale is not limited to ordinary construction scales. |
| serialize, deserialize | Exact sixteen-byte representation and source flag handling; independent of JSON serialization. |
| min_value, max_value, min, max | Full bounds and numeric ordering. Equal-valued operands can retain different scale/sign representations. |
| trunc, trunc_with_scale, fract, abs, floor, ceil | Independent rounding/decomposition operations; preserve zero sign and retained scale where the source does. |
| normalize, normalize_assign | Value-returning and in-place normalization; numeric equality alone cannot prove raw representation parity. |
| round, round_dp, round_dp_with_strategy, round_sf, round_sf_with_strategy | Keep default ties-to-even and optional significant-figure failure; all twelve strategy variants remain representable. |
| from_f32_retain, from_f64_retain | Preserve binary input precision independently of the ordinary floating conversions. |
| as_i128, as_f64 | Infallible scalar responsibilities exposed by `to_i128` and `to_f64`; extended-scale panic boundaries must remain explicit. |
| Clone, Copy, Default, Eq, PartialEq, Ord, PartialOrd | Ordinary C++ scalar copy, default zero, numeric equality and total order. Inherited Ord min/max/clamp remain independently callable as ord_min/ord_max/clamp; Ord max selects the second numeric tie, unlike inherent Decimal max. No allocation domain or Context lifetime is extended by copying a Decimal. |
| Hash | Normalized four-word inputs and standard C++ hashing preserve equal-value hash consistency. Host hash algorithms are not a wire format. |
| Zero, One, Signed, Num | zero/one, inherited set_zero/set_one/is_one defaults, abs/abs_sub/signum/sign predicates and radix conversion remain independently callable. Resetting an identity replaces raw sign/scale as in the source defaults. |
| FromPrimitive, ToPrimitive, From and TryFrom | Signed/unsigned narrow integers, pointer widths, portable 128-bit values and f32/f64 conversions retain their distinct fallibility and error labels. |
| Add, Sub, Mul, Div, Rem and assignment/reference forms | Native operators preserve successful values and failure without partially changing an assignment destination. C++ const references cover Rust borrowed operands. |
| checked_add/sub/mul/div/rem, saturating_add/sub/mul | Optional checked results remain separate from saturation and throwing operators. Source extended-scale panic paths are not converted to saturation. |
| Inv | Independently callable Decimal::inv delegates ONE / self, preserving original successful raw representations, zero/overflow failure and unchanged scalar input. Ninety actual locked-trait calls include extended scales and both zero signs. |
| Sum and Product over values/references | Range folds retain source order and zero/one empty identities. |
| Display, Debug, LowerExp, UpperExp | Explicit-destination DecimalFormat supports precision, Unicode fill, width, alignment, plus and zero padding, including source capacity failures and exponential zero shortcuts. |
| Default Serialize/Deserialize | Serialize a string, retaining scale and the signed fixed-buffer capacity. Deserialize signed/unsigned integers, floating numbers and strings through the enabled visitor; reject unrelated JSON kinds with structured diagnostics. |
| UnpackedDecimal | Public negative/scale/hi/mid/lo fields, scalar copy, structural PartialEq and Debug. Repacking delegates validated construction; extended unpacked scales can fail. No source serde/default/hash derive is invented. |
| RoundingStrategy | All seven current and five deprecated variants, scalar Copy/Clone/PartialEq/Eq and structural Debug. Deprecated spelling does not remove a variant. |
| Error | Six variants with their active payloads, explicit-domain Clone, PartialEq, Debug, Display, conversion from text and the std Error default cause. |

## Inherited Ord defaults repair, 2026-10-08

The enabled dependency derives/implements Ord without overriding its standard
min/max/clamp defaults. Complete default-body review finds these omitted from
the earlier mapping. The original inherent Decimal::max preserves self on a
numeric tie, while Ord::max chooses the other operand; retained scales and zero
signs make that difference observable. The inherited clamp first rejects
reversed bounds, then returns the selected bound or the unchanged caller value.

A compile fixture reproduces the missing native ord_min, ord_max and clamp
members. The repair keeps both inherent operations intact and provides separate
Ord entries. Numeric tie selection preserves complete raw representations;
reversed bounds throw invalid_argument without changing any operand. No default
is replaced with C++ std::clamp's invalid-bound undefined behavior.

The original oracle directly invokes all three standard trait entries alongside
the two inherent methods. Six hundred five inputs cover every left/right pair
from zero signs, retained scales, signed/coefficient extrema and extended scales,
using equal bounds and ordinary, reversed and full-range intervals. Full scalar state,
selected representations, original panic and unchanged operands are retained.
The prior 3,675 observations remain unchanged; all 4,280 original outputs are
checked twice. Native and selected platform consumers are rebuilding; the
existing full native build remains live and will require a final header refresh.
Source/group/stage acceptance is not inferred from generating this corpus.

The standalone native probe then matches all 4,280 live original observations;
the named MIPS probe also matches all 4,280 without output changes. Fresh
thirteen-check Decimal/auxiliary selections pass 13/13 under ASan/UBSan with
leak scanning disabled (76.28 seconds) and 13/13 on OpenWrt MIPS/QEMU
(45.67 seconds). The negative compile fixture now passes. The full native
consumer build remains live, and the later header needs a final refresh before
complete current integration. This does not accept Value/time/Decimal or stages.

## Error diagnostics repair history

The enabled source `impl std::error::Error for Error {}` inherits a cause of
None. A direct C++ compile check reproduced the missing `DecimalError::source`
member. The repaired method is nonthrowing and returns nullptr for all six
variants; it allocates nothing and invents no nested cause.

Source Display uses `Formatter::pad` only for ErrorString. Its precision and width
count Unicode scalar characters, and sign/zero flags do not become numeric
formatting. The other five variants call `write!` and ignore the outer flags.
`DecimalError::format` now preserves that distinction with an explicit output
domain. `to_string` continues to expose the complete unformatted message.

The unchanged 94 auxiliary-diagnostic inputs now observe all six source causes,
plain Display and Clone equality, plus nine Display modes per variant: 5,076
actual original formatting observations. Every previous input and expected
field was checked unchanged against the saved corpus. The Rust oracle invokes
the locked dependency directly; it contains no copied error-format algorithm.
Native contracts additionally disable default PMR, retain all six errors into
Global, delete User, check every retained payload/cause/domain and allocate new
formatted output in the next User. A Unicode fixture distinguishes byte counting
from character counting and verifies zero precision.

## Evidence and remaining acceptance

The latest combined Value/time/Decimal consumers rebuild before execution and
pass 21/21 under ASan/UBSan with leak scanning disabled (78.13 seconds) and
21/21 on OpenWrt MIPS/QEMU (59.04 seconds). These include original Value tests,
owning/scalar Value APIs, Timestamp/UTC and every Decimal/auxiliary corpus,
including both actual pointer widths and all 4,280 inherited/inherent default
observations. The updated clock-floor implementation is linked into these
consumers. Final selected refreshes report no work; current full native
consumer refresh and complete CTest remain required before group acceptance.

Existing registered checks cover arithmetic, extended arithmetic, scale/binary
encoding, exact/radix parsing, primitive conversions, numeric foundations,
default JSON/numeric formatting, inline storage and auxiliary diagnostics.
See [extended arithmetic evidence](decimal-extended-acceptance.md),
[the API audit](parity/api-audit.md), and the corresponding `tests/parity`
original-call fixtures. Existing evidence keeps its original code-state limits.

The updated standalone native ownership executable passes, and all 94 inputs
match live original Rust on native and MIPS, including all 5,076 new format
observations. Fresh ASan/UBSan with leak detection passes all twelve selected
Decimal/auxiliary checks (12/12, 241.88 seconds); the same updated MIPS/QEMU
selection passes 12/12 (48.49 seconds). This includes arithmetic, parsing,
conversion, wire formatting, inline storage and scoped error retention.

A full native consumer refresh remains required. The earlier full-build plan was
interrupted after the final Decimal edits, then restarted once its Ninja process
was confirmed terminal, with eight jobs and 592 tasks. A subsequent process audit
finds another, newer build for concurrent Timestamp public-field modifications
writing the same native directory. This turn's duplicate 592-task build is stopped;
the newer 569-task build is retained. Its completion, any necessary final refresh
and full CTest at the latest worktree state remain required. The selected Decimal
results above precede that subsequent Timestamp change and keep that scope.
Builds started before the formatting definition changed produced stale
archive link failures; those failures are not successful verification. Their
terminal state was confirmed before starting incremental refreshes.

Remaining review must establish complete source arithmetic/conversion/default
and enabled serialization responsibilities, explicit failure/diagnostic behavior
and their use inside Value/SQL/runtime. This document does not accept the complete
Value/time group, Stage 2, downstream privacy/composition, physical-router execution
or the six-stage migration.

## Reciprocal trait repair, 2026-10-08

Complete enabled arithmetic_impls.rs review finds the source `Inv` trait omitted
from the prior C++ mapping. A native compile fixture reproduces the missing
`inv()` member. The implementation now delegates exactly `ONE / *this` to the
existing throwing division kernel. It introduces no alternate rounding policy,
allocation, storage, or dependency in the C++ runtime.

Ninety new calls invoke `num_traits::Inv::inv` on the locked Decimal directly.
Nine coefficient shapes combine both signs and scales 0/1/28/29/56, retaining
complete input/output text, coefficient, scale and sign, failures and unchanged
caller value. Sixteen original calls fail. All earlier 3,495 complete foundation
observations are verified unchanged; the corpus now has 3,585 cases. Oracle-only
num-traits 0.2.19 was already a locked Decimal dependency; its explicit import
allows observation of the trait rather than copying the source implementation.
Native contracts check retained 0.50 division scale, signs, full coefficient
boundaries and unmodified positive/negative zero inputs on failure. The missing
member compile fixture now passes, current-source native Decimal contracts pass
and all 3,585 observations match live pinned Rust. Current consumer and platform
integration remains pending.

## Inherited identity defaults, 2026-10-08

Reading the enabled num-traits 0.2.19 identities.rs body finds three inherited
methods missing from the earlier mapping: Zero::set_zero and One::set_one/is_one.
A compile fixture reproduces all three missing members. Explicit C++ methods
now reset to ZERO/ONE and compare numerically with ONE, matching those actual
trait defaults without allocation or extra storage.

Ninety original trait-default sequences use both signs and ordinary/extended
scales. They retain raw state before mutation, numeric is_one, canonical zero
state and its predicate, then canonical one state and its predicate. All earlier
3,495 complete foundation observations are checked unchanged; with reciprocal
and identity calls the corpus now contains 3,675 inputs. Native contracts also
check numeric one at retained scale and canonical sign/scale replacement at the
same object address with default PMR disabled.

The preceding entity/Timestamp platform checkpoint passes twelve selected
checks under ASan/UBSan (leak scanning disabled, 7.81 seconds) and MIPS/QEMU
(6.68 seconds). This predates the final identity-header refresh and is not
acceptance of the new Decimal defaults. The current seventeen-target sanitizer
and MIPS builds include Decimal, entity/Timestamp and locking/source fixtures;
final native consumer integration remains running.
