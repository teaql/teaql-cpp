# Field Assist: product.sku

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `sku()` / `updateSku(std::string_view)`; changed `changedSku()`; evaluation `evalSku()`; expression `getSku()`.
- Scalar predicates: `withSkuIs`, `withSkuIsNot`, `withSkuGreaterThan`, `withSkuGreaterThanOrEqualTo`, `withSkuLessThan`, `withSkuLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withSkuBetween(lower, upper)`, `withSkuIn(std::span<const String>)`, `withSkuNotIn(std::span<const String>)`, `withSkuIsNull()`, `withSkuIsNotNull()`.
- Projection/order/group: `selectSku()`, `orderBySkuAsc()`, `orderBySkuDesc()`, `groupBySku()`.
- Group/count: `groupBySku()`, `groupBySkuAs(alias)`, `groupBySkuWithFunction(alias, function)`, `countSku()` and `countSkuAs(alias)`.
- Scalar aggregate: `sumSku()` uses alias `"sum_sku"`; `sumSkuAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgSku()` uses alias `"avg_sku"`; `avgSkuAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minSku()` uses alias `"min_sku"`; `minSkuAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxSku()` uses alias `"max_sku"`; `maxSkuAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withSkuContaining(text)`, `withSkuStartingWith(text)`, `withSkuEndingWith(text)`.

Null alias: `withSkuIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withSkuIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Text predicate: `withSkuNotContaining(value)`.

Text predicate: `withSkuNotStartingWith(value)`.

Text predicate: `withSkuNotEndingWith(value)`.

Text predicate: `withSkuSoundingLike(value)`.

[Model/action index](../../../ASSIST.md)
