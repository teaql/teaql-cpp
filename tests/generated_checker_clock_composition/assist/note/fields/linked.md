# Field Assist: note.linked

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `linkedId()` / `updateLinkedId(const std::uint64_t&)`; changed `changedLinkedId()`; evaluation `evalLinkedId()`; expression `getLinkedId()`.
- Scalar predicates: `withLinkedIs`, `withLinkedIsNot`, `withLinkedGreaterThan`, `withLinkedGreaterThanOrEqualTo`, `withLinkedLessThan`, `withLinkedLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withLinkedBetween(lower, upper)`, `withLinkedIn(std::span<const std::uint64_t>)`, `withLinkedNotIn(std::span<const std::uint64_t>)`, `withLinkedIsNull()`, `withLinkedIsNotNull()`.
- Projection/order/group: `selectLinkedId()`, `orderByLinkedIdAsc()`, `orderByLinkedIdDesc()`, `groupByLinked()`.
- Group/count: `groupByLinked()`, `groupByLinkedAs(alias)`, `groupByLinkedWithFunction(alias, function)`, `countLinked()` and `countLinkedAs(alias)`.
- Forward relation: `linked()` returns const Tenant*, `updateLinked(const Tenant&)`; query `selectLinked()`, `selectLinkedWith(TenantRequest<P>)`, `withLinked(TenantRequest<P>)`; expression `getLinked()`.

Null alias: `withLinkedIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withLinkedIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalLinked()`. Matching: `withLinkedMatching(child)`, `withoutLinkedMatching(child)`. Presence: `haveLinked()`, `haveNoLinked()`. Facets: `facetByLinkedAs(name, child)`, `facetByLinkedAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByLinkedWith(child)`, `groupByLinkedWithDetails()`, `groupByLinkedWithDetailsFrom(child)`. Roll-up: `rollUpToLinked()`, `rollUpToLinkedWith(child)`.

[Model/action index](../../../ASSIST.md)
