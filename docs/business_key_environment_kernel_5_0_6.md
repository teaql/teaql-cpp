# Shared Business-ID key environment kernel

This implements the shared parsing and managed-provider responsibility required
by [the source startup repair](business_runtime_environment_alignment_5_0_6.md).
This shared-kernel checkpoint did not connect the generated source-shaped
factories. Their later [startup qualification](generated_business_environment_startup_5_0_6.md)
is separate evidence and closes R04; G12, P6 and P0-P7 remain open.
References remain Rust 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

`read_generated_environment_value` shares the existing immediate environment
copy and Unicode validation with database configuration. Its typed error keeps
missing and non-Unicode values distinct. `read_generated_business_id_key` parses
the source key/version rules into the existing `BusinessIdEncodingKey`.
`install_generated_business_id_environment_key` allocates a
`StaticBusinessIdKeyProvider` in UserContext and installs it through the existing
Business-ID service. It requires infrastructure to exist first; it does not
register an executor or initialize schema. There is no new encoding algorithm,
cryptographic implementation, dependency, hidden owner or environment cache.

## Actual source boundaries

The [fresh original witnesses](parity/business_key_environment_source_witnesses_5_0_6.json.gz)
replay all eighteen retained startup errors against the unchanged original
executable, then observe seven additional boundaries. The
[shared-reader comparison](parity/business_key_environment_kernel_final_observations_5_0_6.json.gz)
matches their genuine typed error payloads and three successful source versions.
Its scope is the shared reader, not complete factory execution or setup order.

| Input | Original responsibility retained |
| --- | --- |
| Missing or non-Unicode key | Typed MissingEnv, preserving the underlying environment error. |
| Wrong key byte length | Configuration error requiring exactly 64 hexadecimal characters. |
| Invalid hexadecimal pair | Configuration error requiring hexadecimal characters. |
| `+A` repeated 32 times | The source radix parser accepts each pair as byte ten. |
| Missing or non-Unicode version | Default to version one; an empty version is a parse error. |
| `+1` version | Accepted as positive unsigned input. |
| Zero, invalid or overflowing version | Preserve the source configuration error. |
| Twenty euro characters followed by four zeroes | A valid 64-byte UTF-8 string crosses the source's first two-byte slice boundary. Rust panics; C++ throws `std::out_of_range`. Neither returns a typed configuration result. |

The last row explicitly retains language-specific unwind behavior. Rust's
process exit 101 and panic diagnostic are not normalized into the C++ observer's
exit 1 and exception diagnostic. Two-byte Unicode pairs instead reach the
ordinary invalid-hex result. Per-pair validation preserves an earlier invalid
pair rather than scanning for a later boundary failure first.

## Ownership and execution

The native fixture disables default PMR allocation, verifies every parsed key
byte, installs the actual managed provider and exercises the existing key
permutation profile: `ORD-20261003-EWCLPK`. A failed parse leaves the previously
installed service unchanged. All nested errors use the caller's resource.
Actual UserContext deletion returns all User bytes; the retained service borrow
then fails with `ReferenceError::Kind::Closed`.

The [terminal checkpoint](parity/business_key_environment_kernel_terminal_5_0_6.json.gz)
passes native 3/3, imported modules 3/3 and selected ASan/UBSan 3/3. Each selection
includes the new managed-key contracts, the existing configuration ownership
contracts and the original configuration-kernel comparison. Leak scanning is
disabled. The same handwritten fixture body runs through both frontends.
The [module reader comparison](parity/business_key_environment_kernel_module_observations_5_0_6.json.gz)
also passes all source error/version/boundary dispositions. The rebuilt module
import contract passes separately. Mechanical checks verify 1,498 exports,
792 consumer bodies, 75 runtime-root registrations and the locked 39 generator
sources/17 capabilities; seven capabilities remain unqualified. Pinned
formatting/checking covers 1,647 handwritten files.

## Retained verification failures

The initial native compile copied a noncopyable configuration error into
`Result::err`; the corrected code moves its existing scoped payload. The first
lifecycle assertion caught `std::logic_error`, whereas the checked borrow
correctly raises `ReferenceError`; the fixture now tests its precise Closed kind.
The [first module fixture build](parity/business_key_environment_initial_module_fixture_failure_5_0_6.json.gz)
identified missing explicit standard-library includes in the fixture.
The [initial export coverage check](parity/business_key_environment_export_coverage_initial_failure_5_0_6.json.gz)
identified three missing isolated lookup declarations. Neither issue required
weakening production lifetime checks or changing module ownership.

## Remaining work

The [provider initialization checkpoint](business_runtime_initialization_5_0_6.md)
now qualifies the shared step after Business-ID infrastructure and before
executor registration. The later generated startup checkpoint now qualifies the
thin model-conditional factories, full typed errors, authentic constants,
artifact reproduction and all three actual source-shaped entries.
Preserve the existing explicit filename/manual-key path. The previous manual-key
54-round comparison is not automatic environment startup evidence.
