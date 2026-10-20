# Field Assist: research_academy.advisor

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `advisorId()` / `updateAdvisorId(const std::optional<std::uint64_t>&)`; changed `changedAdvisorId()`; evaluation `evalAdvisorId()`; expression `getAdvisorId()`.
- Scalar predicates: `withAdvisorIs`, `withAdvisorIsNot`, `withAdvisorGreaterThan`, `withAdvisorGreaterThanOrEqualTo`, `withAdvisorLessThan`, `withAdvisorLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withAdvisorBetween(lower, upper)`, `withAdvisorIn(std::span<const std::uint64_t>)`, `withAdvisorNotIn(std::span<const std::uint64_t>)`, `withAdvisorIsNull()`, `withAdvisorIsNotNull()`.
- Projection/order/group: `selectAdvisorId()`, `orderByAdvisorIdAsc()`, `orderByAdvisorIdDesc()`, `groupByAdvisor()`.
- Group/count: `groupByAdvisor()`, `groupByAdvisorAs(alias)`, `groupByAdvisorWithFunction(alias, function)`, `countAdvisor()` and `countAdvisorAs(alias)`.
- Forward relation: `advisor()` returns const Specialist*, `updateAdvisor(const Specialist&)`; query `selectAdvisor()`, `selectAdvisorWith(SpecialistRequest<P>)`, `withAdvisor(SpecialistRequest<P>)`; expression `getAdvisor()`.

Null alias: `withAdvisorIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withAdvisorIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalAdvisor()`. Matching: `withAdvisorMatching(child)`, `withoutAdvisorMatching(child)`. Presence: `haveAdvisor()`, `haveNoAdvisor()`. Facets: `facetByAdvisorAs(name, child)`, `facetByAdvisorAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByAdvisorWith(child)`, `groupByAdvisorWithDetails()`, `groupByAdvisorWithDetailsFrom(child)`. Roll-up: `rollUpToAdvisor()`, `rollUpToAdvisorWith(child)`.

[Model/action index](../../../ASSIST.md)
