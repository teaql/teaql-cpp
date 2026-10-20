# Field Assist: platform.priority_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalPriorityList()`; query view handle: `priorityListHandle()`.
- Reverse relation: `priorityList()` returns SmartList<Priority>&; query `selectPriorityList()`, `selectPriorityListWith(PriorityRequest<P>)`, `withPriorityList(PriorityRequest<P>)`; expression `getPriorityList()`.

Presence: `havePriorities()`, `haveNoPriorities()`. Matching: `withPriorityListMatching(child)`, `withoutPriorityListMatching(child)`.

Count: `countPriorities()`, `countPrioritiesAs(alias)`, `countPrioritiesWith(alias, child)`. Statistics/details: `statsFromPriorities(child)`, `statsFromPrioritiesAs(alias, child)`, `groupByPrioritiesWithDetails(child)`.

Field statistic: `sumValueOfPriorities()`, `sumValueOfPrioritiesAs(alias, child)`.

Field statistic: `minValueOfPriorities()`, `minValueOfPrioritiesAs(alias, child)`.

Field statistic: `maxValueOfPriorities()`, `maxValueOfPrioritiesAs(alias, child)`.

Field statistic: `avgValueOfPriorities()`, `avgValueOfPrioritiesAs(alias, child)`.

Field statistic: `standardDeviationValueOfPriorities()`, `standardDeviationValueOfPrioritiesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationValueOfPriorities()`, `squareRootOfPopulationStandardDeviationValueOfPrioritiesAs(alias, child)`.

Field statistic: `sampleVarianceValueOfPriorities()`, `sampleVarianceValueOfPrioritiesAs(alias, child)`.

Field statistic: `samplePopulationVarianceValueOfPriorities()`, `samplePopulationVarianceValueOfPrioritiesAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
