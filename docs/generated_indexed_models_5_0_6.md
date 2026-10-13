# Generated indexed model integration for Rust 5.0.6

Status: fixed metadata and initial/setter/row state implemented; complete G01/G15
qualification remains open. This checkpoint follows the frozen producer
45e1572c4956ca963410d8ea436ead3c8857087a and runtime
fe82a18982415f1746df9a2cf7c42f5e9abdc6d3.

## Runtime and ownership

ContextArena installs each generated C++ model type's immutable FieldLayout once
in its explicit application Global arena. User Contexts borrow that geometry.
LoadedSnapshot pools belong to each result arena; Global never retains a User
pool or User snapshot. The small type catalog is an arena-owned cleanup payload;
its map allocations propagate the Global allocator. Snapshot pool bindings and
layout entries validate registered object identities. No singleton, default
resource, TLS or implicit shared owner is introduced.

The generated allocator constructor requires a live runtime Context. Generic
memory arenas do not manufacture application state. Cross-domain model copies
keep the existing explicit allocator-aware copy protocol. Their immutable Global
layout can remain canonical while snapshots, values and mutable ledgers belong to
the destination. Application code still uses UserContext to access runtime and
Global allocation; no application-facing Global argument is added.

## Generator adapters

CppFieldLayoutMetadata is confined to the C++ frontend. It follows the locked
ObjectDescriptor/PredefinedFieldIndex algorithm: normalized inherited field order,
canonical/member/column aliases, reserved dynamic-name exclusion, duplicate and
inheritance validation, and the same SHA-256 layout revision input. Other-language
generators and the user's original generator checkout remain unchanged. Supporting
metadata collection does not remove the frontend's inheritance feature guard.

Generated models emit fixed metadata and delegate installation to ContextArena.
They start with NotLoaded converted to Indexed. Scalar setters mark only their
field loaded, including explicit zero, false, empty text and optional NULL. Row
decoders use CompactRow.indexed_load_state and the model's canonical layout.
Checker field-selection installation converts legacy partial names into Indexed;
complete typed Checker generation/reconstruction is still pending.

Relation E evaluation checks an explicitly installed child before missing-load
fallback. Optional forward NULL requires its foreign-key field to be loaded and
NULL. Missing required children remain NotLoaded. Reverse list evaluation uses
its explicit loaded list view. An old fixture that treated a fresh uninitialized
required relation as loaded NULL now expects NotLoaded, matching the 5.0.6 source
contract. These changes do not qualify complete relation-handle generation.

## Evidence and remaining work

The copied School model fixture has SHA-256
0664ef5b32b1126100676782ec5c6c2d56e9e491920bfdfacadb4790a66b9ca9.
The C++ frontend's normalized fixed names and revision match the retained Rust
artifact's 14b4bfbb4cda258779665e3bc38ab991040fe75793a416d29eddd06f8fa8f586.
A wide metadata regression covers positions beyond 64 and order-sensitive revision
changes. These compare authentic producer metadata, not the complete School
runtime. Constants and model-driven fixes are still guarded or pending.

The final focused native integration passes 9/9: model/Q/E binding and expression
contracts, runtime layout ownership, the generated SQLite example, FieldLayout
contracts and the refreshed 44/411 source corpora. Probes are rebuilt against the
current runtime before consuming their outputs. Selected Java regressions pass
8/8. Both checked-in generated fixture sets reproduce all ten files; one positive
and nine negative compiler contracts pass. Targeted ASan/UBSan passes 3/3 with
leak scanning disabled. Named-module import passes 1/1 and directly instantiates
the per-application metadata catalog; all 1,312 exports and seven canonical facade
archives verify. All handwritten formatting and diff whitespace checks pass.
These are focused integration gates, not a full suite or complete model parity.

Relation/schema/durable result metadata, complete batch hydration, shared original
snapshots, typed Checker reconstruction, per-field changed/eval APIs and governed
save qualification remain open. The existing 411 kernel source observations remain
scoped kernel evidence; they are not relabelled as complete model parity. P1 stays
active and all 17 top-level capabilities remain unqualified.
