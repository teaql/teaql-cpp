# Field Assist: platform.serialization_member_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSerializationMemberList()`; query view handle: `serializationMemberListHandle()`.
- Reverse relation: `serializationMemberList()` returns SmartList<SerializationMember>&; query `selectSerializationMemberList()`, `selectSerializationMemberListWith(SerializationMemberRequest<P>)`, `withSerializationMemberList(SerializationMemberRequest<P>)`; expression `getSerializationMemberList()`.

Presence: `haveSerializationMembers()`, `haveNoSerializationMembers()`. Matching: `withSerializationMemberListMatching(child)`, `withoutSerializationMemberListMatching(child)`.

Count: `countSerializationMembers()`, `countSerializationMembersAs(alias)`, `countSerializationMembersWith(alias, child)`. Statistics/details: `statsFromSerializationMembers(child)`, `statsFromSerializationMembersAs(alias, child)`, `groupBySerializationMembersWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
