# Field Assist: entry.operator

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `operatorId()` / `updateOperatorId(const std::optional<std::uint64_t>&)`; changed `changedOperatorId()`; evaluation `evalOperatorId()`; expression `getOperatorId()`.
- Scalar predicates: `withOperatorIs`, `withOperatorIsNot`, `withOperatorGreaterThan`, `withOperatorGreaterThanOrEqualTo`, `withOperatorLessThan`, `withOperatorLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withOperatorBetween(lower, upper)`, `withOperatorIn(std::span<const std::uint64_t>)`, `withOperatorNotIn(std::span<const std::uint64_t>)`, `withOperatorIsNull()`, `withOperatorIsNotNull()`.
- Projection/order/group: `selectOperatorId()`, `orderByOperatorIdAsc()`, `orderByOperatorIdDesc()`, `groupByModelOperator()`.
- Group/count: `groupByModelOperator()`, `groupByModelOperatorAs(alias)`, `groupByModelOperatorWithFunction(alias, function)`, `countModelOperator()` and `countModelOperatorAs(alias)`.
- Forward relation: `modelOperator()` returns const Owner*, `updateModelOperator(const Owner&)`; query `selectModelOperator()`, `selectModelOperatorWith(OwnerRequest<P>)`, `withOperator(OwnerRequest<P>)`; expression `getModelOperator()`.

Null alias: `withOperatorIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withOperatorIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalModelOperator()`. Matching: `withOperatorMatching(child)`, `withoutOperatorMatching(child)`. Presence: `haveOperator()`, `haveNoOperator()`. Facets: `facetByOperatorAs(name, child)`, `facetByOperatorAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByOperatorWith(child)`, `groupByOperatorWithDetails()`, `groupByOperatorWithDetailsFrom(child)`. Roll-up: `rollUpToOperator()`, `rollUpToOperatorWith(child)`.

[Model/action index](../../../ASSIST.md)
