# Field Assist: platform.json_graph_row_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalJsonGraphRowList()`; query view handle: `jsonGraphRowListHandle()`.
- Reverse relation: `jsonGraphRowList()` returns SmartList<JsonGraphRow>&; query `selectJsonGraphRowList()`, `selectJsonGraphRowListWith(JsonGraphRowRequest<P>)`, `withJsonGraphRowList(JsonGraphRowRequest<P>)`; expression `getJsonGraphRowList()`.

Presence: `haveJsonGraphRows()`, `haveNoJsonGraphRows()`. Matching: `withJsonGraphRowListMatching(child)`, `withoutJsonGraphRowListMatching(child)`.

Count: `countJsonGraphRows()`, `countJsonGraphRowsAs(alias)`, `countJsonGraphRowsWith(alias, child)`. Statistics/details: `statsFromJsonGraphRows(child)`, `statsFromJsonGraphRowsAs(alias, child)`, `groupByJsonGraphRowsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
