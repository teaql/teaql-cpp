# Field Assist: category.parent

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `parentId()`; constant ID assignment is internal; changed `changedParentId()`; evaluation `evalParentId()`; expression `getParentId()`.
- Candidate filters: `withParentIsC1()` and `withParentIsNotC1()`; exact value `1`.
- Candidate model update: `updateParentToC1()`; model and E check: `parentIsC1()`. E preserves NotLoaded and never performs SQL.
- Candidate filters: `withParentIsC22()` and `withParentIsNotC22()`; exact value `22`.
- Candidate model update: `updateParentToC22()`; model and E check: `parentIsC22()`. E preserves NotLoaded and never performs SQL.
- Scalar predicates: `withParentIs`, `withParentIsNot`, `withParentGreaterThan`, `withParentGreaterThanOrEqualTo`, `withParentLessThan`, `withParentLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withParentBetween(lower, upper)`, `withParentIn(std::span<const std::uint64_t>)`, `withParentNotIn(std::span<const std::uint64_t>)`, `withParentIsNull()`, `withParentIsNotNull()`.
- Projection/order/group: `selectParentId()`, `orderByParentIdAsc()`, `orderByParentIdDesc()`, `groupByParent()`.
- Group/count: `groupByParent()`, `groupByParentAs(alias)`, `groupByParentWithFunction(alias, function)`, `countParent()` and `countParentAs(alias)`.
- Forward relation: `parent()` returns const Category*; constant assignment uses candidate update methods; query `selectParent()`, `selectParentWith(CategoryRequest<P>)`, `withParent(CategoryRequest<P>)`; expression `getParent()`.

Null alias: `withParentIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withParentIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalParent()`. Matching: `withParentMatching(child)`, `withoutParentMatching(child)`. Presence: `haveParent()`, `haveNoParent()`. Facets: `facetByParentAs(name, child)`, `facetByParentAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByParentWith(child)`, `groupByParentWithDetails()`, `groupByParentWithDetailsFrom(child)`. Roll-up: `rollUpToParent()`, `rollUpToParentWith(child)`.

[Model/action index](../../../ASSIST.md)
