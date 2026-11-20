# Option model name compatibility exception

Approved by the user on 2026-11-20. Preserve the correct C++ domain type and
record the locked Rust failure separately.

The reference remains Rust runtime 5.0.6 at
`fe82a18982415f1746df9a2cf7c42f5e9abdc6d3` and generator
`45e1572c4956ca963410d8ea436ead3c8857087a`.

The complete retained `option_plain_self` model declares an entity named
`option`, an inherited child, optional fields and a reverse-relation projection
whose `json_type` is `Self`. The locked Rust generator emits a domain `Option`
that shadows the prelude's generic `Option<T>`. Its generated library fails
compilation with exit 101, including E0107. The C++ generated consumer compiles
with exit 0 and projects the modeled `Option` entity.

The [original diagnostic](parity/ap02_plain_option_model_diagnostic_5_0_6.json.gz)
retains the complete model, both generated libraries, consumer, commands and
compiler diagnostics. The mismatch remains visible; approval does not turn it
into a matching Rust result or a successful original build.

The approved contract preserves that C++ behavior. It does not authorize
guessing an unknown type, ignoring a genuinely required prelude binding,
accepting unresolved projections or treating every compiler error as this
exception. Existing controls for `ToOwned`, `Option<Self>` and explicit
qualified paths retain their meanings.

This is a generated-type/compiler exception. It establishes no SQLite execution
or graph-save result for this model. The formal compiler-backed generation
route still propagates original compiler failures; joining this approved C++
behavior to that route remains an explicit integration obligation. Full G14,
G13, G17 and P0-P7 acceptance remain separate.

The machine-readable scope and immutable evidence hash are in
[`compatibility_exceptions_5_0_6.json`](compatibility_exceptions_5_0_6.json).
