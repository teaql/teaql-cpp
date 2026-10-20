# Field Assist: platform.record_list_as_namespace

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalRecordListAsNamespace()`; query view handle: `recordListAsNamespaceHandle()`.
- Reverse relation: `recordListAsNamespace()` returns SmartList<Record>&; query `selectRecordListAsNamespace()`, `selectRecordListAsNamespaceWith(RecordRequest<P>)`, `withRecordListAsNamespace(RecordRequest<P>)`; expression `getRecordListAsNamespace()`.

Presence: `haveRecordsAsNamespace()`, `haveNoRecordsAsNamespace()`. Matching: `withRecordListAsNamespaceMatching(child)`, `withoutRecordListAsNamespaceMatching(child)`.

Count: `countRecordsAsNamespace()`, `countRecordsAsNamespaceAs(alias)`, `countRecordsAsNamespaceWith(alias, child)`. Statistics/details: `statsFromRecordsAsNamespace(child)`, `statsFromRecordsAsNamespaceAs(alias, child)`, `groupByRecordsAsNamespaceWithDetails(child)`.

Field statistic: `sumClassOfRecordsAsNamespace()`, `sumClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `minClassOfRecordsAsNamespace()`, `minClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `maxClassOfRecordsAsNamespace()`, `maxClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `avgClassOfRecordsAsNamespace()`, `avgClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `standardDeviationClassOfRecordsAsNamespace()`, `standardDeviationClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationClassOfRecordsAsNamespace()`, `squareRootOfPopulationStandardDeviationClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sampleVarianceClassOfRecordsAsNamespace()`, `sampleVarianceClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `samplePopulationVarianceClassOfRecordsAsNamespace()`, `samplePopulationVarianceClassOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sumOperatorOfRecordsAsNamespace()`, `sumOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `minOperatorOfRecordsAsNamespace()`, `minOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `maxOperatorOfRecordsAsNamespace()`, `maxOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `avgOperatorOfRecordsAsNamespace()`, `avgOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `standardDeviationOperatorOfRecordsAsNamespace()`, `standardDeviationOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOperatorOfRecordsAsNamespace()`, `squareRootOfPopulationStandardDeviationOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sampleVarianceOperatorOfRecordsAsNamespace()`, `sampleVarianceOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `samplePopulationVarianceOperatorOfRecordsAsNamespace()`, `samplePopulationVarianceOperatorOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sumTemplateOfRecordsAsNamespace()`, `sumTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `minTemplateOfRecordsAsNamespace()`, `minTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `maxTemplateOfRecordsAsNamespace()`, `maxTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `avgTemplateOfRecordsAsNamespace()`, `avgTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `standardDeviationTemplateOfRecordsAsNamespace()`, `standardDeviationTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationTemplateOfRecordsAsNamespace()`, `squareRootOfPopulationStandardDeviationTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sampleVarianceTemplateOfRecordsAsNamespace()`, `sampleVarianceTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `samplePopulationVarianceTemplateOfRecordsAsNamespace()`, `samplePopulationVarianceTemplateOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sumTypeFieldOfRecordsAsNamespace()`, `sumTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `minTypeFieldOfRecordsAsNamespace()`, `minTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `maxTypeFieldOfRecordsAsNamespace()`, `maxTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `avgTypeFieldOfRecordsAsNamespace()`, `avgTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `standardDeviationTypeFieldOfRecordsAsNamespace()`, `standardDeviationTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationTypeFieldOfRecordsAsNamespace()`, `squareRootOfPopulationStandardDeviationTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sampleVarianceTypeFieldOfRecordsAsNamespace()`, `sampleVarianceTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `samplePopulationVarianceTypeFieldOfRecordsAsNamespace()`, `samplePopulationVarianceTypeFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sumMatchFieldOfRecordsAsNamespace()`, `sumMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `minMatchFieldOfRecordsAsNamespace()`, `minMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `maxMatchFieldOfRecordsAsNamespace()`, `maxMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `avgMatchFieldOfRecordsAsNamespace()`, `avgMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `standardDeviationMatchFieldOfRecordsAsNamespace()`, `standardDeviationMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationMatchFieldOfRecordsAsNamespace()`, `squareRootOfPopulationStandardDeviationMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sampleVarianceMatchFieldOfRecordsAsNamespace()`, `sampleVarianceMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `samplePopulationVarianceMatchFieldOfRecordsAsNamespace()`, `samplePopulationVarianceMatchFieldOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sumOrdinaryOfRecordsAsNamespace()`, `sumOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `minOrdinaryOfRecordsAsNamespace()`, `minOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `maxOrdinaryOfRecordsAsNamespace()`, `maxOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `avgOrdinaryOfRecordsAsNamespace()`, `avgOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `standardDeviationOrdinaryOfRecordsAsNamespace()`, `standardDeviationOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOrdinaryOfRecordsAsNamespace()`, `squareRootOfPopulationStandardDeviationOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `sampleVarianceOrdinaryOfRecordsAsNamespace()`, `sampleVarianceOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

Field statistic: `samplePopulationVarianceOrdinaryOfRecordsAsNamespace()`, `samplePopulationVarianceOrdinaryOfRecordsAsNamespaceAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
