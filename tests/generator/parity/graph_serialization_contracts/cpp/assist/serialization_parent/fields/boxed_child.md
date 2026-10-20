# Field Assist: serialization_parent.boxed_child

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `boxedChildId()` / `updateBoxedChildId(const std::optional<std::uint64_t>&)`; changed `changedBoxedChildId()`; evaluation `evalBoxedChildId()`; expression `getBoxedChildId()`.
- Scalar predicates: `withBoxedChildIs`, `withBoxedChildIsNot`, `withBoxedChildGreaterThan`, `withBoxedChildGreaterThanOrEqualTo`, `withBoxedChildLessThan`, `withBoxedChildLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withBoxedChildBetween(lower, upper)`, `withBoxedChildIn(std::span<const std::uint64_t>)`, `withBoxedChildNotIn(std::span<const std::uint64_t>)`, `withBoxedChildIsNull()`, `withBoxedChildIsNotNull()`.
- Projection/order/group: `selectBoxedChildId()`, `orderByBoxedChildIdAsc()`, `orderByBoxedChildIdDesc()`, `groupByBoxedChild()`.
- Group/count: `groupByBoxedChild()`, `groupByBoxedChildAs(alias)`, `groupByBoxedChildWithFunction(alias, function)`, `countBoxedChild()` and `countBoxedChildAs(alias)`.
- Forward relation: `boxedChild()` returns const JsonGraphRow*, `updateBoxedChild(const JsonGraphRow&)`; query `selectBoxedChild()`, `selectBoxedChildWith(JsonGraphRowRequest<P>)`, `withBoxedChild(JsonGraphRowRequest<P>)`; expression `getBoxedChild()`.

Null alias: `withBoxedChildIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withBoxedChildIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalBoxedChild()`. Matching: `withBoxedChildMatching(child)`, `withoutBoxedChildMatching(child)`. Presence: `haveBoxedChild()`, `haveNoBoxedChild()`. Facets: `facetByBoxedChildAs(name, child)`, `facetByBoxedChildAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByBoxedChildWith(child)`, `groupByBoxedChildWithDetails()`, `groupByBoxedChildWithDetailsFrom(child)`. Roll-up: `rollUpToBoxedChild()`, `rollUpToBoxedChildWith(child)`.

[Model/action index](../../../ASSIST.md)
