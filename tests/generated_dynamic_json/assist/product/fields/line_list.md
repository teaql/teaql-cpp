# Field Assist: product.line_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalLineList()`; query view handle: `lineListHandle()`.
- Reverse relation: `lineList()` returns SmartList<Line>&; query `selectLineList()`, `selectLineListWith(LineRequest<P>)`, `withLineList(LineRequest<P>)`; expression `getLineList()`.

Presence: `haveLines()`, `haveNoLines()`. Matching: `withLineListMatching(child)`, `withoutLineListMatching(child)`.

Count: `countLines()`, `countLinesAs(alias)`, `countLinesWith(alias, child)`. Statistics/details: `statsFromLines(child)`, `statsFromLinesAs(alias, child)`, `groupByLinesWithDetails(child)`.

Field statistic: `sumQuantityOfLines()`, `sumQuantityOfLinesAs(alias, child)`.

Field statistic: `minQuantityOfLines()`, `minQuantityOfLinesAs(alias, child)`.

Field statistic: `maxQuantityOfLines()`, `maxQuantityOfLinesAs(alias, child)`.

Field statistic: `avgQuantityOfLines()`, `avgQuantityOfLinesAs(alias, child)`.

Field statistic: `standardDeviationQuantityOfLines()`, `standardDeviationQuantityOfLinesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationQuantityOfLines()`, `squareRootOfPopulationStandardDeviationQuantityOfLinesAs(alias, child)`.

Field statistic: `sampleVarianceQuantityOfLines()`, `sampleVarianceQuantityOfLinesAs(alias, child)`.

Field statistic: `samplePopulationVarianceQuantityOfLines()`, `samplePopulationVarianceQuantityOfLinesAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
