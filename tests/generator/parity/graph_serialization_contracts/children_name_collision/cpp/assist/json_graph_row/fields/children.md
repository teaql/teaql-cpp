# Field Assist: json_graph_row.children

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalChildren()`; query view handle: `childrenHandle()`.
- Reverse relation: `children()` returns SmartList<JsonGraphRow>&; query `selectChildren()`, `selectChildrenWith(JsonGraphRowRequest<P>)`, `withChildren(JsonGraphRowRequest<P>)`; expression `getChildren()`.

Presence: `haveChildren()`, `haveNoChildren()`. Matching: `withChildrenMatching(child)`, `withoutChildrenMatching(child)`.

Count: `countChildren()`, `countChildrenAs(alias)`, `countChildrenWith(alias, child)`. Statistics/details: `statsFromChildren(child)`, `statsFromChildrenAs(alias, child)`, `groupByChildrenWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
