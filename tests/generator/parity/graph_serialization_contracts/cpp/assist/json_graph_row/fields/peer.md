# Field Assist: json_graph_row.peer

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `peerId()` / `updatePeerId(const std::optional<std::uint64_t>&)`; changed `changedPeerId()`; evaluation `evalPeerId()`; expression `getPeerId()`.
- Scalar predicates: `withPeerIs`, `withPeerIsNot`, `withPeerGreaterThan`, `withPeerGreaterThanOrEqualTo`, `withPeerLessThan`, `withPeerLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPeerBetween(lower, upper)`, `withPeerIn(std::span<const std::uint64_t>)`, `withPeerNotIn(std::span<const std::uint64_t>)`, `withPeerIsNull()`, `withPeerIsNotNull()`.
- Projection/order/group: `selectPeerId()`, `orderByPeerIdAsc()`, `orderByPeerIdDesc()`, `groupByPeer()`.
- Group/count: `groupByPeer()`, `groupByPeerAs(alias)`, `groupByPeerWithFunction(alias, function)`, `countPeer()` and `countPeerAs(alias)`.
- Forward relation: `peer()` returns const JsonGraphRow*, `updatePeer(const JsonGraphRow&)`; query `selectPeer()`, `selectPeerWith(JsonGraphRowRequest<P>)`, `withPeer(JsonGraphRowRequest<P>)`; expression `getPeer()`.

Null alias: `withPeerIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPeerIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalPeer()`. Matching: `withPeerMatching(child)`, `withoutPeerMatching(child)`. Presence: `havePeer()`, `haveNoPeer()`. Facets: `facetByPeerAs(name, child)`, `facetByPeerAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByPeerWith(child)`, `groupByPeerWithDetails()`, `groupByPeerWithDetailsFrom(child)`. Roll-up: `rollUpToPeer()`, `rollUpToPeerWith(child)`.

[Model/action index](../../../ASSIST.md)
