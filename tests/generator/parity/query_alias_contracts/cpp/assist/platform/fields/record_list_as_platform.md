# Field Assist: platform.record_list_as_platform

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalRecordListAsPlatform()`; query view handle: `recordListAsPlatformHandle()`.
- Reverse relation: `recordListAsPlatform()` returns SmartList<Record>&; query `selectRecordListAsPlatform()`, `selectRecordListAsPlatformWith(RecordRequest<P>)`, `withRecordListAsPlatform(RecordRequest<P>)`; expression `getRecordListAsPlatform()`.

Presence: `haveRecordsAsPlatform()`, `haveNoRecordsAsPlatform()`. Matching: `withRecordListAsPlatformMatching(child)`, `withoutRecordListAsPlatformMatching(child)`.

Count: `countRecordsAsPlatform()`, `countRecordsAsPlatformAs(alias)`, `countRecordsAsPlatformWith(alias, child)`. Statistics/details: `statsFromRecordsAsPlatform(child)`, `statsFromRecordsAsPlatformAs(alias, child)`, `groupByRecordsAsPlatformWithDetails(child)`.

Field statistic: `sumClassOfRecordsAsPlatform()`, `sumClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `minClassOfRecordsAsPlatform()`, `minClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `maxClassOfRecordsAsPlatform()`, `maxClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `avgClassOfRecordsAsPlatform()`, `avgClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `standardDeviationClassOfRecordsAsPlatform()`, `standardDeviationClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationClassOfRecordsAsPlatform()`, `squareRootOfPopulationStandardDeviationClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sampleVarianceClassOfRecordsAsPlatform()`, `sampleVarianceClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `samplePopulationVarianceClassOfRecordsAsPlatform()`, `samplePopulationVarianceClassOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sumOperatorOfRecordsAsPlatform()`, `sumOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `minOperatorOfRecordsAsPlatform()`, `minOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `maxOperatorOfRecordsAsPlatform()`, `maxOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `avgOperatorOfRecordsAsPlatform()`, `avgOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `standardDeviationOperatorOfRecordsAsPlatform()`, `standardDeviationOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOperatorOfRecordsAsPlatform()`, `squareRootOfPopulationStandardDeviationOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sampleVarianceOperatorOfRecordsAsPlatform()`, `sampleVarianceOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `samplePopulationVarianceOperatorOfRecordsAsPlatform()`, `samplePopulationVarianceOperatorOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sumTemplateOfRecordsAsPlatform()`, `sumTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `minTemplateOfRecordsAsPlatform()`, `minTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `maxTemplateOfRecordsAsPlatform()`, `maxTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `avgTemplateOfRecordsAsPlatform()`, `avgTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `standardDeviationTemplateOfRecordsAsPlatform()`, `standardDeviationTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationTemplateOfRecordsAsPlatform()`, `squareRootOfPopulationStandardDeviationTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sampleVarianceTemplateOfRecordsAsPlatform()`, `sampleVarianceTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `samplePopulationVarianceTemplateOfRecordsAsPlatform()`, `samplePopulationVarianceTemplateOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sumTypeFieldOfRecordsAsPlatform()`, `sumTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `minTypeFieldOfRecordsAsPlatform()`, `minTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `maxTypeFieldOfRecordsAsPlatform()`, `maxTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `avgTypeFieldOfRecordsAsPlatform()`, `avgTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `standardDeviationTypeFieldOfRecordsAsPlatform()`, `standardDeviationTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationTypeFieldOfRecordsAsPlatform()`, `squareRootOfPopulationStandardDeviationTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sampleVarianceTypeFieldOfRecordsAsPlatform()`, `sampleVarianceTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `samplePopulationVarianceTypeFieldOfRecordsAsPlatform()`, `samplePopulationVarianceTypeFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sumMatchFieldOfRecordsAsPlatform()`, `sumMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `minMatchFieldOfRecordsAsPlatform()`, `minMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `maxMatchFieldOfRecordsAsPlatform()`, `maxMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `avgMatchFieldOfRecordsAsPlatform()`, `avgMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `standardDeviationMatchFieldOfRecordsAsPlatform()`, `standardDeviationMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationMatchFieldOfRecordsAsPlatform()`, `squareRootOfPopulationStandardDeviationMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sampleVarianceMatchFieldOfRecordsAsPlatform()`, `sampleVarianceMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `samplePopulationVarianceMatchFieldOfRecordsAsPlatform()`, `samplePopulationVarianceMatchFieldOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sumOrdinaryOfRecordsAsPlatform()`, `sumOrdinaryOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `minOrdinaryOfRecordsAsPlatform()`, `minOrdinaryOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `maxOrdinaryOfRecordsAsPlatform()`, `maxOrdinaryOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `avgOrdinaryOfRecordsAsPlatform()`, `avgOrdinaryOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `standardDeviationOrdinaryOfRecordsAsPlatform()`, `standardDeviationOrdinaryOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOrdinaryOfRecordsAsPlatform()`, `squareRootOfPopulationStandardDeviationOrdinaryOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `sampleVarianceOrdinaryOfRecordsAsPlatform()`, `sampleVarianceOrdinaryOfRecordsAsPlatformAs(alias, child)`.

Field statistic: `samplePopulationVarianceOrdinaryOfRecordsAsPlatform()`, `samplePopulationVarianceOrdinaryOfRecordsAsPlatformAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
