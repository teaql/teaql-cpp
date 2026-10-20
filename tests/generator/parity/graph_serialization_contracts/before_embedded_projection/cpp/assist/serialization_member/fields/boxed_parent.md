# Field Assist: serialization_member.boxed_parent

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `boxedParentId()` / `updateBoxedParentId(const std::optional<std::uint64_t>&)`; changed `changedBoxedParentId()`; evaluation `evalBoxedParentId()`; expression `getBoxedParentId()`.
- Scalar predicates: `withBoxedParentIs`, `withBoxedParentIsNot`, `withBoxedParentGreaterThan`, `withBoxedParentGreaterThanOrEqualTo`, `withBoxedParentLessThan`, `withBoxedParentLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withBoxedParentBetween(lower, upper)`, `withBoxedParentIn(std::span<const std::uint64_t>)`, `withBoxedParentNotIn(std::span<const std::uint64_t>)`, `withBoxedParentIsNull()`, `withBoxedParentIsNotNull()`.
- Projection/order/group: `selectBoxedParentId()`, `orderByBoxedParentIdAsc()`, `orderByBoxedParentIdDesc()`, `groupByBoxedParent()`.
- Group/count: `groupByBoxedParent()`, `groupByBoxedParentAs(alias)`, `groupByBoxedParentWithFunction(alias, function)`, `countBoxedParent()` and `countBoxedParentAs(alias)`.
- Forward relation: `boxedParent()` returns const SerializationParent*, `updateBoxedParent(const SerializationParent&)`; query `selectBoxedParent()`, `selectBoxedParentWith(SerializationParentRequest<P>)`, `withBoxedParent(SerializationParentRequest<P>)`; expression `getBoxedParent()`.

Null alias: `withBoxedParentIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withBoxedParentIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalBoxedParent()`. Matching: `withBoxedParentMatching(child)`, `withoutBoxedParentMatching(child)`. Presence: `haveBoxedParent()`, `haveNoBoxedParent()`. Facets: `facetByBoxedParentAs(name, child)`, `facetByBoxedParentAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByBoxedParentWith(child)`, `groupByBoxedParentWithDetails()`, `groupByBoxedParentWithDetailsFrom(child)`. Roll-up: `rollUpToBoxedParent()`, `rollUpToBoxedParentWith(child)`.

[Model/action index](../../../ASSIST.md)
