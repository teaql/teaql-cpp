# Field Assist: serialization_parent.child

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `childId()` / `updateChildId(const std::optional<std::uint64_t>&)`; changed `changedChildId()`; evaluation `evalChildId()`; expression `getChildId()`.
- Scalar predicates: `withChildIs`, `withChildIsNot`, `withChildGreaterThan`, `withChildGreaterThanOrEqualTo`, `withChildLessThan`, `withChildLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withChildBetween(lower, upper)`, `withChildIn(std::span<const std::uint64_t>)`, `withChildNotIn(std::span<const std::uint64_t>)`, `withChildIsNull()`, `withChildIsNotNull()`.
- Projection/order/group: `selectChildId()`, `orderByChildIdAsc()`, `orderByChildIdDesc()`, `groupByChild()`.
- Group/count: `groupByChild()`, `groupByChildAs(alias)`, `groupByChildWithFunction(alias, function)`, `countChild()` and `countChildAs(alias)`.
- Forward relation: `child()` returns const JsonGraphRow*, `updateChild(const JsonGraphRow&)`; query `selectChild()`, `selectChildWith(JsonGraphRowRequest<P>)`, `withChild(JsonGraphRowRequest<P>)`; expression `getChild()`.

Null alias: `withChildIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withChildIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalChild()`. Matching: `withChildMatching(child)`, `withoutChildMatching(child)`. Presence: `haveChild()`, `haveNoChild()`. Facets: `facetByChildAs(name, child)`, `facetByChildAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByChildWith(child)`, `groupByChildWithDetails()`, `groupByChildWithDetailsFrom(child)`. Roll-up: `rollUpToChild()`, `rollUpToChildWith(child)`.

[Model/action index](../../../ASSIST.md)
