# Field Assist: order_line.product

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `productId()` / `updateProductId(const std::uint64_t&)`; changed `changedProductId()`; evaluation `evalProductId()`; expression `getProductId()`.
- Scalar predicates: `withProductIs`, `withProductIsNot`, `withProductGreaterThan`, `withProductGreaterThanOrEqualTo`, `withProductLessThan`, `withProductLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withProductBetween(lower, upper)`, `withProductIn(std::span<const std::uint64_t>)`, `withProductNotIn(std::span<const std::uint64_t>)`, `withProductIsNull()`, `withProductIsNotNull()`.
- Projection/order/group: `selectProductId()`, `orderByProductIdAsc()`, `orderByProductIdDesc()`, `groupByProduct()`.
- Group/count: `groupByProduct()`, `groupByProductAs(alias)`, `groupByProductWithFunction(alias, function)`, `countProduct()` and `countProductAs(alias)`.
- Forward relation: `product()` returns const Product*, `updateProduct(const Product&)`; query `selectProduct()`, `selectProductWith(ProductRequest<P>)`, `withProduct(ProductRequest<P>)`; expression `getProduct()`.

Null alias: `withProductIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withProductIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalProduct()`. Matching: `withProductMatching(child)`, `withoutProductMatching(child)`. Presence: `haveProduct()`, `haveNoProduct()`. Facets: `facetByProductAs(name, child)`, `facetByProductAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByProductWith(child)`, `groupByProductWithDetails()`, `groupByProductWithDetailsFrom(child)`. Roll-up: `rollUpToProduct()`, `rollUpToProductWith(child)`.

[Model/action index](../../../ASSIST.md)
