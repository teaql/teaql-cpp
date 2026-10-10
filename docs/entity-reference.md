# Portable entity-reference codec

The EntityReferenceCodec boundary is implemented from Rust v5.0.5
`62940ae2b88e7acc32763853b8b4685a01c7e90a`. It is separate from process-local
Context `Ref<T>` IDs and from the separate
[ContextBoundReferenceRuntime](round-trip-reference.md).
This codec provides encrypted portable identity claims; it does not itself
implement principal authorization or document-bound governed references.

## Preserved contracts

- EntityReferenceClaims contains entity type, u64 identity, i64 version, UTC
  issued/expiry times, purpose and u32 key version. Strings use the requested
  destination allocator. Negative versions remain valid in this older codec.
- EntityReferenceTokenError retains `ENTITY_REFERENCE_INVALID` and
  `ENTITY_REFERENCE_CODEC_REQUIRED` without exposing parsing or key details.
- EntityReferenceCodec retains distinct encode/decode responsibilities. UserContext
  installs a checked scoped resource and delegates both calls; missing codecs
  fail closed. Calling a closed Context or using an early-destroyed codec rejects
  access before virtual dispatch.
- AeadEntityReferenceCodec requires 32-byte keys and a present active version.
  Duplicate numeric versions preserve the source's last-value replacement rule.
  Retained older versions remain usable for decoding after rotation.
- Wire output is exactly `tqr1.` followed by URL-safe unpadded base64 of the
  big-endian u32 key version, 12-byte nonce, ciphertext and 16-byte tag.
  AES-256-GCM authenticates `teaql.entity-reference.v1`. The numeric version
  header is not extra AAD; relabeling among retained identical key materials
  follows the original behavior rather than silently changing the protocol.
- The encrypted claims use u16 UTF-8 byte lengths and big-endian numeric fields.
  Encoding rejects whitespace-only entity types, zero identity and zero lifetime.
  Decoding rejects malformed framing, invalid UTF-8, empty type, zero identity,
  trailing bytes, unknown keys, invalid dates and authentication failure.
- Decode binds exact untrimmed entity type and purpose, rejects expiry at or before
  now, and rejects issued times more than one minute in the future. Wire times
  intentionally discard subsecond precision, as in Rust. ReferenceLifetime
  preserves Rust Duration's full u64 seconds domain and normalizes nanoseconds;
  chrono TimeDelta conversion errors remain token errors, while DateTime addition
  overflow remains an exception corresponding to the source panic.

## Ownership and callbacks

Allocate codecs in Global or User through the scoped factories and register them
through UserContext. Business callers can use checked references without obtaining
Global. Token output, decoded claims and protocol buffers use the requested
destination. Global retention requires explicit deep copies. Native acceptance
destroys the original User and then resolves retained Global claims, keys and
allocator-aware clock/nonce callbacks through another User.

Default clocks use UTC now. Linux nonces come from `getrandom`, retrying partial
reads and interruptions; kernels without that syscall fall back to `/dev/urandom`.
Entropy failures propagate and never substitute predictable nonce bytes. Other
platforms use std::random_device only when it reports an entropy source and are
not covered by the current Linux/OpenWrt acceptance. Injectable clock/nonce
callbacks preserve the original testability contract. Fixed nonces in fixtures
are deterministic test inputs; application nonce sources retain the obligation
to avoid reuse with the same key.

Callbacks live in the codec's Context. Cross-domain copies require allocator-aware
state or the existing capture-free callable contract; arbitrary captured borrows
are not silently retained. Synchronous guards reject reentrant reclamation of
either codec owner or output domain while a callback is active. Key-material
objects and expanded AES schedules wipe their retained storage before destruction;
plaintext scratch is wiped on normal and exceptional exits. Arena reclamation
does not imply wiping every compiler temporary or external allocation.

## Compact cryptographic foundation

The internal primitive uses C++20 fixed arrays for AES-256 key expansion and block
operations, fixed-operation GF arithmetic for substitution, and bitwise GHASH.
There are no secret-indexed lookup tables, process-owned key caches, or additional
C++ production libraries. GCM supports the original codecs' 96-bit nonce and
128-bit tag shape, checks input bounds, and authenticates the complete tag before
allocating or returning plaintext. Ciphertext and decoded payloads use PMR.

The algorithm sources are [NIST FIPS 197](https://csrc.nist.gov/pubs/fips/197/final)
and [NIST SP 800-38D](https://csrc.nist.gov/pubs/sp/800/38/d/final). The 750 selected
AES-256/96-bit-IV/128-bit-tag cases are unmodified data from the
[official NIST CAVP archive](https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/CAVP-TESTING-BLOCK-CIPHER-MODES).
The archive SHA-256 is
`f9fc479e134cde2980b3bb7cddbcb567b2cd96fd753835243ed067699f26a023`.
The AES block check uses the unchanged algorithm's Appendix C.3 example in the
[archived FIPS 197](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf).
These are correctness checks, not CAVP/FIPS certification, independent security
review, or a measured compiler/platform side-channel guarantee.

## Reproducible evidence

The 548 observations call unchanged original public AeadEntityReferenceCodec and
UserContext methods. They retain exact tokens and claims, constructor errors,
large numeric boundaries, nanosecond/leap-second behavior, expiration, rotation,
purpose/type mismatches, base64 mutation and the u16 text limits. Authenticated
malformed plaintext fixtures use the original already-locked AES-GCM primitive;
they do not copy either codec's parsing logic. Source panics are recorded separately
from token errors. Original module tests run unchanged.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-values" cargo build --locked --offline \
  --manifest-path tests/parity/rust-values/Cargo.toml --bin entity_reference_oracle
python3 tests/parity/generate_entity_reference.py \
  --rust build/rust-values/debug/entity_reference_oracle
python3 tests/parity/compare_entity_reference.py \
  --cpp build/teaql_entity_reference_probe \
  --rust build/rust-values/debug/entity_reference_oracle
cargo test --locked --offline \
  --manifest-path .reference/teaql-rs/teaql-runtime/Cargo.toml \
  --lib entity_reference::tests
ctest --test-dir build --output-on-failure \
  -R '^(entity_reference_.*|reference_crypto_nist_contracts)$'
```

The NIST extraction tool takes the original downloaded archive and verifies its
checksum before selecting records. Native, ASan/UBSan and MIPS/QEMU verification
are required for the Context integration. Physical-router execution remains
unverified. Round-trip principal/document/authorization/wire/profile behavior
is implemented and validated separately. Runtime reexports and the broader
six-stage acceptance remain separate work.

Acceptance checkpoint on 2026-10-07: after rebuilding every consumer, native Debug
passes 168/168 CTest checks. Five selected ASan/UBSan checks pass with leak
detection. OpenWrt MIPS/QEMU passes four native checks, including all 750 NIST
vectors, and matches all 548 live original Rust observations. The native probe
matches the same 548 live observations; both original Rust module tests pass.
Locked upstream, 58-module inventory, clang-format 18.1.8 and whitespace checks
pass. This checkpoint completes the older codec implementation's tested gate,
not the separate context-bound reference runtime or remaining migration stages.


## Direct original source-test review

Both indexed `entity_reference.rs` tests now have individually reviewed direct
C++ evidence. `portable_reference_original_contract` retains every source
input and assertion: key version two and 32 bytes of 0x22, the fixed UTC clock
2026-09-26T12:00:00Z, twelve nonce bytes of 0x33, OrderItem/42/7/edit-order,
3600-second lifetime, the exact original GOLDEN wire value, absence of cleartext
entity type, the complete 42/7/2 decoded tuple, both type/purpose mismatches and
replacement of the token's final character with A. It calls the real codec
encode/decode directly. The source test's name mentions rotation and expiry,
but its body does not assert them; the broader corpus/native tests remain
separate evidence for those contracts.

`native_contracts` retains the original Context test's missing-codec input and
exact error code, checked codec installation, complete golden token and decoded
identity. Explicit User allocation/registration replaces Rust's owned Arc.
Existing additional tests actually destroy User and verify deep Global copies,
resource expiry and callback/key ownership; they do not prolong its arena.
Default PMR is disabled in these C++ tests.

On 2026-10-07 unchanged Rust passes 2/2 original module tests. After rebuilding,
`entity_reference_context_contracts` passes 1/1 on native Debug, ASan/UBSan with
leak detection, and OpenWrt MIPS/QEMU. The fixture helpers only provide fixed
clock/nonce inputs and key bytes; they contain no codec algorithm. No production
behavior, dependency or generator changed. Physical-device and full-stage
acceptance remain separate.
