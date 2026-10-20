# Field Assist: platform.sample_item_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSampleItemList()`; query view handle: `sampleItemListHandle()`.
- Reverse relation: `sampleItemList()` returns SmartList<SampleItem>&; query `selectSampleItemList()`, `selectSampleItemListWith(SampleItemRequest<P>)`, `withSampleItemList(SampleItemRequest<P>)`; expression `getSampleItemList()`.

Presence: `haveSampleItems()`, `haveNoSampleItems()`. Matching: `withSampleItemListMatching(child)`, `withoutSampleItemListMatching(child)`.

Count: `countSampleItems()`, `countSampleItemsAs(alias)`, `countSampleItemsWith(alias, child)`. Statistics/details: `statsFromSampleItems(child)`, `statsFromSampleItemsAs(alias, child)`, `groupBySampleItemsWithDetails(child)`.

Field statistic: `sumValueOfSampleItems()`, `sumValueOfSampleItemsAs(alias, child)`.

Field statistic: `minValueOfSampleItems()`, `minValueOfSampleItemsAs(alias, child)`.

Field statistic: `maxValueOfSampleItems()`, `maxValueOfSampleItemsAs(alias, child)`.

Field statistic: `avgValueOfSampleItems()`, `avgValueOfSampleItemsAs(alias, child)`.

Field statistic: `standardDeviationValueOfSampleItems()`, `standardDeviationValueOfSampleItemsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationValueOfSampleItems()`, `squareRootOfPopulationStandardDeviationValueOfSampleItemsAs(alias, child)`.

Field statistic: `sampleVarianceValueOfSampleItems()`, `sampleVarianceValueOfSampleItemsAs(alias, child)`.

Field statistic: `samplePopulationVarianceValueOfSampleItems()`, `samplePopulationVarianceValueOfSampleItemsAs(alias, child)`.

Field statistic: `sumOptionalValueOfSampleItems()`, `sumOptionalValueOfSampleItemsAs(alias, child)`.

Field statistic: `minOptionalValueOfSampleItems()`, `minOptionalValueOfSampleItemsAs(alias, child)`.

Field statistic: `maxOptionalValueOfSampleItems()`, `maxOptionalValueOfSampleItemsAs(alias, child)`.

Field statistic: `avgOptionalValueOfSampleItems()`, `avgOptionalValueOfSampleItemsAs(alias, child)`.

Field statistic: `standardDeviationOptionalValueOfSampleItems()`, `standardDeviationOptionalValueOfSampleItemsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOptionalValueOfSampleItems()`, `squareRootOfPopulationStandardDeviationOptionalValueOfSampleItemsAs(alias, child)`.

Field statistic: `sampleVarianceOptionalValueOfSampleItems()`, `sampleVarianceOptionalValueOfSampleItemsAs(alias, child)`.

Field statistic: `samplePopulationVarianceOptionalValueOfSampleItems()`, `samplePopulationVarianceOptionalValueOfSampleItemsAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
