# Field Assist: note.optional_linked

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `optionalLinkedId()` / `updateOptionalLinkedId(const std::optional<std::uint64_t>&)`; changed `changedOptionalLinkedId()`; evaluation `evalOptionalLinkedId()`; expression `getOptionalLinkedId()`.
- Scalar predicates: `withOptionalLinkedIs`, `withOptionalLinkedIsNot`, `withOptionalLinkedGreaterThan`, `withOptionalLinkedGreaterThanOrEqualTo`, `withOptionalLinkedLessThan`, `withOptionalLinkedLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withOptionalLinkedBetween(lower, upper)`, `withOptionalLinkedIn(std::span<const std::uint64_t>)`, `withOptionalLinkedNotIn(std::span<const std::uint64_t>)`, `withOptionalLinkedIsNull()`, `withOptionalLinkedIsNotNull()`.
- Projection/order/group: `selectOptionalLinkedId()`, `orderByOptionalLinkedIdAsc()`, `orderByOptionalLinkedIdDesc()`, `groupByOptionalLinked()`.
- Group/count: `groupByOptionalLinked()`, `groupByOptionalLinkedAs(alias)`, `groupByOptionalLinkedWithFunction(alias, function)`, `countOptionalLinked()` and `countOptionalLinkedAs(alias)`.
- Forward relation: `optionalLinked()` returns const Tenant*, `updateOptionalLinked(const Tenant&)`; query `selectOptionalLinked()`, `selectOptionalLinkedWith(TenantRequest<P>)`, `withOptionalLinked(TenantRequest<P>)`; expression `getOptionalLinked()`.

Null alias: `withOptionalLinkedIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOptionalLinkedIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalOptionalLinked()`. Matching: `withOptionalLinkedMatching(child)`, `withoutOptionalLinkedMatching(child)`. Presence: `haveOptionalLinked()`, `haveNoOptionalLinked()`. Facets: `facetByOptionalLinkedAs(name, child)`, `facetByOptionalLinkedAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByOptionalLinkedWith(child)`, `groupByOptionalLinkedWithDetails()`, `groupByOptionalLinkedWithDetailsFrom(child)`. Roll-up: `rollUpToOptionalLinked()`, `rollUpToOptionalLinkedWith(child)`.

[Model/action index](../../../ASSIST.md)
