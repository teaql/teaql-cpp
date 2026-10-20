# Field Assist: order.customer

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `customerId()` / `updateCustomerId(const std::uint64_t&)`; changed `changedCustomerId()`; evaluation `evalCustomerId()`; expression `getCustomerId()`.
- Scalar predicates: `withCustomerIs`, `withCustomerIsNot`, `withCustomerGreaterThan`, `withCustomerGreaterThanOrEqualTo`, `withCustomerLessThan`, `withCustomerLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withCustomerBetween(lower, upper)`, `withCustomerIn(std::span<const std::uint64_t>)`, `withCustomerNotIn(std::span<const std::uint64_t>)`, `withCustomerIsNull()`, `withCustomerIsNotNull()`.
- Projection/order/group: `selectCustomerId()`, `orderByCustomerIdAsc()`, `orderByCustomerIdDesc()`, `groupByCustomer()`.
- Group/count: `groupByCustomer()`, `groupByCustomerAs(alias)`, `groupByCustomerWithFunction(alias, function)`, `countCustomer()` and `countCustomerAs(alias)`.
- Forward relation: `customer()` returns const Person*, `updateCustomer(const Person&)`; query `selectCustomer()`, `selectCustomerWith(PersonRequest<P>)`, `withCustomer(PersonRequest<P>)`; expression `getCustomer()`.

Null alias: `withCustomerIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCustomerIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalCustomer()`. Matching: `withCustomerMatching(child)`, `withoutCustomerMatching(child)`. Presence: `haveCustomer()`, `haveNoCustomer()`. Facets: `facetByCustomerAs(name, child)`, `facetByCustomerAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByCustomerWith(child)`, `groupByCustomerWithDetails()`, `groupByCustomerWithDetailsFrom(child)`. Roll-up: `rollUpToCustomer()`, `rollUpToCustomerWith(child)`.

[Model/action index](../../../ASSIST.md)
