# Field Assist: platform.product_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalProductList()`; query view handle: `productListHandle()`.
- Reverse relation: `productList()` returns SmartList<Product>&; query `selectProductList()`, `selectProductListWith(ProductRequest<P>)`, `withProductList(ProductRequest<P>)`; expression `getProductList()`.

Presence: `haveProducts()`, `haveNoProducts()`. Matching: `withProductListMatching(child)`, `withoutProductListMatching(child)`.

Count: `countProducts()`, `countProductsAs(alias)`, `countProductsWith(alias, child)`. Statistics/details: `statsFromProducts(child)`, `statsFromProductsAs(alias, child)`, `groupByProductsWithDetails(child)`.

Field statistic: `sumPriceOfProducts()`, `sumPriceOfProductsAs(alias, child)`.

Field statistic: `minPriceOfProducts()`, `minPriceOfProductsAs(alias, child)`.

Field statistic: `maxPriceOfProducts()`, `maxPriceOfProductsAs(alias, child)`.

Field statistic: `avgPriceOfProducts()`, `avgPriceOfProductsAs(alias, child)`.

Field statistic: `standardDeviationPriceOfProducts()`, `standardDeviationPriceOfProductsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationPriceOfProducts()`, `squareRootOfPopulationStandardDeviationPriceOfProductsAs(alias, child)`.

Field statistic: `sampleVariancePriceOfProducts()`, `sampleVariancePriceOfProductsAs(alias, child)`.

Field statistic: `samplePopulationVariancePriceOfProducts()`, `samplePopulationVariancePriceOfProductsAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
