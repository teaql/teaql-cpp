# Field Assist: serialization_member.parent

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `parentId()` / `updateParentId(const std::optional<std::uint64_t>&)`; changed `changedParentId()`; evaluation `evalParentId()`; expression `getParentId()`.
- Scalar predicates: `withParentIs`, `withParentIsNot`, `withParentGreaterThan`, `withParentGreaterThanOrEqualTo`, `withParentLessThan`, `withParentLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withParentBetween(lower, upper)`, `withParentIn(std::span<const std::uint64_t>)`, `withParentNotIn(std::span<const std::uint64_t>)`, `withParentIsNull()`, `withParentIsNotNull()`.
- Projection/order/group: `selectParentId()`, `orderByParentIdAsc()`, `orderByParentIdDesc()`, `groupByParent()`.
- Group/count: `groupByParent()`, `groupByParentAs(alias)`, `groupByParentWithFunction(alias, function)`, `countParent()` and `countParentAs(alias)`.
- Forward relation: `parent()` returns const SerializationParent*, `updateParent(const SerializationParent&)`; query `selectParent()`, `selectParentWith(SerializationParentRequest<P>)`, `withParent(SerializationParentRequest<P>)`; expression `getParent()`.

Null alias: `withParentIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withParentIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalParent()`. Matching: `withParentMatching(child)`, `withoutParentMatching(child)`. Presence: `haveParent()`, `haveNoParent()`. Facets: `facetByParentAs(name, child)`, `facetByParentAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByParentWith(child)`, `groupByParentWithDetails()`, `groupByParentWithDetailsFrom(child)`. Roll-up: `rollUpToParent()`, `rollUpToParentWith(child)`.

[Model/action index](../../../ASSIST.md)
