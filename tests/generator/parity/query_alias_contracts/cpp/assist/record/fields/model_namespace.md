# Field Assist: record.namespace

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `namespaceId()` / `updateNamespaceId(const std::optional<std::uint64_t>&)`; changed `changedNamespaceId()`; evaluation `evalNamespaceId()`; expression `getNamespaceId()`.
- Scalar predicates: `withNamespaceIs`, `withNamespaceIsNot`, `withNamespaceGreaterThan`, `withNamespaceGreaterThanOrEqualTo`, `withNamespaceLessThan`, `withNamespaceLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withNamespaceBetween(lower, upper)`, `withNamespaceIn(std::span<const std::uint64_t>)`, `withNamespaceNotIn(std::span<const std::uint64_t>)`, `withNamespaceIsNull()`, `withNamespaceIsNotNull()`.
- Projection/order/group: `selectNamespaceId()`, `orderByNamespaceIdAsc()`, `orderByNamespaceIdDesc()`, `groupByModelNamespace()`.
- Group/count: `groupByModelNamespace()`, `groupByModelNamespaceAs(alias)`, `groupByModelNamespaceWithFunction(alias, function)`, `countModelNamespace()` and `countModelNamespaceAs(alias)`.
- Forward relation: `modelNamespace()` returns const Platform*, `updateModelNamespace(const Platform&)`; query `selectModelNamespace()`, `selectModelNamespaceWith(PlatformRequest<P>)`, `withNamespace(PlatformRequest<P>)`; expression `getModelNamespace()`.

Null alias: `withNamespaceIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withNamespaceIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalModelNamespace()`. Matching: `withNamespaceMatching(child)`, `withoutNamespaceMatching(child)`. Presence: `haveNamespace()`, `haveNoNamespace()`. Facets: `facetByNamespaceAs(name, child)`, `facetByNamespaceAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByNamespaceWith(child)`, `groupByNamespaceWithDetails()`, `groupByNamespaceWithDetailsFrom(child)`. Roll-up: `rollUpToNamespace()`, `rollUpToNamespaceWith(child)`.

[Model/action index](../../../ASSIST.md)
