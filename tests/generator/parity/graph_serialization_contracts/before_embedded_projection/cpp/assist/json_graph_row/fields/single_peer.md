# Field Assist: json_graph_row.single_peer

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `singlePeerId()` / `updateSinglePeerId(const std::optional<std::uint64_t>&)`; changed `changedSinglePeerId()`; evaluation `evalSinglePeerId()`; expression `getSinglePeerId()`.
- Scalar predicates: `withSinglePeerIs`, `withSinglePeerIsNot`, `withSinglePeerGreaterThan`, `withSinglePeerGreaterThanOrEqualTo`, `withSinglePeerLessThan`, `withSinglePeerLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withSinglePeerBetween(lower, upper)`, `withSinglePeerIn(std::span<const std::uint64_t>)`, `withSinglePeerNotIn(std::span<const std::uint64_t>)`, `withSinglePeerIsNull()`, `withSinglePeerIsNotNull()`.
- Projection/order/group: `selectSinglePeerId()`, `orderBySinglePeerIdAsc()`, `orderBySinglePeerIdDesc()`, `groupBySinglePeer()`.
- Group/count: `groupBySinglePeer()`, `groupBySinglePeerAs(alias)`, `groupBySinglePeerWithFunction(alias, function)`, `countSinglePeer()` and `countSinglePeerAs(alias)`.
- Forward relation: `singlePeer()` returns const JsonGraphRow*, `updateSinglePeer(const JsonGraphRow&)`; query `selectSinglePeer()`, `selectSinglePeerWith(JsonGraphRowRequest<P>)`, `withSinglePeer(JsonGraphRowRequest<P>)`; expression `getSinglePeer()`.

Null alias: `withSinglePeerIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withSinglePeerIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalSinglePeer()`. Matching: `withSinglePeerMatching(child)`, `withoutSinglePeerMatching(child)`. Presence: `haveSinglePeer()`, `haveNoSinglePeer()`. Facets: `facetBySinglePeerAs(name, child)`, `facetBySinglePeerAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupBySinglePeerWith(child)`, `groupBySinglePeerWithDetails()`, `groupBySinglePeerWithDetailsFrom(child)`. Roll-up: `rollUpToSinglePeer()`, `rollUpToSinglePeerWith(child)`.

[Model/action index](../../../ASSIST.md)
