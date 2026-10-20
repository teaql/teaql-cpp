# Field Assist: order.seller

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `sellerId()` / `updateSellerId(const std::uint64_t&)`; changed `changedSellerId()`; evaluation `evalSellerId()`; expression `getSellerId()`.
- Scalar predicates: `withSellerIs`, `withSellerIsNot`, `withSellerGreaterThan`, `withSellerGreaterThanOrEqualTo`, `withSellerLessThan`, `withSellerLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withSellerBetween(lower, upper)`, `withSellerIn(std::span<const std::uint64_t>)`, `withSellerNotIn(std::span<const std::uint64_t>)`, `withSellerIsNull()`, `withSellerIsNotNull()`.
- Projection/order/group: `selectSellerId()`, `orderBySellerIdAsc()`, `orderBySellerIdDesc()`, `groupBySeller()`.
- Group/count: `groupBySeller()`, `groupBySellerAs(alias)`, `groupBySellerWithFunction(alias, function)`, `countSeller()` and `countSellerAs(alias)`.
- Forward relation: `seller()` returns const Company*, `updateSeller(const Company&)`; query `selectSeller()`, `selectSellerWith(CompanyRequest<P>)`, `withSeller(CompanyRequest<P>)`; expression `getSeller()`.

Null alias: `withSellerIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withSellerIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalSeller()`. Matching: `withSellerMatching(child)`, `withoutSellerMatching(child)`. Presence: `haveSeller()`, `haveNoSeller()`. Facets: `facetBySellerAs(name, child)`, `facetBySellerAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupBySellerWith(child)`, `groupBySellerWithDetails()`, `groupBySellerWithDetailsFrom(child)`. Roll-up: `rollUpToSeller()`, `rollUpToSellerWith(child)`.

[Model/action index](../../../ASSIST.md)
