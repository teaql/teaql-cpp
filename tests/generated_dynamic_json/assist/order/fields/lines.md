# Field Assist: order.lines

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalLines()`; query view handle: `linesHandle()`.
- Reverse relation: `lines()` returns SmartList<Line>&; query `selectLines()`, `selectLinesWith(LineRequest<P>)`, `withLines(LineRequest<P>)`; expression `getLines()`.

Presence: `haveLines()`, `haveNoLines()`. Matching: `withLinesMatching(child)`, `withoutLinesMatching(child)`.

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
