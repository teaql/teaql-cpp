# Field Assist: person.status

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `statusId()`; constant ID assignment is internal; changed `changedStatusId()`; evaluation `evalStatusId()`; expression `getStatusId()`.
- Candidate filters: `whoseStatusIsDraft()` and `whoseStatusIsNotDraft()`; exact value `11`.
- Candidate model update: `updateStatusToDraft()`; model and E check: `statusIsDraft()`. E preserves NotLoaded and never performs SQL.
- Candidate filters: `whoseStatusIsInProgress()` and `whoseStatusIsNotInProgress()`; exact value `22`.
- Candidate model update: `updateStatusToInProgress()`; model and E check: `statusIsInProgress()`. E preserves NotLoaded and never performs SQL.
- Candidate filters: `whoseStatusIsDone()` and `whoseStatusIsNotDone()`; exact value `2104194`.
- Candidate model update: `updateStatusToDone()`; model and E check: `statusIsDone()`. E preserves NotLoaded and never performs SQL.
- Scalar predicates: `whoseStatusIs`, `whoseStatusIsNot`, `whoseStatusGreaterThan`, `whoseStatusGreaterThanOrEqualTo`, `whoseStatusLessThan`, `whoseStatusLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `whoseStatusBetween(lower, upper)`, `whoseStatusIn(std::span<const std::uint64_t>)`, `whoseStatusNotIn(std::span<const std::uint64_t>)`, `whoseStatusIsNull()`, `whoseStatusIsNotNull()`.
- Projection/order/group: `selectStatusId()`, `orderByStatusIdAsc()`, `orderByStatusIdDesc()`, `groupByStatus()`.
- Group/count: `groupByStatus()`, `groupByStatusAs(alias)`, `groupByStatusWithFunction(alias, function)`, `countStatus()` and `countStatusAs(alias)`.
- Forward relation: `status()` returns const OrderStatus*; constant assignment uses candidate update methods; query `selectStatus()`, `selectStatusWith(OrderStatusRequest<P>)`, `whoseStatus(OrderStatusRequest<P>)`; expression `getStatus()`.

Null alias: `whoseStatusIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `whoseStatusIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalStatus()`. Matching: `withStatusMatching(child)`, `withoutStatusMatching(child)`. Presence: `haveStatus()`, `haveNoStatus()`. Facets: `facetByStatusAs(name, child)`, `facetByStatusAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByStatusWith(child)`, `groupByStatusWithDetails()`, `groupByStatusWithDetailsFrom(child)`. Roll-up: `rollUpToStatus()`, `rollUpToStatusWith(child)`.

[Model/action index](../../../ASSIST.md)
