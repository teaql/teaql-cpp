# Field Assist: serialization_parent.boxed_children

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalBoxedChildren()`; query view handle: `boxedChildrenHandle()`.
- Reverse relation: `boxedChildren()` returns SmartList<SerializationMember>&; query `selectBoxedChildren()`, `selectBoxedChildrenWith(SerializationMemberRequest<P>)`, `withBoxedChildren(SerializationMemberRequest<P>)`; expression `getBoxedChildren()`.

Presence: `haveBoxedChildren()`, `haveNoBoxedChildren()`. Matching: `withBoxedChildrenMatching(child)`, `withoutBoxedChildrenMatching(child)`.

Count: `countBoxedChildren()`, `countBoxedChildrenAs(alias)`, `countBoxedChildrenWith(alias, child)`. Statistics/details: `statsFromBoxedChildren(child)`, `statsFromBoxedChildrenAs(alias, child)`, `groupByBoxedChildrenWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
