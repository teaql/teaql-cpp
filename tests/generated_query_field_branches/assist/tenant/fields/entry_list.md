# Field Assist: tenant.entry_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalEntryList()`; query view handle: `entryListHandle()`.
- Reverse relation: `entryList()` returns SmartList<Entry>&; query `selectEntryList()`, `selectEntryListWith(EntryRequest<P>)`, `withEntryList(EntryRequest<P>)`; expression `getEntryList()`.

Presence: `haveEntries()`, `haveNoEntries()`. Matching: `withEntryListMatching(child)`, `withoutEntryListMatching(child)`.

Count: `countEntries()`, `countEntriesAs(alias)`, `countEntriesWith(alias, child)`. Statistics/details: `statsFromEntries(child)`, `statsFromEntriesAs(alias, child)`, `groupByEntriesWithDetails(child)`.

Field statistic: `sumAmountOfEntries()`, `sumAmountOfEntriesAs(alias, child)`.

Field statistic: `minAmountOfEntries()`, `minAmountOfEntriesAs(alias, child)`.

Field statistic: `maxAmountOfEntries()`, `maxAmountOfEntriesAs(alias, child)`.

Field statistic: `avgAmountOfEntries()`, `avgAmountOfEntriesAs(alias, child)`.

Field statistic: `standardDeviationAmountOfEntries()`, `standardDeviationAmountOfEntriesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationAmountOfEntries()`, `squareRootOfPopulationStandardDeviationAmountOfEntriesAs(alias, child)`.

Field statistic: `sampleVarianceAmountOfEntries()`, `sampleVarianceAmountOfEntriesAs(alias, child)`.

Field statistic: `samplePopulationVarianceAmountOfEntries()`, `samplePopulationVarianceAmountOfEntriesAs(alias, child)`.

Field statistic: `sumOptionalAmountOfEntries()`, `sumOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `minOptionalAmountOfEntries()`, `minOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `maxOptionalAmountOfEntries()`, `maxOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `avgOptionalAmountOfEntries()`, `avgOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `standardDeviationOptionalAmountOfEntries()`, `standardDeviationOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOptionalAmountOfEntries()`, `squareRootOfPopulationStandardDeviationOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `sampleVarianceOptionalAmountOfEntries()`, `sampleVarianceOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `samplePopulationVarianceOptionalAmountOfEntries()`, `samplePopulationVarianceOptionalAmountOfEntriesAs(alias, child)`.

Field statistic: `sumHeightOfEntries()`, `sumHeightOfEntriesAs(alias, child)`.

Field statistic: `minHeightOfEntries()`, `minHeightOfEntriesAs(alias, child)`.

Field statistic: `maxHeightOfEntries()`, `maxHeightOfEntriesAs(alias, child)`.

Field statistic: `avgHeightOfEntries()`, `avgHeightOfEntriesAs(alias, child)`.

Field statistic: `standardDeviationHeightOfEntries()`, `standardDeviationHeightOfEntriesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationHeightOfEntries()`, `squareRootOfPopulationStandardDeviationHeightOfEntriesAs(alias, child)`.

Field statistic: `sampleVarianceHeightOfEntries()`, `sampleVarianceHeightOfEntriesAs(alias, child)`.

Field statistic: `samplePopulationVarianceHeightOfEntries()`, `samplePopulationVarianceHeightOfEntriesAs(alias, child)`.

Field statistic: `sumRatioOfEntries()`, `sumRatioOfEntriesAs(alias, child)`.

Field statistic: `minRatioOfEntries()`, `minRatioOfEntriesAs(alias, child)`.

Field statistic: `maxRatioOfEntries()`, `maxRatioOfEntriesAs(alias, child)`.

Field statistic: `avgRatioOfEntries()`, `avgRatioOfEntriesAs(alias, child)`.

Field statistic: `standardDeviationRatioOfEntries()`, `standardDeviationRatioOfEntriesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationRatioOfEntries()`, `squareRootOfPopulationStandardDeviationRatioOfEntriesAs(alias, child)`.

Field statistic: `sampleVarianceRatioOfEntries()`, `sampleVarianceRatioOfEntriesAs(alias, child)`.

Field statistic: `samplePopulationVarianceRatioOfEntries()`, `samplePopulationVarianceRatioOfEntriesAs(alias, child)`.

Field statistic: `sumCounterOfEntries()`, `sumCounterOfEntriesAs(alias, child)`.

Field statistic: `minCounterOfEntries()`, `minCounterOfEntriesAs(alias, child)`.

Field statistic: `maxCounterOfEntries()`, `maxCounterOfEntriesAs(alias, child)`.

Field statistic: `avgCounterOfEntries()`, `avgCounterOfEntriesAs(alias, child)`.

Field statistic: `standardDeviationCounterOfEntries()`, `standardDeviationCounterOfEntriesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationCounterOfEntries()`, `squareRootOfPopulationStandardDeviationCounterOfEntriesAs(alias, child)`.

Field statistic: `sampleVarianceCounterOfEntries()`, `sampleVarianceCounterOfEntriesAs(alias, child)`.

Field statistic: `samplePopulationVarianceCounterOfEntries()`, `samplePopulationVarianceCounterOfEntriesAs(alias, child)`.

Field statistic: `minDayOfEntries()`, `minDayOfEntriesAs(alias, child)`.

Field statistic: `maxDayOfEntries()`, `maxDayOfEntriesAs(alias, child)`.

Field statistic: `minOptionalDayOfEntries()`, `minOptionalDayOfEntriesAs(alias, child)`.

Field statistic: `maxOptionalDayOfEntries()`, `maxOptionalDayOfEntriesAs(alias, child)`.

Field statistic: `minMomentOfEntries()`, `minMomentOfEntriesAs(alias, child)`.

Field statistic: `maxMomentOfEntries()`, `maxMomentOfEntriesAs(alias, child)`.

Field statistic: `minOptionalMomentOfEntries()`, `minOptionalMomentOfEntriesAs(alias, child)`.

Field statistic: `maxOptionalMomentOfEntries()`, `maxOptionalMomentOfEntriesAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
