# Field Assist: label.priority

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `priorityId()`; constant ID assignment is internal; changed `changedPriorityId()`; evaluation `evalPriorityId()`; expression `getPriorityId()`.
- Candidate filters: `withPriorityIsBasic()` and `withPriorityIsNotBasic()`; exact value `1`.
- Candidate model update: `updatePriorityToBasic()`; model and E check: `priorityIsBasic()`. E preserves NotLoaded and never performs SQL.
- Candidate filters: `withPriorityIsExtended()` and `withPriorityIsNotExtended()`; exact value `22`.
- Candidate model update: `updatePriorityToExtended()`; model and E check: `priorityIsExtended()`. E preserves NotLoaded and never performs SQL.
- Scalar predicates: `withPriorityIs`, `withPriorityIsNot`, `withPriorityGreaterThan`, `withPriorityGreaterThanOrEqualTo`, `withPriorityLessThan`, `withPriorityLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPriorityBetween(lower, upper)`, `withPriorityIn(std::span<const std::uint64_t>)`, `withPriorityNotIn(std::span<const std::uint64_t>)`, `withPriorityIsNull()`, `withPriorityIsNotNull()`.
- Projection/order/group: `selectPriorityId()`, `orderByPriorityIdAsc()`, `orderByPriorityIdDesc()`, `groupByPriority()`.
- Group/count: `groupByPriority()`, `groupByPriorityAs(alias)`, `groupByPriorityWithFunction(alias, function)`, `countPriority()` and `countPriorityAs(alias)`.
- Forward relation: `priority()` returns const Priority*; constant assignment uses candidate update methods; query `selectPriority()`, `selectPriorityWith(PriorityRequest<P>)`, `withPriority(PriorityRequest<P>)`; expression `getPriority()`.

Null alias: `withPriorityIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPriorityIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalPriority()`. Matching: `withPriorityMatching(child)`, `withoutPriorityMatching(child)`. Presence: `havePriority()`, `haveNoPriority()`. Facets: `facetByPriorityAs(name, child)`, `facetByPriorityAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByPriorityWith(child)`, `groupByPriorityWithDetails()`, `groupByPriorityWithDetailsFrom(child)`. Roll-up: `rollUpToPriority()`, `rollUpToPriorityWith(child)`.

[Model/action index](../../../ASSIST.md)
