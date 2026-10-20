# Field Assist: device.owner

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `ownerId()` / `updateOwnerId(const std::uint64_t&)`; changed `changedOwnerId()`; evaluation `evalOwnerId()`; expression `getOwnerId()`.
- Scalar predicates: `withOwnerIs`, `withOwnerIsNot`, `withOwnerGreaterThan`, `withOwnerGreaterThanOrEqualTo`, `withOwnerLessThan`, `withOwnerLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withOwnerBetween(lower, upper)`, `withOwnerIn(std::span<const std::uint64_t>)`, `withOwnerNotIn(std::span<const std::uint64_t>)`, `withOwnerIsNull()`, `withOwnerIsNotNull()`.
- Projection/order/group: `selectOwnerId()`, `orderByOwnerIdAsc()`, `orderByOwnerIdDesc()`, `groupByOwner()`.
- Group/count: `groupByOwner()`, `groupByOwnerAs(alias)`, `groupByOwnerWithFunction(alias, function)`, `countOwner()` and `countOwnerAs(alias)`.
- Forward relation: `owner()` returns const Person*, `updateOwner(const Person&)`; query `selectOwner()`, `selectOwnerWith(PersonRequest<P>)`, `withOwner(PersonRequest<P>)`; expression `getOwner()`.

Null alias: `withOwnerIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOwnerIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalOwner()`. Matching: `withOwnerMatching(child)`, `withoutOwnerMatching(child)`. Presence: `haveOwner()`, `haveNoOwner()`. Facets: `facetByOwnerAs(name, child)`, `facetByOwnerAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByOwnerWith(child)`, `groupByOwnerWithDetails()`, `groupByOwnerWithDetailsFrom(child)`. Roll-up: `rollUpToOwner()`, `rollUpToOwnerWith(child)`.

[Model/action index](../../../ASSIST.md)
