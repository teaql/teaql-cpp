# Field Assist: platform.entry_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalEntryList()`; query view handle: `entryListHandle()`.
- Reverse relation: `entryList()` returns SmartList<Entry>&; query `selectEntryList()`, `selectEntryListWith(EntryRequest<P>)`, `withEntryList(EntryRequest<P>)`; expression `getEntryList()`.

Presence: `haveEntries()`, `haveNoEntries()`. Matching: `withEntryListMatching(child)`, `withoutEntryListMatching(child)`.

Count: `countEntries()`, `countEntriesAs(alias)`, `countEntriesWith(alias, child)`. Statistics/details: `statsFromEntries(child)`, `statsFromEntriesAs(alias, child)`, `groupByEntriesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
