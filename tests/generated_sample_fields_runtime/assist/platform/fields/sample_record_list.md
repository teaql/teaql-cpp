# Field Assist: platform.sample_record_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalSampleRecordList()`; query view handle: `sampleRecordListHandle()`.
- Reverse relation: `sampleRecordList()` returns SmartList<SampleRecord>&; query `selectSampleRecordList()`, `selectSampleRecordListWith(SampleRecordRequest<P>)`, `withSampleRecordList(SampleRecordRequest<P>)`; expression `getSampleRecordList()`.

Presence: `haveSampleRecords()`, `haveNoSampleRecords()`. Matching: `withSampleRecordListMatching(child)`, `withoutSampleRecordListMatching(child)`.

Count: `countSampleRecords()`, `countSampleRecordsAs(alias)`, `countSampleRecordsWith(alias, child)`. Statistics/details: `statsFromSampleRecords(child)`, `statsFromSampleRecordsAs(alias, child)`, `groupBySampleRecordsWithDetails(child)`.

Field statistic: `sumQuantityOfSampleRecords()`, `sumQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `minQuantityOfSampleRecords()`, `minQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `maxQuantityOfSampleRecords()`, `maxQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `avgQuantityOfSampleRecords()`, `avgQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationQuantityOfSampleRecords()`, `standardDeviationQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationQuantityOfSampleRecords()`, `squareRootOfPopulationStandardDeviationQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceQuantityOfSampleRecords()`, `sampleVarianceQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceQuantityOfSampleRecords()`, `samplePopulationVarianceQuantityOfSampleRecordsAs(alias, child)`.

Field statistic: `sumZeroBoundOfSampleRecords()`, `sumZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `minZeroBoundOfSampleRecords()`, `minZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `maxZeroBoundOfSampleRecords()`, `maxZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `avgZeroBoundOfSampleRecords()`, `avgZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationZeroBoundOfSampleRecords()`, `standardDeviationZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationZeroBoundOfSampleRecords()`, `squareRootOfPopulationStandardDeviationZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceZeroBoundOfSampleRecords()`, `sampleVarianceZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceZeroBoundOfSampleRecords()`, `samplePopulationVarianceZeroBoundOfSampleRecordsAs(alias, child)`.

Field statistic: `sumNegativeMaxOfSampleRecords()`, `sumNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `minNegativeMaxOfSampleRecords()`, `minNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `maxNegativeMaxOfSampleRecords()`, `maxNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `avgNegativeMaxOfSampleRecords()`, `avgNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationNegativeMaxOfSampleRecords()`, `standardDeviationNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationNegativeMaxOfSampleRecords()`, `squareRootOfPopulationStandardDeviationNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceNegativeMaxOfSampleRecords()`, `sampleVarianceNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceNegativeMaxOfSampleRecords()`, `samplePopulationVarianceNegativeMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `sumSignedMaxOfSampleRecords()`, `sumSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `minSignedMaxOfSampleRecords()`, `minSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `maxSignedMaxOfSampleRecords()`, `maxSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `avgSignedMaxOfSampleRecords()`, `avgSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationSignedMaxOfSampleRecords()`, `standardDeviationSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationSignedMaxOfSampleRecords()`, `squareRootOfPopulationStandardDeviationSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceSignedMaxOfSampleRecords()`, `sampleVarianceSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceSignedMaxOfSampleRecords()`, `samplePopulationVarianceSignedMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `sumWideMaxOfSampleRecords()`, `sumWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `minWideMaxOfSampleRecords()`, `minWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `maxWideMaxOfSampleRecords()`, `maxWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `avgWideMaxOfSampleRecords()`, `avgWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationWideMaxOfSampleRecords()`, `standardDeviationWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationWideMaxOfSampleRecords()`, `squareRootOfPopulationStandardDeviationWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceWideMaxOfSampleRecords()`, `sampleVarianceWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceWideMaxOfSampleRecords()`, `samplePopulationVarianceWideMaxOfSampleRecordsAs(alias, child)`.

Field statistic: `sumAmountOfSampleRecords()`, `sumAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `minAmountOfSampleRecords()`, `minAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `maxAmountOfSampleRecords()`, `maxAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `avgAmountOfSampleRecords()`, `avgAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationAmountOfSampleRecords()`, `standardDeviationAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationAmountOfSampleRecords()`, `squareRootOfPopulationStandardDeviationAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceAmountOfSampleRecords()`, `sampleVarianceAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceAmountOfSampleRecords()`, `samplePopulationVarianceAmountOfSampleRecordsAs(alias, child)`.

Field statistic: `sumHeightOfSampleRecords()`, `sumHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `minHeightOfSampleRecords()`, `minHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `maxHeightOfSampleRecords()`, `maxHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `avgHeightOfSampleRecords()`, `avgHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationHeightOfSampleRecords()`, `standardDeviationHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationHeightOfSampleRecords()`, `squareRootOfPopulationStandardDeviationHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceHeightOfSampleRecords()`, `sampleVarianceHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceHeightOfSampleRecords()`, `samplePopulationVarianceHeightOfSampleRecordsAs(alias, child)`.

Field statistic: `sumOptionalCountOfSampleRecords()`, `sumOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `minOptionalCountOfSampleRecords()`, `minOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `maxOptionalCountOfSampleRecords()`, `maxOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `avgOptionalCountOfSampleRecords()`, `avgOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationOptionalCountOfSampleRecords()`, `standardDeviationOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOptionalCountOfSampleRecords()`, `squareRootOfPopulationStandardDeviationOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceOptionalCountOfSampleRecords()`, `sampleVarianceOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceOptionalCountOfSampleRecords()`, `samplePopulationVarianceOptionalCountOfSampleRecordsAs(alias, child)`.

Field statistic: `minDayOfSampleRecords()`, `minDayOfSampleRecordsAs(alias, child)`.

Field statistic: `maxDayOfSampleRecords()`, `maxDayOfSampleRecordsAs(alias, child)`.

Field statistic: `minOptionalDayOfSampleRecords()`, `minOptionalDayOfSampleRecordsAs(alias, child)`.

Field statistic: `maxOptionalDayOfSampleRecords()`, `maxOptionalDayOfSampleRecordsAs(alias, child)`.

Field statistic: `minRecordedAtOfSampleRecords()`, `minRecordedAtOfSampleRecordsAs(alias, child)`.

Field statistic: `maxRecordedAtOfSampleRecords()`, `maxRecordedAtOfSampleRecordsAs(alias, child)`.

Field statistic: `minOptionalRecordedAtOfSampleRecords()`, `minOptionalRecordedAtOfSampleRecordsAs(alias, child)`.

Field statistic: `maxOptionalRecordedAtOfSampleRecords()`, `maxOptionalRecordedAtOfSampleRecordsAs(alias, child)`.

Field statistic: `sumRatioOfSampleRecords()`, `sumRatioOfSampleRecordsAs(alias, child)`.

Field statistic: `minRatioOfSampleRecords()`, `minRatioOfSampleRecordsAs(alias, child)`.

Field statistic: `maxRatioOfSampleRecords()`, `maxRatioOfSampleRecordsAs(alias, child)`.

Field statistic: `avgRatioOfSampleRecords()`, `avgRatioOfSampleRecordsAs(alias, child)`.

Field statistic: `standardDeviationRatioOfSampleRecords()`, `standardDeviationRatioOfSampleRecordsAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationRatioOfSampleRecords()`, `squareRootOfPopulationStandardDeviationRatioOfSampleRecordsAs(alias, child)`.

Field statistic: `sampleVarianceRatioOfSampleRecords()`, `sampleVarianceRatioOfSampleRecordsAs(alias, child)`.

Field statistic: `samplePopulationVarianceRatioOfSampleRecords()`, `samplePopulationVarianceRatioOfSampleRecordsAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
