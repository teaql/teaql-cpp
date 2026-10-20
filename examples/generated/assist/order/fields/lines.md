# Field Assist: order.lines

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalLines()`; query view handle: `linesHandle()`.
- Reverse relation: `lines()` returns SmartList<OrderLine>&; query `selectLines()`, `selectLinesWith(OrderLineRequest<P>)`, `withLines(OrderLineRequest<P>)`; expression `getLines()`.

Presence: `haveLines()`, `haveNoLines()`. Matching: `withLinesMatching(child)`, `withoutLinesMatching(child)`.

Count: `countLines()`, `countLinesAs(alias)`, `countLinesWith(alias, child)`. Statistics/details: `statsFromLines(child)`, `statsFromLinesAs(alias, child)`, `groupByLinesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
