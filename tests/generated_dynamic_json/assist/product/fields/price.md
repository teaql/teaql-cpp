# Field Assist: product.price

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `price()` / `updatePrice(const Decimal&)`; changed `changedPrice()`; evaluation `evalPrice()`; expression `getPrice()`.
- Scalar predicates: `withPriceIs`, `withPriceIsNot`, `withPriceGreaterThan`, `withPriceGreaterThanOrEqualTo`, `withPriceLessThan`, `withPriceLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withPriceBetween(lower, upper)`, `withPriceIn(std::span<const Decimal>)`, `withPriceNotIn(std::span<const Decimal>)`, `withPriceIsNull()`, `withPriceIsNotNull()`.
- Projection/order/group: `selectPrice()`, `orderByPriceAsc()`, `orderByPriceDesc()`, `groupByPrice()`.
- Group/count: `groupByPrice()`, `groupByPriceAs(alias)`, `groupByPriceWithFunction(alias, function)`, `countPrice()` and `countPriceAs(alias)`.
- Scalar aggregate: `sumPrice()` uses alias `"sum_price"`; `sumPriceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPrice()` uses alias `"avg_price"`; `avgPriceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPrice()` uses alias `"min_price"`; `minPriceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPrice()` uses alias `"max_price"`; `maxPriceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationPrice()` and `standardDeviationPriceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationPrice()` and `squareRootOfPopulationStandardDeviationPriceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVariancePrice()` and `sampleVariancePriceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVariancePrice()` and `samplePopulationVariancePriceAs(alias)`; SQLite requires the corresponding SQL function.

Null alias: `withPriceIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withPriceIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

[Model/action index](../../../ASSIST.md)
