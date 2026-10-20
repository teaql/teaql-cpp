# Field Assist: json_graph_row.parents

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalParents()`; query view handle: `parentsHandle()`.
- Reverse relation: `parents()` returns SmartList<SerializationParent>&; query `selectParents()`, `selectParentsWith(SerializationParentRequest<P>)`, `withParents(SerializationParentRequest<P>)`; expression `getParents()`.

Presence: `haveParents()`, `haveNoParents()`. Matching: `withParentsMatching(child)`, `withoutParentsMatching(child)`.

Count: `countParents()`, `countParentsAs(alias)`, `countParentsWith(alias, child)`. Statistics/details: `statsFromParents(child)`, `statsFromParentsAs(alias, child)`, `groupByParentsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
