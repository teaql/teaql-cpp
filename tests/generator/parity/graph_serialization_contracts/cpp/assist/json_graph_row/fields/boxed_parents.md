# Field Assist: json_graph_row.boxed_parents

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalBoxedParents()`; query view handle: `boxedParentsHandle()`.
- Reverse relation: `boxedParents()` returns SmartList<SerializationParent>&; query `selectBoxedParents()`, `selectBoxedParentsWith(SerializationParentRequest<P>)`, `withBoxedParents(SerializationParentRequest<P>)`; expression `getBoxedParents()`.

Presence: `haveBoxedParents()`, `haveNoBoxedParents()`. Matching: `withBoxedParentsMatching(child)`, `withoutBoxedParentsMatching(child)`.

Count: `countBoxedParents()`, `countBoxedParentsAs(alias)`, `countBoxedParentsWith(alias, child)`. Statistics/details: `statsFromBoxedParents(child)`, `statsFromBoxedParentsAs(alias, child)`, `groupByBoxedParentsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
