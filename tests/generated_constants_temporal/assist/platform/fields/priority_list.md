# Field Assist: platform.priority_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalPriorityList()`; query view handle: `priorityListHandle()`.
- Reverse relation: `priorityList()` returns SmartList<Priority>&; query `selectPriorityList()`, `selectPriorityListWith(PriorityRequest<P>)`, `withPriorityList(PriorityRequest<P>)`; expression `getPriorityList()`.

Presence: `havePriorities()`, `haveNoPriorities()`. Matching: `withPriorityListMatching(child)`, `withoutPriorityListMatching(child)`.

Count: `countPriorities()`, `countPrioritiesAs(alias)`, `countPrioritiesWith(alias, child)`. Statistics/details: `statsFromPriorities(child)`, `statsFromPrioritiesAs(alias, child)`, `groupByPrioritiesWithDetails(child)`.

Field statistic: `minInstantOfPriorities()`, `minInstantOfPrioritiesAs(alias, child)`.

Field statistic: `maxInstantOfPriorities()`, `maxInstantOfPrioritiesAs(alias, child)`.

Field statistic: `minOptionalInstantOfPriorities()`, `minOptionalInstantOfPrioritiesAs(alias, child)`.

Field statistic: `maxOptionalInstantOfPriorities()`, `maxOptionalInstantOfPrioritiesAs(alias, child)`.

Field statistic: `minCreatedAtOfPriorities()`, `minCreatedAtOfPrioritiesAs(alias, child)`.

Field statistic: `maxCreatedAtOfPriorities()`, `maxCreatedAtOfPrioritiesAs(alias, child)`.

Field statistic: `minUpdatedAtOfPriorities()`, `minUpdatedAtOfPrioritiesAs(alias, child)`.

Field statistic: `maxUpdatedAtOfPriorities()`, `maxUpdatedAtOfPrioritiesAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
