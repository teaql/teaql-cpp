# Field Assist: platform.holder_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalHolderList()`; query view handle: `holderListHandle()`.
- Reverse relation: `holderList()` returns SmartList<Holder>&; query `selectHolderList()`, `selectHolderListWith(HolderRequest<P>)`, `withHolderList(HolderRequest<P>)`; expression `getHolderList()`.

Presence: `haveHolders()`, `haveNoHolders()`. Matching: `withHolderListMatching(child)`, `withoutHolderListMatching(child)`.

Count: `countHolders()`, `countHoldersAs(alias)`, `countHoldersWith(alias, child)`. Statistics/details: `statsFromHolders(child)`, `statsFromHoldersAs(alias, child)`, `groupByHoldersWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
