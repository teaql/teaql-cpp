# Field Assist: platform.status_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalStatusList()`; query view handle: `statusListHandle()`.
- Reverse relation: `statusList()` returns SmartList<Status>&; query `selectStatusList()`, `selectStatusListWith(StatusRequest<P>)`, `withStatusList(StatusRequest<P>)`; expression `getStatusList()`.

Presence: `haveStatuses()`, `haveNoStatuses()`. Matching: `withStatusListMatching(child)`, `withoutStatusListMatching(child)`.

Count: `countStatuses()`, `countStatusesAs(alias)`, `countStatusesWith(alias, child)`. Statistics/details: `statsFromStatuses(child)`, `statsFromStatusesAs(alias, child)`, `groupByStatusesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
