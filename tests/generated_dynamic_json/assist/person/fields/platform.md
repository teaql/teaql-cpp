# Field Assist: person.platform

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `platformId()`; Context-root assignment is managed through UserContext; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `whosePlatformIs`, `whosePlatformIsNot`, `whosePlatformGreaterThan`, `whosePlatformGreaterThanOrEqualTo`, `whosePlatformLessThan`, `whosePlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `whosePlatformBetween(lower, upper)`, `whosePlatformIn(std::span<const std::uint64_t>)`, `whosePlatformNotIn(std::span<const std::uint64_t>)`, `whosePlatformIsNull()`, `whosePlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*; Context-root assignment is managed through UserContext; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `whosePlatform(PlatformRequest<P>)`; expression `getPlatform()`.

Null alias: `whosePlatformIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whosePlatformIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalPlatform()`. Matching: `withPlatformMatching(child)`, `withoutPlatformMatching(child)`. Presence: `havePlatform()`, `haveNoPlatform()`. Facets: `facetByPlatformAs(name, child)`, `facetByPlatformAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByPlatformWith(child)`, `groupByPlatformWithDetails()`, `groupByPlatformWithDetailsFrom(child)`. Roll-up: `rollUpToPlatform()`, `rollUpToPlatformWith(child)`.

[Model/action index](../../../ASSIST.md)
