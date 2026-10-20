# Field Assist: platform.serialization_parent_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSerializationParentList()`; query view handle: `serializationParentListHandle()`.
- Reverse relation: `serializationParentList()` returns SmartList<SerializationParent>&; query `selectSerializationParentList()`, `selectSerializationParentListWith(SerializationParentRequest<P>)`, `withSerializationParentList(SerializationParentRequest<P>)`; expression `getSerializationParentList()`.

Presence: `haveSerializationParents()`, `haveNoSerializationParents()`. Matching: `withSerializationParentListMatching(child)`, `withoutSerializationParentListMatching(child)`.

Count: `countSerializationParents()`, `countSerializationParentsAs(alias)`, `countSerializationParentsWith(alias, child)`. Statistics/details: `statsFromSerializationParents(child)`, `statsFromSerializationParentsAs(alias, child)`, `groupBySerializationParentsWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
