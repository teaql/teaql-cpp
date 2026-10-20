# Field Assist: platform.class_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalClassList()`; query view handle: `classListHandle()`.
- Reverse relation: `classList()` returns SmartList<Class>&; query `selectClassList()`, `selectClassListWith(ClassRequest<P>)`, `withClassList(ClassRequest<P>)`; expression `getClassList()`.

Presence: `haveClasses()`, `haveNoClasses()`. Matching: `withClassListMatching(child)`, `withoutClassListMatching(child)`.

Count: `countClasses()`, `countClassesAs(alias)`, `countClassesWith(alias, child)`. Statistics/details: `statsFromClasses(child)`, `statsFromClassesAs(alias, child)`, `groupByClassesWithDetails(child)`.

Field statistic: `sumOperatorOfClasses()`, `sumOperatorOfClassesAs(alias, child)`.

Field statistic: `minOperatorOfClasses()`, `minOperatorOfClassesAs(alias, child)`.

Field statistic: `maxOperatorOfClasses()`, `maxOperatorOfClassesAs(alias, child)`.

Field statistic: `avgOperatorOfClasses()`, `avgOperatorOfClassesAs(alias, child)`.

Field statistic: `standardDeviationOperatorOfClasses()`, `standardDeviationOperatorOfClassesAs(alias, child)`.

Field statistic: `squareRootOfPopulationStandardDeviationOperatorOfClasses()`, `squareRootOfPopulationStandardDeviationOperatorOfClassesAs(alias, child)`.

Field statistic: `sampleVarianceOperatorOfClasses()`, `sampleVarianceOperatorOfClassesAs(alias, child)`.

Field statistic: `samplePopulationVarianceOperatorOfClasses()`, `samplePopulationVarianceOperatorOfClassesAs(alias, child)`.

[Model/action index](../../../ASSIST.md)
