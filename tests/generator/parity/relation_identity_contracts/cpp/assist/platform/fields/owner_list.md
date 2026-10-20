# Field Assist: platform.owner_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalOwnerList()`; query view handle: `ownerListHandle()`.
- Reverse relation: `ownerList()` returns SmartList<Owner>&; query `selectOwnerList()`, `selectOwnerListWith(OwnerRequest<P>)`, `withOwnerList(OwnerRequest<P>)`; expression `getOwnerList()`.

Presence: `haveOwners()`, `haveNoOwners()`. Matching: `withOwnerListMatching(child)`, `withoutOwnerListMatching(child)`.

Count: `countOwners()`, `countOwnersAs(alias)`, `countOwnersWith(alias, child)`. Statistics/details: `statsFromOwners(child)`, `statsFromOwnersAs(alias, child)`, `groupByOwnersWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
