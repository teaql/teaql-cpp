# Field Assist: json_graph_row.child_rows

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalChildRows()`; query view handle: `childRowsHandle()`.
- Reverse relation: `childRows()` returns SmartList<JsonGraphRow>&; query `selectChildRows()`, `selectChildRowsWith(JsonGraphRowRequest<P>)`, `withChildRows(JsonGraphRowRequest<P>)`; expression `getChildRows()`.

Presence: `haveChildRows()`, `haveNoChildRows()`. Matching: `withChildRowsMatching(child)`, `withoutChildRowsMatching(child)`.

Count: `countChildRows()`, `countChildRowsAs(alias)`, `countChildRowsWith(alias, child)`. Statistics/details: `statsFromChildRows(child)`, `statsFromChildRowsAs(alias, child)`, `groupByChildRowsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
