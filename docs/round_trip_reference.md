# Context-bound round-trip references

This implements the separate Rust v5.0.5 round_trip_reference module and all
associated UserContext methods. The older [entity-reference codec](entity_reference.md)
remains independent. Neither portable token type is a process-local Context
memory Ref ID.

## Responsibilities and flow

TrustedReferencePrincipal retains authentication realm, subject, domain-root type
and identity. ReferenceDocumentScope retains document identity, purpose, aggregate
type/identity and aggregate revision. ReferenceIdentity retains entity type,
identity and version. Their checked factories preserve the original validation;
raw public fields can still be changed, so issue/consume revalidate their inputs.
Bound text must be nonempty, at most 4,096 UTF-8 bytes and already trimmed.
NUL bytes remain valid text. Identities must be nonzero, and revisions/versions
nonnegative. These rules differ from the older codec and are not merged with it.

ReferenceKey, ReferenceKeyProvider, StaticReferenceKeyProvider and
ReferenceAuthorizationPolicy remain distinct. Keys require a 32-byte master and
an ASCII alphanumeric/hyphen/underscore ID of 1–64 bytes. Static providers reject
duplicate IDs and require the configured current key. Key Debug output redacts
master material. The FunctionReferenceAuthorizationPolicy adapter retains the
source blanket Fn implementation with explicit scoped capture rules.

ContextBoundReferenceRuntime stores deployment profile, mode, service/environment,
checked providers and scoped clock/nonce callbacks. Both assembly paths validate
service/environment and the active key, including raw mode. `governed` ignores
process acknowledgement. `from_process_environment` accepts only the exact source
acknowledgement; Production rejects it. Raw startup notices retain the source
ERROR message, profile, telemetry pair and response headers, with no secrets.
The process-environment assembly writes the same notice to stderr. This is
configuration/metadata, not an HTTP server implementation.

Issue validates principal, document and identity, requires at least one whole
second of lifetime, and authorizes before choosing raw or governed output. Raw
output still authorizes and preserves identity/version. Governed output then
requests the current key, samples time, validates chrono conversion/expiry,
computes the actor fingerprint, frames/encrypts the payload and samples a nonce.
Checked expiry-addition failure is a token error here, unlike the older codec's
addition-panic contract. Raw mode does not perform chrono conversion or encryption.

Consume validates principal/document and expected type, checks wire mode, and
authenticates/decrypts governed tokens. Payload validation precedes the clock;
expiry precedes the one-minute issued-time tolerance, actor/document/type binding
and return authorization. Current authorization is always rechecked after a
successful resolve. Purpose, service/environment and key ID participate in AAD
and key derivation; their cryptographic mismatches return INVALID. Actor, document,
aggregate revision and entity-type mismatches return SCOPE_MISMATCH. Expiry and
authorization retain their separate source codes. UTC conversion of returned
issued/expiry metadata remains optional, matching the source even for authenticated
timestamps outside chrono's calendar range.

ReferenceWireCodec serializes governed references as strings and raw references
as exactly `id` and `version`. Governed deserialization only requires the `tqr1.`
prefix; full token validation belongs to consume. Raw deserialization rejects
extra fields, zero/negative identities, negative versions and floating/string
number substitutes. It retains u64 identities and signed i64 versions. UserContext
serialization/deserialization requires an installed runtime and the correct mode;
it does not invent a second authorization pass for these wire operations.

## Wire and cryptography

Governed tokens are `tqr1.<key-id>.<URL-safe-unpadded-base64>`, with a maximum input
token length of 24,000 bytes. The binary envelope is nonce plus AES-256-GCM
ciphertext/tag. HKDF-SHA256 uses the source salt, length-prefixed labels,
service/environment and selected key ID to derive separate encryption and
actor-fingerprint keys. HMAC-SHA256 binds the canonical trusted principal;
the encrypted payload retains the source field order and big-endian integers.
AAD binds the prefix, selected key ID, service/environment and purpose.

The internal HMAC/HKDF primitives reuse fixed-storage SHA256 and add no production
dependency. Their checks include [RFC 4231](https://www.rfc-editor.org/rfc/rfc4231)
and the first output block of [RFC 5869](https://www.rfc-editor.org/rfc/rfc5869).
The full original round-trip golden token also matches byte for byte. AES/GCM
uses the existing independently vector-tested kernel. Fixed vector nonces are
test inputs; default nonces use OS entropy. These checks do not establish
certification, independent cryptographic review or platform side-channel claims.

## Context ownership

Runtime, providers, callbacks, trusted principal, claims, wire values and protocol
buffers have explicit Global/User ownership. UserContext copies installed trusted
principal values into User storage. Its high-level reference methods preserve
missing-runtime before missing-principal error order and checked resource access.
Business callers can register managed references without exposing Global.

ResourceBorrow replaces the source Arc ownership extension. Copying a runtime
into Global may rebind already Global-owned providers, but rejects User-owned
providers. Callback captures follow the established allocator-aware/empty-function
copy contract. Native acceptance destroys User, rebinds an explicitly retained
Global runtime into another User, and resolves its retained nested claims. Earlier
access does not become valid again. Provider destruction is detected before
virtual lookup. Synchronous guards prevent reentrant owner/output close during
key, clock, nonce or authorization callbacks.

Managed cleanup wipes retained key and derived-key material before arena release;
HMAC state containing extraction input and protocol plaintext scratch is wiped.
This does not promise wiping arbitrary external storage or every compiler temporary.
There is no shared ownership, hidden key cache or default-PMR query storage.

## Differential evidence

The 933 observations invoke unchanged original public model, provider, wire,
runtime and UserContext methods. SPI fixtures record complete authorization
arguments and key/clock/nonce order. They cover exact acknowledgement/profile
gating, safe startup metadata, validation/length/numeric boundaries, missing
installation, raw/governed wire shapes, revoked authorization, actor/document/type
binding, purpose/service/environment mismatch, fresh and old keys, expiration,
nanoseconds/leap seconds and authenticated malformed payloads. Malformed fixtures
mutate bytes decrypted from the original golden token and reauthenticate them
with already-locked original Rust primitives; they do not copy codec parsing.

All 17 original module tests run unchanged. The shared Cargo target failed with
macro/runtime API mismatches; a fresh isolated target passed without changing
the reference source. Use an isolated target for reproducible original unit tests:

```sh
CARGO_TARGET_DIR="$PWD/build/rust-round-trip-reference" cargo test --locked --offline \
  --manifest-path .reference/teaql-rs/teaql-runtime/Cargo.toml \
  --lib round_trip_reference::tests
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust_values/Cargo.toml --bin round_trip_reference_oracle
python3 tests/parity/generate_round_trip_reference.py \
  --rust build/rust-values/debug/round_trip_reference_oracle
python3 tests/parity/compare_round_trip_reference.py \
  --cpp build/teaql_round_trip_reference_probe \
  --rust build/rust-values/debug/round_trip_reference_oracle
ctest --test-dir build --output-on-failure -R '^round_trip_reference_'
```

Native lifetime checks additionally require ASan/UBSan and OpenWrt MIPS/QEMU.
Physical-router execution remains unverified. Runtime reexports and the complete
API/privacy/source-test audits remain separate six-stage acceptance gates.

Acceptance checkpoint on 2026-10-07: after rebuilding all consumers, the full
native Debug suite passes 172/172. Four selected ASan/UBSan checks pass with leak
detection; three OpenWrt MIPS/QEMU native ownership/reentrant checks pass. Native
and MIPS probes both match all 933 live original observations, including the
authenticated malformed payloads and old-key rotation cases. All 17 original
module tests pass unchanged in the isolated target. Locked Rust v5.0.5,
58-module inventory, pinned clang-format 18.1.8 and whitespace checks pass.
This completes the tested context-bound reference implementation checkpoint;
it does not establish the remaining complete API/export/privacy/source-test gates.


## Individual original source-test review

All 17 indexed tests in `teaql-runtime/src/round_trip_reference.rs` now have
individually reviewed direct ports in
`tests/parity/teaql_runtime/round_trip_reference.cpp`. The controlled helpers
provide only original key/principal/document/identity/clock/nonce inputs and the
original authorization predicate. All crypto, wire parsing, profile selection,
issuance, consumption and Context delegation execute production code. The
Fn-style policy uses the actual FunctionReferenceAuthorizationPolicy adapter.
Every fixture object and nested payload uses an explicit User allocator; default
PMR is disabled. Atomic policy/nonce state belongs to User rather than an Arc
that could prolong its lifetime.

The reviewed cases preserve every source assertion:

- Complete original golden token, prefix and absence of cleartext entity/actor;
  exact resolved identity/key/mode; bob/doc101 scope codes and wrong-type rejection.
- Old k1/0x11 key with nonce0x44 remains decode-only after k2 rotation, while
  k2-only removal yields the exact coarse invalid code.
- Service/environment/domain-root binding, all purpose/aggregate-ID/revision
  substitutions and original final-character A/B tampering.
- Sequential atomic nonce bytes1/2 yield distinct references that both resolve;
  nonce0x55 with60-second life rejects at the original61-second consumer clock.
- UserContext missing-principal and missing-runtime error precedence, complete
  installed issue/wire/resolve roundtrip and exact resolved identity.
- All nine raw-mode tests: absent/exact/near-match acknowledgement behavior,
  Production configuration rejection, current bob authorization, changed-mode
  wire rejection, retained version/type guards and complete ordered safe startup
  headers/telemetry with both secret-omission assertions.
- Current authorization changes from atomic1 to0 after issue and is rechecked
  on return. This case retains the source default OS clock and nonce; fixed
  vector inputs are not substituted for it.
- Three invalid raw JSON shapes/numbers, exact999ms life and24001-byte oversized
  token body are all rejected by the real codec/runtime.

The private Rust acknowledgement constructor returns a Result; its C++ scoped
constructor counterpart throws the same allocation-independent coarse error.
Explicit scoped borrows replace owned Arc providers without altering issuance
or binding semantics. Failed construction does not publish a runtime. These
source assertions remain separate from full protocol/API/default/lifecycle and
physical-target acceptance; golden vectors are not cryptographic certification.

On 2026-10-07 unchanged Rust passes exactly17/17 under the registered
round_trip_reference::tests filter. The direct C++ target passes native Debug.
The disposition ledger is 364 covered, 77 pending, five backend and five external
exclusions. Remaining indexed source reviews are 75 runtime root tests and two
entity-runtime graph-reference lifetime adaptations. Complete-stage acceptance
and the separate public API/composition/privacy/target audit remain open; no
production behavior, dependency or generator changed.


The complete native Debug regression subsequently passes 265/265 in62.90
seconds. After rebuilding, the original source group and existing Context
ownership/policy group pass 2/2 under ASan/UBSan with leak detection and 2/2
under OpenWrt MIPS/QEMU. The latter separately covers actual User deletion,
Global provider/runtime/claim copies, shorter-provider rejection and checked
resource expiry. This does not extend the source assertions into complete
runtime/API/lifecycle or physical-device acceptance.

```sh
cmake --build build --target teaql_original_round_trip_reference_tests \
  teaql_round_trip_reference_tests
ctest --test-dir build --output-on-failure \
  -R '^(original_runtime_round_trip_reference_contracts|round_trip_reference_context_contracts)$'
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" \
  cargo test --manifest-path .reference/teaql-rs/Cargo.toml \
  -p teaql-runtime --lib round_trip_reference::tests --locked --offline
```

Use `build-sanitize` with leak detection/UBSan halt-on-error, or
`build-wndr4300-memory` with its registered QEMU emulator. Pinned source,
58-module inventory, registered dispositions, root-export locators, compiled
runtime layer separation and541-file formatting/whitespace checks remain
separate evidence from the full requirement-by-requirement completion audit.
