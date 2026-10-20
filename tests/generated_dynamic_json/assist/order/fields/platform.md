# Field Assist: order.platform

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `platformId()`; Context-root assignment is managed through UserContext; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `withPlatformIs`, `withPlatformIsNot`, `withPlatformGreaterThan`, `withPlatformGreaterThanOrEqualTo`, `withPlatformLessThan`, `withPlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPlatformBetween(lower, upper)`, `withPlatformIn(std::span<const std::uint64_t>)`, `withPlatformNotIn(std::span<const std::uint64_t>)`, `withPlatformIsNull()`, `withPlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*; Context-root assignment is managed through UserContext; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `withPlatform(PlatformRequest<P>)`; expression `getPlatform()`.

Null alias: `withPlatformIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPlatformIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalPlatform()`. Matching: `withPlatformMatching(child)`, `withoutPlatformMatching(child)`. Presence: `havePlatform()`, `haveNoPlatform()`. Facets: `facetByPlatformAs(name, child)`, `facetByPlatformAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByPlatformWith(child)`, `groupByPlatformWithDetails()`, `groupByPlatformWithDetailsFrom(child)`. Roll-up: `rollUpToPlatform()`, `rollUpToPlatformWith(child)`.

[Model/action index](../../../ASSIST.md)
