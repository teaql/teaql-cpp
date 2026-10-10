# Annotated source input graph API and ownership acceptance

Baseline: Rust v5.0.5 at `62940ae2b88e7acc32763853b8b4685a01c7e90a`,
`teaql-core/src/entity_graph.rs`. This is the annotated mutation input graph,
separate from the runtime identity/loading graph and graph-save execution.

## Complete source-group verdict, 2026-10-08

The entire production body is reviewed and accepted: operation variants/traits,
all five node fields and defaults, private builder node/public graph root,
generic entity construction, all four builder methods, recursive node Clone
and raw diagnostics. Save/Delete has no source Default, Hash or serde; node
has no PartialEq/Eq, Default, Hash or serde. Builder/graph have no derived
Clone/Debug/Default/comparison/serde. Explicit C++ allocator copies of wrappers
are lifetime adaptations; construction still requires no input Clone or Debug.

A fresh unchanged-source test passes 1/1. Current native, ASan/UBSan and MIPS
selections each pass 4/4 in 1.30, 2.47 and 1.77 seconds respectively. Current
sanitizers disable leak scanning under tracing and retain explicit reclamation
assertions. Fresh named native/MIPS probes match all 240 original inputs and
12,960 exact outputs. Native C++ is unchanged since the full 281/281 regression
in 72.92 seconds, and Ninja is clean; no repeat of that complete suite is needed
for this documentation-only acceptance update. Pinned source verification passes.

This closes only annotated graph construction. Runtime graph identity, persisted
save, governance, audit/log privacy and complete stages remain separate gates.
Historical checkpoints below retain their original scope.

## Complete source contract mapping

| Source responsibility | Preserved C++ contract |
| --- | --- |
| EntityGraphOperation | Save/Delete, ordinary copy/equality and standalone unit diagnostics. Native invalid enum states reject explicitly. |
| EntityGraphNode | All five fields: entity_type, MutationValues, optional comment, operation and ordered relation-name/node child pairs. Explicit-domain Clone and complete derived diagnostics retain every nested field. |
| EntityGraph::new<T> | from_entity consumes an Entity without a Clone or Debug requirement. Node type comes from entity_descriptor().name, not the potentially different ENTITY_NAME constant. into_values supplies the distinct mutation role. |
| Builder comment | Repeated assignment keeps the latest string; None, Some(empty) and nonempty/control/Unicode comments remain distinct. |
| Builder delete | Selects Delete independently of comments/children; repeated deletion remains Delete. |
| Builder child | Consumes the child builder, preserving relation text, append order, duplicate names, empty names and complete child nodes. |
| Builder build | Consumes the builder and produces its root input node, without performing persistence. |
| Source trait boundaries | Only operation/node derive Debug; only node derives Clone. The source builder and whole graph have neither derive. C++ explicit-domain copies of wrappers are documented lifetime adapters, not new source traits. Node has no source PartialEq/serde derive. |

entity_graph_debug.hpp exposes the two actual source derived capabilities through
core.hpp. It reuses the mutation/Value formatter and all inherited options.
Relation/name-child pairs use two-element tuple shapes, including pretty nesting;
they are not maps or single-element tuples. Raw business comments/values are not
safe SQL log projections.

The node now has an owning MutationValues overload. from_entity forwards its
consumed mutation result into that overload. Same-owner map nodes and heap string
payloads transfer instead of selecting the borrowed copying constructor. Foreign
retention follows the existing explicit destination-copy domain policy; no
shared owner extends User lifetime. No dependency or generator changed.

## Original and ownership evidence

The actual original oracle consumes non-Clone/non-Debug entities and observes all
builder methods, source node Clone and nine formatter modes. Its 240 inputs
retain 12,960 exact outputs, covering all Value variants, integer bounds, NaN,
Decimal scale, recursive JSON/containers, absent/explicit-empty/reassigned comments,
Save/Delete, descriptor-name provenance, duplicates, empty relation names and
three node levels. Edited Clone snapshots do not change the original node.
The initial native probe matches every output.

The unchanged original entity_graph.rs unit test passes 1/1. Its exact parent/child
inputs and annotation/operation assertions are ported in core_graph_api_tests.cpp.
The standalone native contract passes with default PMR disabled. It observes real
map-node and long-character storage identity through a consumed entity, keeps
separate duplicate relation nodes, and calls qualified core::write_debug.

The retained Global graph and pretty output remain valid after actual User
destruction and explicit rebinding through another User. Every nested mutation,
relation name, comment and node uses Global; independently edited copies retain
snapshot isolation. Formatting scratch consumes no new managed identities. An
isolated old declaration/forwarding overlay reproduces the unnecessary copy by
failing the storage-identity assertion; the repaired production header passes.

## Integrated acceptance pending

core_graph_api_contracts/core_graph_api_rust_parity are registered. The full native build and regression completed successfully with 234/234 checks.
Fresh ASan/UBSan with leak detection and OpenWrt MIPS/QEMU builds each pass all
ten selected checks covering input graph, entity expression/lifetime,
mutation/query/Trace diagnostics and core models. Fresh named native/MIPS probes
match all 240 live original inputs and 12,960 exact outputs. All three original
pipeline handles returned successful terminal results. The previous completed 232-check checkpoint predates this header repair.

This model gate does not establish persisted graph save, runtime graph identity,
full governance/audit/log parity, all six stages or physical-device execution.
Continue those responsibilities independently without collapsing their layers.

```sh
CARGO_TARGET_DIR="$PWD/build/rust-original-roundtrip" cargo build \
  --manifest-path tests/parity/rust_values/Cargo.toml \
  --bin core_graph_api_oracle --locked --offline
python3 tests/parity/generate_core_graph_api.py \
  --rust build/rust-original-roundtrip/debug/core_graph_api_oracle
python3 tests/parity/compare_core_graph_api.py --cpp <probe> \
  --rust build/rust-original-roundtrip/debug/core_graph_api_oracle
```

For MIPS add --cpp-emulator='/usr/bin/qemu-mips;-cpu;74Kf'.

## Selected integration checkpoint

Fresh ASan/UBSan with leak detection and OpenWrt MIPS/QEMU builds completed and
passed all ten selected checks. The fresh named MIPS graph probe matches all 240
live original inputs and 12,960 outputs. The full native rebuild/regression also completed with 234/234 checks; its
fresh named probe matches all 240 live original inputs. These scoped
results do not establish whole-stage or physical-device completion.
