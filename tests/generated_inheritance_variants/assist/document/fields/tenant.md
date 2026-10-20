# Field Assist: document.tenant

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `tenantId()`; Context-root assignment is managed through UserContext; changed `changedTenantId()`; evaluation `evalTenantId()`; expression `getTenantId()`.
- Scalar predicates: `withTenantIs`, `withTenantIsNot`, `withTenantGreaterThan`, `withTenantGreaterThanOrEqualTo`, `withTenantLessThan`, `withTenantLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withTenantBetween(lower, upper)`, `withTenantIn(std::span<const std::uint64_t>)`, `withTenantNotIn(std::span<const std::uint64_t>)`, `withTenantIsNull()`, `withTenantIsNotNull()`.
- Projection/order/group: `selectTenantId()`, `orderByTenantIdAsc()`, `orderByTenantIdDesc()`, `groupByTenant()`.
- Group/count: `groupByTenant()`, `groupByTenantAs(alias)`, `groupByTenantWithFunction(alias, function)`, `countTenant()` and `countTenantAs(alias)`.
- Forward relation: `tenant()` returns const Tenant*; Context-root assignment is managed through UserContext; query `selectTenant()`, `selectTenantWith(TenantRequest<P>)`, `withTenant(TenantRequest<P>)`; expression `getTenant()`.

Null alias: `withTenantIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withTenantIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalTenant()`. Matching: `withTenantMatching(child)`, `withoutTenantMatching(child)`. Presence: `haveTenant()`, `haveNoTenant()`. Facets: `facetByTenantAs(name, child)`, `facetByTenantAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByTenantWith(child)`, `groupByTenantWithDetails()`, `groupByTenantWithDetailsFrom(child)`. Roll-up: `rollUpToTenant()`, `rollUpToTenantWith(child)`.

[Model/action index](../../../ASSIST.md)
