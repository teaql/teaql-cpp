# Generated C++ model-aware API

Namespace: `cpp_sample_runtime`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

## Platform
Entries: `Q.platforms()` and `Q.platformsMinimal()`. Evaluation: `E::platform(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.platformsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Platform. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Candidate filters: `withNameIsString()` and `withNameIsNotString()`; exact value `string()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model relation evaluation: `evalSampleRecordList()`; query view handle: `sampleRecordListHandle()`.
- Reverse relation: `sampleRecordList()` returns SmartList<SampleRecord>&; query `selectSampleRecordList()`, `selectSampleRecordListWith(SampleRecordRequest<P>)`, `withSampleRecordList(SampleRecordRequest<P>)`; expression `getSampleRecordList()`.
## SampleRecord
Entries: `Q.sampleRecords()` and `Q.sampleRecordsMinimal()`. Evaluation: `E::sampleRecord(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.sampleRecordsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByPlatformAs(name, child)` defaults to all facets; `facetByPlatformAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates SampleRecord. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `platformId()` / `updatePlatformId(const std::uint64_t&)`; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `withPlatformIs`, `withPlatformIsNot`, `withPlatformGreaterThan`, `withPlatformGreaterThanOrEqualTo`, `withPlatformLessThan`, `withPlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPlatformBetween(lower, upper)`, `withPlatformIn(std::span<const std::uint64_t>)`, `withPlatformNotIn(std::span<const std::uint64_t>)`, `withPlatformIsNull()`, `withPlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*, `updatePlatform(const Platform&)`; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `withPlatform(PlatformRequest<P>)`; expression `getPlatform()`.
- Model: `label()` / `updateLabel(std::string_view)`; changed `changedLabel()`; evaluation `evalLabel()`; expression `getLabel()`.
- Scalar predicates: `withLabelIs`, `withLabelIsNot`, `withLabelGreaterThan`, `withLabelGreaterThanOrEqualTo`, `withLabelLessThan`, `withLabelLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withLabelBetween(lower, upper)`, `withLabelIn(std::span<const String>)`, `withLabelNotIn(std::span<const String>)`, `withLabelIsNull()`, `withLabelIsNotNull()`.
- Projection/order/group: `selectLabel()`, `orderByLabelAsc()`, `orderByLabelDesc()`, `groupByLabel()`.
- Group/count: `groupByLabel()`, `groupByLabelAs(alias)`, `groupByLabelWithFunction(alias, function)`, `countLabel()` and `countLabelAs(alias)`.
- Scalar aggregate: `sumLabel()` uses alias `sumLabel`; `sumLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgLabel()` uses alias `avgLabel`; `avgLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minLabel()` uses alias `minLabel`; `minLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxLabel()` uses alias `maxLabel`; `maxLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withLabelContaining(text)`, `withLabelStartingWith(text)`, `withLabelEndingWith(text)`.
- Model: `quantity()` / `updateQuantity(const std::int64_t&)`; changed `changedQuantity()`; evaluation `evalQuantity()`; expression `getQuantity()`.
- Scalar predicates: `withQuantityIs`, `withQuantityIsNot`, `withQuantityGreaterThan`, `withQuantityGreaterThanOrEqualTo`, `withQuantityLessThan`, `withQuantityLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withQuantityBetween(lower, upper)`, `withQuantityIn(std::span<const std::int64_t>)`, `withQuantityNotIn(std::span<const std::int64_t>)`, `withQuantityIsNull()`, `withQuantityIsNotNull()`.
- Projection/order/group: `selectQuantity()`, `orderByQuantityAsc()`, `orderByQuantityDesc()`, `groupByQuantity()`.
- Group/count: `groupByQuantity()`, `groupByQuantityAs(alias)`, `groupByQuantityWithFunction(alias, function)`, `countQuantity()` and `countQuantityAs(alias)`.
- Scalar aggregate: `sumQuantity()` uses alias `sumQuantity`; `sumQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgQuantity()` uses alias `avgQuantity`; `avgQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minQuantity()` uses alias `minQuantity`; `minQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxQuantity()` uses alias `maxQuantity`; `maxQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationQuantity()` and `standardDeviationQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationQuantity()` and `squareRootOfPopulationStandardDeviationQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceQuantity()` and `sampleVarianceQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceQuantity()` and `samplePopulationVarianceQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `zeroBound()` / `updateZeroBound(const std::int64_t&)`; changed `changedZeroBound()`; evaluation `evalZeroBound()`; expression `getZeroBound()`.
- Scalar predicates: `withZeroBoundIs`, `withZeroBoundIsNot`, `withZeroBoundGreaterThan`, `withZeroBoundGreaterThanOrEqualTo`, `withZeroBoundLessThan`, `withZeroBoundLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withZeroBoundBetween(lower, upper)`, `withZeroBoundIn(std::span<const std::int64_t>)`, `withZeroBoundNotIn(std::span<const std::int64_t>)`, `withZeroBoundIsNull()`, `withZeroBoundIsNotNull()`.
- Projection/order/group: `selectZeroBound()`, `orderByZeroBoundAsc()`, `orderByZeroBoundDesc()`, `groupByZeroBound()`.
- Group/count: `groupByZeroBound()`, `groupByZeroBoundAs(alias)`, `groupByZeroBoundWithFunction(alias, function)`, `countZeroBound()` and `countZeroBoundAs(alias)`.
- Scalar aggregate: `sumZeroBound()` uses alias `sumZeroBound`; `sumZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgZeroBound()` uses alias `avgZeroBound`; `avgZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minZeroBound()` uses alias `minZeroBound`; `minZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxZeroBound()` uses alias `maxZeroBound`; `maxZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationZeroBound()` and `standardDeviationZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationZeroBound()` and `squareRootOfPopulationStandardDeviationZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceZeroBound()` and `sampleVarianceZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceZeroBound()` and `samplePopulationVarianceZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `negativeMax()` / `updateNegativeMax(const std::int64_t&)`; changed `changedNegativeMax()`; evaluation `evalNegativeMax()`; expression `getNegativeMax()`.
- Scalar predicates: `withNegativeMaxIs`, `withNegativeMaxIsNot`, `withNegativeMaxGreaterThan`, `withNegativeMaxGreaterThanOrEqualTo`, `withNegativeMaxLessThan`, `withNegativeMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withNegativeMaxBetween(lower, upper)`, `withNegativeMaxIn(std::span<const std::int64_t>)`, `withNegativeMaxNotIn(std::span<const std::int64_t>)`, `withNegativeMaxIsNull()`, `withNegativeMaxIsNotNull()`.
- Projection/order/group: `selectNegativeMax()`, `orderByNegativeMaxAsc()`, `orderByNegativeMaxDesc()`, `groupByNegativeMax()`.
- Group/count: `groupByNegativeMax()`, `groupByNegativeMaxAs(alias)`, `groupByNegativeMaxWithFunction(alias, function)`, `countNegativeMax()` and `countNegativeMaxAs(alias)`.
- Scalar aggregate: `sumNegativeMax()` uses alias `sumNegativeMax`; `sumNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNegativeMax()` uses alias `avgNegativeMax`; `avgNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNegativeMax()` uses alias `minNegativeMax`; `minNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNegativeMax()` uses alias `maxNegativeMax`; `maxNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationNegativeMax()` and `standardDeviationNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationNegativeMax()` and `squareRootOfPopulationStandardDeviationNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceNegativeMax()` and `sampleVarianceNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceNegativeMax()` and `samplePopulationVarianceNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `signedMax()` / `updateSignedMax(const std::int64_t&)`; changed `changedSignedMax()`; evaluation `evalSignedMax()`; expression `getSignedMax()`.
- Scalar predicates: `withSignedMaxIs`, `withSignedMaxIsNot`, `withSignedMaxGreaterThan`, `withSignedMaxGreaterThanOrEqualTo`, `withSignedMaxLessThan`, `withSignedMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withSignedMaxBetween(lower, upper)`, `withSignedMaxIn(std::span<const std::int64_t>)`, `withSignedMaxNotIn(std::span<const std::int64_t>)`, `withSignedMaxIsNull()`, `withSignedMaxIsNotNull()`.
- Projection/order/group: `selectSignedMax()`, `orderBySignedMaxAsc()`, `orderBySignedMaxDesc()`, `groupBySignedMax()`.
- Group/count: `groupBySignedMax()`, `groupBySignedMaxAs(alias)`, `groupBySignedMaxWithFunction(alias, function)`, `countSignedMax()` and `countSignedMaxAs(alias)`.
- Scalar aggregate: `sumSignedMax()` uses alias `sumSignedMax`; `sumSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgSignedMax()` uses alias `avgSignedMax`; `avgSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minSignedMax()` uses alias `minSignedMax`; `minSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxSignedMax()` uses alias `maxSignedMax`; `maxSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationSignedMax()` and `standardDeviationSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationSignedMax()` and `squareRootOfPopulationStandardDeviationSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceSignedMax()` and `sampleVarianceSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceSignedMax()` and `samplePopulationVarianceSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `wideMax()` / `updateWideMax(const std::int64_t&)`; changed `changedWideMax()`; evaluation `evalWideMax()`; expression `getWideMax()`.
- Scalar predicates: `withWideMaxIs`, `withWideMaxIsNot`, `withWideMaxGreaterThan`, `withWideMaxGreaterThanOrEqualTo`, `withWideMaxLessThan`, `withWideMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withWideMaxBetween(lower, upper)`, `withWideMaxIn(std::span<const std::int64_t>)`, `withWideMaxNotIn(std::span<const std::int64_t>)`, `withWideMaxIsNull()`, `withWideMaxIsNotNull()`.
- Projection/order/group: `selectWideMax()`, `orderByWideMaxAsc()`, `orderByWideMaxDesc()`, `groupByWideMax()`.
- Group/count: `groupByWideMax()`, `groupByWideMaxAs(alias)`, `groupByWideMaxWithFunction(alias, function)`, `countWideMax()` and `countWideMaxAs(alias)`.
- Scalar aggregate: `sumWideMax()` uses alias `sumWideMax`; `sumWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgWideMax()` uses alias `avgWideMax`; `avgWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minWideMax()` uses alias `minWideMax`; `minWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxWideMax()` uses alias `maxWideMax`; `maxWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationWideMax()` and `standardDeviationWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationWideMax()` and `squareRootOfPopulationStandardDeviationWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceWideMax()` and `sampleVarianceWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceWideMax()` and `samplePopulationVarianceWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `negativeBound()` / `updateNegativeBound(std::string_view)`; changed `changedNegativeBound()`; evaluation `evalNegativeBound()`; expression `getNegativeBound()`.
- Scalar predicates: `withNegativeBoundIs`, `withNegativeBoundIsNot`, `withNegativeBoundGreaterThan`, `withNegativeBoundGreaterThanOrEqualTo`, `withNegativeBoundLessThan`, `withNegativeBoundLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNegativeBoundBetween(lower, upper)`, `withNegativeBoundIn(std::span<const String>)`, `withNegativeBoundNotIn(std::span<const String>)`, `withNegativeBoundIsNull()`, `withNegativeBoundIsNotNull()`.
- Projection/order/group: `selectNegativeBound()`, `orderByNegativeBoundAsc()`, `orderByNegativeBoundDesc()`, `groupByNegativeBound()`.
- Group/count: `groupByNegativeBound()`, `groupByNegativeBoundAs(alias)`, `groupByNegativeBoundWithFunction(alias, function)`, `countNegativeBound()` and `countNegativeBoundAs(alias)`.
- Scalar aggregate: `sumNegativeBound()` uses alias `sumNegativeBound`; `sumNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNegativeBound()` uses alias `avgNegativeBound`; `avgNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNegativeBound()` uses alias `minNegativeBound`; `minNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNegativeBound()` uses alias `maxNegativeBound`; `maxNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNegativeBoundContaining(text)`, `withNegativeBoundStartingWith(text)`, `withNegativeBoundEndingWith(text)`.
- Model: `amount()` / `updateAmount(const Decimal&)`; changed `changedAmount()`; evaluation `evalAmount()`; expression `getAmount()`.
- Scalar predicates: `withAmountIs`, `withAmountIsNot`, `withAmountGreaterThan`, `withAmountGreaterThanOrEqualTo`, `withAmountLessThan`, `withAmountLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withAmountBetween(lower, upper)`, `withAmountIn(std::span<const Decimal>)`, `withAmountNotIn(std::span<const Decimal>)`, `withAmountIsNull()`, `withAmountIsNotNull()`.
- Projection/order/group: `selectAmount()`, `orderByAmountAsc()`, `orderByAmountDesc()`, `groupByAmount()`.
- Group/count: `groupByAmount()`, `groupByAmountAs(alias)`, `groupByAmountWithFunction(alias, function)`, `countAmount()` and `countAmountAs(alias)`.
- Scalar aggregate: `sumAmount()` uses alias `sumAmount`; `sumAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgAmount()` uses alias `avgAmount`; `avgAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minAmount()` uses alias `minAmount`; `minAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxAmount()` uses alias `maxAmount`; `maxAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationAmount()` and `standardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationAmount()` and `squareRootOfPopulationStandardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceAmount()` and `sampleVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceAmount()` and `samplePopulationVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `height()` / `updateHeight(const double&)`; changed `changedHeight()`; evaluation `evalHeight()`; expression `getHeight()`.
- Scalar predicates: `withHeightIs`, `withHeightIsNot`, `withHeightGreaterThan`, `withHeightGreaterThanOrEqualTo`, `withHeightLessThan`, `withHeightLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withHeightBetween(lower, upper)`, `withHeightIn(std::span<const double>)`, `withHeightNotIn(std::span<const double>)`, `withHeightIsNull()`, `withHeightIsNotNull()`.
- Projection/order/group: `selectHeight()`, `orderByHeightAsc()`, `orderByHeightDesc()`, `groupByHeight()`.
- Group/count: `groupByHeight()`, `groupByHeightAs(alias)`, `groupByHeightWithFunction(alias, function)`, `countHeight()` and `countHeightAs(alias)`.
- Scalar aggregate: `sumHeight()` uses alias `sumHeight`; `sumHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgHeight()` uses alias `avgHeight`; `avgHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minHeight()` uses alias `minHeight`; `minHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxHeight()` uses alias `maxHeight`; `maxHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationHeight()` and `standardDeviationHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationHeight()` and `squareRootOfPopulationStandardDeviationHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceHeight()` and `sampleVarianceHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceHeight()` and `samplePopulationVarianceHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `optionalCount()` / `updateOptionalCount(const std::optional<std::int64_t>&)`; changed `changedOptionalCount()`; evaluation `evalOptionalCount()`; expression `getOptionalCount()`.
- Scalar predicates: `withOptionalCountIs`, `withOptionalCountIsNot`, `withOptionalCountGreaterThan`, `withOptionalCountGreaterThanOrEqualTo`, `withOptionalCountLessThan`, `withOptionalCountLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalCountBetween(lower, upper)`, `withOptionalCountIn(std::span<const std::int64_t>)`, `withOptionalCountNotIn(std::span<const std::int64_t>)`, `withOptionalCountIsNull()`, `withOptionalCountIsNotNull()`.
- Projection/order/group: `selectOptionalCount()`, `orderByOptionalCountAsc()`, `orderByOptionalCountDesc()`, `groupByOptionalCount()`.
- Group/count: `groupByOptionalCount()`, `groupByOptionalCountAs(alias)`, `groupByOptionalCountWithFunction(alias, function)`, `countOptionalCount()` and `countOptionalCountAs(alias)`.
- Scalar aggregate: `sumOptionalCount()` uses alias `sumOptionalCount`; `sumOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalCount()` uses alias `avgOptionalCount`; `avgOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalCount()` uses alias `minOptionalCount`; `minOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalCount()` uses alias `maxOptionalCount`; `maxOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalCount()` and `standardDeviationOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalCount()` and `squareRootOfPopulationStandardDeviationOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalCount()` and `sampleVarianceOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalCount()` and `samplePopulationVarianceOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `disabled()` / `updateDisabled(const bool&)`; changed `changedDisabled()`; evaluation `evalDisabled()`; expression `getDisabled()`.
- Scalar predicates: `withDisabledIs`, `withDisabledIsNot`, `withDisabledGreaterThan`, `withDisabledGreaterThanOrEqualTo`, `withDisabledLessThan`, `withDisabledLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withDisabledBetween(lower, upper)`, `withDisabledIn(std::span<const bool>)`, `withDisabledNotIn(std::span<const bool>)`, `withDisabledIsNull()`, `withDisabledIsNotNull()`.
- Projection/order/group: `selectDisabled()`, `orderByDisabledAsc()`, `orderByDisabledDesc()`, `groupByDisabled()`.
- Group/count: `groupByDisabled()`, `groupByDisabledAs(alias)`, `groupByDisabledWithFunction(alias, function)`, `countDisabled()` and `countDisabledAs(alias)`.
- Scalar aggregate: `sumDisabled()` uses alias `sumDisabled`; `sumDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDisabled()` uses alias `avgDisabled`; `avgDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDisabled()` uses alias `minDisabled`; `minDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDisabled()` uses alias `maxDisabled`; `maxDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreDisabled()`, `whichAreNotDisabled()`.
- Model: `optionalActive()` / `updateOptionalActive(const std::optional<bool>&)`; changed `changedOptionalActive()`; evaluation `evalOptionalActive()`; expression `getOptionalActive()`.
- Scalar predicates: `withOptionalActiveIs`, `withOptionalActiveIsNot`, `withOptionalActiveGreaterThan`, `withOptionalActiveGreaterThanOrEqualTo`, `withOptionalActiveLessThan`, `withOptionalActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withOptionalActiveBetween(lower, upper)`, `withOptionalActiveIn(std::span<const bool>)`, `withOptionalActiveNotIn(std::span<const bool>)`, `withOptionalActiveIsNull()`, `withOptionalActiveIsNotNull()`.
- Projection/order/group: `selectOptionalActive()`, `orderByOptionalActiveAsc()`, `orderByOptionalActiveDesc()`, `groupByOptionalActive()`.
- Group/count: `groupByOptionalActive()`, `groupByOptionalActiveAs(alias)`, `groupByOptionalActiveWithFunction(alias, function)`, `countOptionalActive()` and `countOptionalActiveAs(alias)`.
- Scalar aggregate: `sumOptionalActive()` uses alias `sumOptionalActive`; `sumOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalActive()` uses alias `avgOptionalActive`; `avgOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalActive()` uses alias `minOptionalActive`; `minOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalActive()` uses alias `maxOptionalActive`; `maxOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreOptionalActive()`, `whichAreNotOptionalActive()`.
- Model: `day()` / `updateDay(const Date&)`; changed `changedDay()`; evaluation `evalDay()`; expression `getDay()`.
- Scalar predicates: `withDayIs`, `withDayIsNot`, `withDayGreaterThan`, `withDayGreaterThanOrEqualTo`, `withDayLessThan`, `withDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDayBetween(lower, upper)`, `withDayIn(std::span<const Date>)`, `withDayNotIn(std::span<const Date>)`, `withDayIsNull()`, `withDayIsNotNull()`.
- Projection/order/group: `selectDay()`, `orderByDayAsc()`, `orderByDayDesc()`, `groupByDay()`.
- Group/count: `groupByDay()`, `groupByDayAs(alias)`, `groupByDayWithFunction(alias, function)`, `countDay()` and `countDayAs(alias)`.
- Scalar aggregate: `sumDay()` uses alias `sumDay`; `sumDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDay()` uses alias `avgDay`; `avgDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDay()` uses alias `minDay`; `minDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDay()` uses alias `maxDay`; `maxDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `optionalDay()` / `updateOptionalDay(const std::optional<Date>&)`; changed `changedOptionalDay()`; evaluation `evalOptionalDay()`; expression `getOptionalDay()`.
- Scalar predicates: `withOptionalDayIs`, `withOptionalDayIsNot`, `withOptionalDayGreaterThan`, `withOptionalDayGreaterThanOrEqualTo`, `withOptionalDayLessThan`, `withOptionalDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withOptionalDayBetween(lower, upper)`, `withOptionalDayIn(std::span<const Date>)`, `withOptionalDayNotIn(std::span<const Date>)`, `withOptionalDayIsNull()`, `withOptionalDayIsNotNull()`.
- Projection/order/group: `selectOptionalDay()`, `orderByOptionalDayAsc()`, `orderByOptionalDayDesc()`, `groupByOptionalDay()`.
- Group/count: `groupByOptionalDay()`, `groupByOptionalDayAs(alias)`, `groupByOptionalDayWithFunction(alias, function)`, `countOptionalDay()` and `countOptionalDayAs(alias)`.
- Scalar aggregate: `sumOptionalDay()` uses alias `sumOptionalDay`; `sumOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDay()` uses alias `avgOptionalDay`; `avgOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDay()` uses alias `minOptionalDay`; `minOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDay()` uses alias `maxOptionalDay`; `maxOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `recordedAt()` / `updateRecordedAt(const Timestamp&)`; changed `changedRecordedAt()`; evaluation `evalRecordedAt()`; expression `getRecordedAt()`.
- Scalar predicates: `withRecordedAtIs`, `withRecordedAtIsNot`, `withRecordedAtGreaterThan`, `withRecordedAtGreaterThanOrEqualTo`, `withRecordedAtLessThan`, `withRecordedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withRecordedAtBetween(lower, upper)`, `withRecordedAtIn(std::span<const Timestamp>)`, `withRecordedAtNotIn(std::span<const Timestamp>)`, `withRecordedAtIsNull()`, `withRecordedAtIsNotNull()`.
- Projection/order/group: `selectRecordedAt()`, `orderByRecordedAtAsc()`, `orderByRecordedAtDesc()`, `groupByRecordedAt()`.
- Group/count: `groupByRecordedAt()`, `groupByRecordedAtAs(alias)`, `groupByRecordedAtWithFunction(alias, function)`, `countRecordedAt()` and `countRecordedAtAs(alias)`.
- Scalar aggregate: `sumRecordedAt()` uses alias `sumRecordedAt`; `sumRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRecordedAt()` uses alias `avgRecordedAt`; `avgRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRecordedAt()` uses alias `minRecordedAt`; `minRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRecordedAt()` uses alias `maxRecordedAt`; `maxRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `optionalRecordedAt()` / `updateOptionalRecordedAt(const std::optional<Timestamp>&)`; changed `changedOptionalRecordedAt()`; evaluation `evalOptionalRecordedAt()`; expression `getOptionalRecordedAt()`.
- Scalar predicates: `withOptionalRecordedAtIs`, `withOptionalRecordedAtIsNot`, `withOptionalRecordedAtGreaterThan`, `withOptionalRecordedAtGreaterThanOrEqualTo`, `withOptionalRecordedAtLessThan`, `withOptionalRecordedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withOptionalRecordedAtBetween(lower, upper)`, `withOptionalRecordedAtIn(std::span<const Timestamp>)`, `withOptionalRecordedAtNotIn(std::span<const Timestamp>)`, `withOptionalRecordedAtIsNull()`, `withOptionalRecordedAtIsNotNull()`.
- Projection/order/group: `selectOptionalRecordedAt()`, `orderByOptionalRecordedAtAsc()`, `orderByOptionalRecordedAtDesc()`, `groupByOptionalRecordedAt()`.
- Group/count: `groupByOptionalRecordedAt()`, `groupByOptionalRecordedAtAs(alias)`, `groupByOptionalRecordedAtWithFunction(alias, function)`, `countOptionalRecordedAt()` and `countOptionalRecordedAtAs(alias)`.
- Scalar aggregate: `sumOptionalRecordedAt()` uses alias `sumOptionalRecordedAt`; `sumOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalRecordedAt()` uses alias `avgOptionalRecordedAt`; `avgOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalRecordedAt()` uses alias `minOptionalRecordedAt`; `minOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalRecordedAt()` uses alias `maxOptionalRecordedAt`; `maxOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `clockText()` / `updateClockText(std::string_view)`; changed `changedClockText()`; evaluation `evalClockText()`; expression `getClockText()`.
- Scalar predicates: `withClockTextIs`, `withClockTextIsNot`, `withClockTextGreaterThan`, `withClockTextGreaterThanOrEqualTo`, `withClockTextLessThan`, `withClockTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withClockTextBetween(lower, upper)`, `withClockTextIn(std::span<const String>)`, `withClockTextNotIn(std::span<const String>)`, `withClockTextIsNull()`, `withClockTextIsNotNull()`.
- Projection/order/group: `selectClockText()`, `orderByClockTextAsc()`, `orderByClockTextDesc()`, `groupByClockText()`.
- Group/count: `groupByClockText()`, `groupByClockTextAs(alias)`, `groupByClockTextWithFunction(alias, function)`, `countClockText()` and `countClockTextAs(alias)`.
- Scalar aggregate: `sumClockText()` uses alias `sumClockText`; `sumClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgClockText()` uses alias `avgClockText`; `avgClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minClockText()` uses alias `minClockText`; `minClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxClockText()` uses alias `maxClockText`; `maxClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withClockTextContaining(text)`, `withClockTextStartingWith(text)`, `withClockTextEndingWith(text)`.
- Model: `optionalClockText()` / `updateOptionalClockText(const std::optional<String>&)`; changed `changedOptionalClockText()`; evaluation `evalOptionalClockText()`; expression `getOptionalClockText()`.
- Scalar predicates: `withOptionalClockTextIs`, `withOptionalClockTextIsNot`, `withOptionalClockTextGreaterThan`, `withOptionalClockTextGreaterThanOrEqualTo`, `withOptionalClockTextLessThan`, `withOptionalClockTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalClockTextBetween(lower, upper)`, `withOptionalClockTextIn(std::span<const String>)`, `withOptionalClockTextNotIn(std::span<const String>)`, `withOptionalClockTextIsNull()`, `withOptionalClockTextIsNotNull()`.
- Projection/order/group: `selectOptionalClockText()`, `orderByOptionalClockTextAsc()`, `orderByOptionalClockTextDesc()`, `groupByOptionalClockText()`.
- Group/count: `groupByOptionalClockText()`, `groupByOptionalClockTextAs(alias)`, `groupByOptionalClockTextWithFunction(alias, function)`, `countOptionalClockText()` and `countOptionalClockTextAs(alias)`.
- Scalar aggregate: `sumOptionalClockText()` uses alias `sumOptionalClockText`; `sumOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalClockText()` uses alias `avgOptionalClockText`; `avgOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalClockText()` uses alias `minOptionalClockText`; `minOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalClockText()` uses alias `maxOptionalClockText`; `maxOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalClockTextContaining(text)`, `withOptionalClockTextStartingWith(text)`, `withOptionalClockTextEndingWith(text)`.
- Model: `ratio()` / `updateRatio(const std::optional<float>&)`; changed `changedRatio()`; evaluation `evalRatio()`; expression `getRatio()`.
- Scalar predicates: `withRatioIs`, `withRatioIsNot`, `withRatioGreaterThan`, `withRatioGreaterThanOrEqualTo`, `withRatioLessThan`, `withRatioLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withRatioBetween(lower, upper)`, `withRatioIn(std::span<const float>)`, `withRatioNotIn(std::span<const float>)`, `withRatioIsNull()`, `withRatioIsNotNull()`.
- Projection/order/group: `selectRatio()`, `orderByRatioAsc()`, `orderByRatioDesc()`, `groupByRatio()`.
- Group/count: `groupByRatio()`, `groupByRatioAs(alias)`, `groupByRatioWithFunction(alias, function)`, `countRatio()` and `countRatioAs(alias)`.
- Scalar aggregate: `sumRatio()` uses alias `sumRatio`; `sumRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRatio()` uses alias `avgRatio`; `avgRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRatio()` uses alias `minRatio`; `minRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRatio()` uses alias `maxRatio`; `maxRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `payload()` / `updatePayload(const std::optional<Json>&)`; changed `changedPayload()`; evaluation `evalPayload()`; expression `getPayload()`.
- Scalar predicates: `withPayloadIs`, `withPayloadIsNot`, `withPayloadGreaterThan`, `withPayloadGreaterThanOrEqualTo`, `withPayloadLessThan`, `withPayloadLessThanOrEqualTo`. Arguments: `const Json&`.
- Range/set/null predicates: `withPayloadBetween(lower, upper)`, `withPayloadIn(std::span<const Json>)`, `withPayloadNotIn(std::span<const Json>)`, `withPayloadIsNull()`, `withPayloadIsNotNull()`.
- Projection/order/group: `selectPayload()`, `orderByPayloadAsc()`, `orderByPayloadDesc()`, `groupByPayload()`.
- Group/count: `groupByPayload()`, `groupByPayloadAs(alias)`, `groupByPayloadWithFunction(alias, function)`, `countPayload()` and `countPayloadAs(alias)`.
- Scalar aggregate: `sumPayload()` uses alias `sumPayload`; `sumPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPayload()` uses alias `avgPayload`; `avgPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPayload()` uses alias `minPayload`; `minPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPayload()` uses alias `maxPayload`; `maxPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model relation evaluation: `evalPlatform()` returns a checked entity borrow or Null/NotLoaded.
- Model relation evaluation: `evalFirstLinks()`; query view handle: `firstLinksHandle()`.
- Reverse relation: `firstLinks()` returns SmartList<SampleLink>&; query `selectFirstLinks()`, `selectFirstLinksWith(SampleLinkRequest<P>)`, `withFirstLinks(SampleLinkRequest<P>)`; expression `getFirstLinks()`.
- Model relation evaluation: `evalSecondLinks()`; query view handle: `secondLinksHandle()`.
- Reverse relation: `secondLinks()` returns SmartList<SampleLink>&; query `selectSecondLinks()`, `selectSecondLinksWith(SampleLinkRequest<P>)`, `withSecondLinks(SampleLinkRequest<P>)`; expression `getSecondLinks()`.
## SampleLink
Entries: `Q.sampleLinks()` and `Q.sampleLinksMinimal()`. Evaluation: `E::sampleLink(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.sampleLinksWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByFirstRecordAs(name, child)` defaults to all facets; `facetByFirstRecordAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Facet: `facetBySecondRecordAs(name, child)` defaults to all facets; `facetBySecondRecordAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates SampleLink. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `firstRecordId()` / `updateFirstRecordId(const std::uint64_t&)`; changed `changedFirstRecordId()`; evaluation `evalFirstRecordId()`; expression `getFirstRecordId()`.
- Scalar predicates: `withFirstRecordIs`, `withFirstRecordIsNot`, `withFirstRecordGreaterThan`, `withFirstRecordGreaterThanOrEqualTo`, `withFirstRecordLessThan`, `withFirstRecordLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withFirstRecordBetween(lower, upper)`, `withFirstRecordIn(std::span<const std::uint64_t>)`, `withFirstRecordNotIn(std::span<const std::uint64_t>)`, `withFirstRecordIsNull()`, `withFirstRecordIsNotNull()`.
- Projection/order/group: `selectFirstRecordId()`, `orderByFirstRecordIdAsc()`, `orderByFirstRecordIdDesc()`, `groupByFirstRecord()`.
- Group/count: `groupByFirstRecord()`, `groupByFirstRecordAs(alias)`, `groupByFirstRecordWithFunction(alias, function)`, `countFirstRecord()` and `countFirstRecordAs(alias)`.
- Forward relation: `firstRecord()` returns const SampleRecord*, `updateFirstRecord(const SampleRecord&)`; query `selectFirstRecord()`, `selectFirstRecordWith(SampleRecordRequest<P>)`, `withFirstRecord(SampleRecordRequest<P>)`; expression `getFirstRecord()`.
- Model: `secondRecordId()` / `updateSecondRecordId(const std::uint64_t&)`; changed `changedSecondRecordId()`; evaluation `evalSecondRecordId()`; expression `getSecondRecordId()`.
- Scalar predicates: `withSecondRecordIs`, `withSecondRecordIsNot`, `withSecondRecordGreaterThan`, `withSecondRecordGreaterThanOrEqualTo`, `withSecondRecordLessThan`, `withSecondRecordLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withSecondRecordBetween(lower, upper)`, `withSecondRecordIn(std::span<const std::uint64_t>)`, `withSecondRecordNotIn(std::span<const std::uint64_t>)`, `withSecondRecordIsNull()`, `withSecondRecordIsNotNull()`.
- Projection/order/group: `selectSecondRecordId()`, `orderBySecondRecordIdAsc()`, `orderBySecondRecordIdDesc()`, `groupBySecondRecord()`.
- Group/count: `groupBySecondRecord()`, `groupBySecondRecordAs(alias)`, `groupBySecondRecordWithFunction(alias, function)`, `countSecondRecord()` and `countSecondRecordAs(alias)`.
- Forward relation: `secondRecord()` returns const SampleRecord*, `updateSecondRecord(const SampleRecord&)`; query `selectSecondRecord()`, `selectSecondRecordWith(SampleRecordRequest<P>)`, `withSecondRecord(SampleRecordRequest<P>)`; expression `getSecondRecord()`.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `sumVersion`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `avgVersion`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `minVersion`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `maxVersion`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model relation evaluation: `evalFirstRecord()` returns a checked entity borrow or Null/NotLoaded.
- Model relation evaluation: `evalSecondRecord()` returns a checked entity borrow or Null/NotLoaded.

Common request methods: `comment(text)`, `purpose(text)`, `filter(expr)`, `andFilter(expr)`, `orFilter(expr)`, `selectSelf()`, `selectSelfFields()`, `limit(n)`, `offset(n)`, `skip(n)`, `hardLimit(n)`, `searchWithText(text)`. Purpose stages an executable type; modifiers remain available.
Embedded child requests must remain unpurposed: select relations, enhance children, facets, relation matching, grouping and aggregates with a child before its purpose() transition. The parent retains its own purpose; ordinary C++ modifiers may appear before or after that parent transition. Keep child comments and filters on the child. Root execution still requires both a nonblank comment and purpose.
Membership: matchingAnyOf(child) and matchAnyOf(child) compare root IDs with the child's ID subquery. They copy its core query, including selected relations, but do not import its independent selection options, comment, raw SQL or enhancements. Ordinary child requests may come from another generated executor domain. Keep the child unpurposed and in the same UserContext, or copy it explicitly first. Direct QuerySelection and SelectQuery inputs are also accepted in the receiving allocation scope. selection() captures independent options; moving that value into into_query() applies them before core-query membership. Use Context make() for retained conversion payloads; stack-owned payloads must be destroyed before their Context closes.
Dynamic fields: Q requests provide `selectDynamicFields(selection)`; models provide `dynamicFieldValues()`, `updateDynamicField(code, value)`, `deleteDynamicField(code)` and `hasPendingDynamicMutations()`. Durable fields use the runtime Context provider and audited ledger save; native getters and mutation values remain separate.
Readonly computed properties: `hasDynamicProperty(key)` reports an underscore-key projection even when its value is NULL; `dynamicProperty(key)` returns a borrowed non-NULL value, treating typed NULL and JSON null as absent. False, zero and empty text remain readable. These accessors do not allocate, execute SQL or grant durable dynamic-field write authority; returned values borrow the model's Context.
Context operations: `stream(chunk_size)`, `copyTo(user_context)`, `copyToGlobal(user_context)`, `rebindGlobal(user_context)`. Mutable request copies are independent; retained copies are explicit.
Executable methods (exactly one UserContext argument): `executeForList(user_context)`, `executeForEntityList(user_context)`, `execute(user_context)`, `executeForOne(user_context)`, `executeForStream(user_context)`, `executeForEntity(user_context)`, `executeForRows(user_context)`, `newEntity(user_context)`. Pollable results use `.wait()`; only newEntity returns a managed model directly.
Expression methods: `eval()`, `unwrap()`, `isNull()`, `isNotNull()`, `orIfNull(fallback)`, `orElseIfNull(callback)`, `orDefaultIfNull()`. List expressions add `size()` (scalar expression), `first()` and `get(index)` (entity expressions). NotLoaded is never a NULL fallback.
Missing-load diagnostics: catch `ExpressionNotLoadedError` and inspect `error.diagnostic()` for the captured root, access_path, failed_node, missing_preload, suggested_fix, optional nested_fix, severity and English human_message. Suggestions use generated C++ camelCase selection names. The error borrows its Context; diagnostic access fails after close. `what()` is static and remains safe. Retention requires an explicit destination copy before closing the source Context.
Diagnostic repair scope: a loaded relationship can leave a leaf-only diagnostic path. Apply the selection to that relationship's child request and retain the enclosing selection. Nested C++ selections use `select...With(child)`; leaf selections use `select...()`. Renderer entry placeholders and captured id=... expressions are explanatory pseudocode. Use current object/field Assist to resolve the actual Q entry, child type and selection names before executing a repair.
Expression composition: `map(callback)`, `andThen(node, callback)` and owning scalar `copyTo(user_context)`. Entity/list bindings are checked borrows; temporary entity entries are rejected. Scalar access is an eager snapshot. All Q/E wrappers expire with their scope; Global diagnostics must outlive surviving wrappers.
Save: `AuditedSaveExt<Model>::save(std::move(entity).audit_as("reason"), user_context).wait()`. Allocate requests/models with UserContext and call setup(user_context) once at bootstrap. Use explicit destination copies before closing the owning context. Unknown fields are compile errors.
Sample data: `generateSampleData(user_context, SampleDataPlan{SampleDataScale::Tiny, seed}).wait()` uses Context-bound queries and audited saves. It loads roots/constants, skips missing required dependencies, and generates business rows in depth/name order. Run setup first. Reference selection and numeric values follow the seed/index rules; temporal fields use the wall clock. Repeated calls create additional business rows. Reports and errors belong to UserContext; copy explicitly before close.
Sample failure: generation stops at the first save error; earlier successful saves remain committed. A failed invocation returns an error, and a later invocation may create another business batch.
JSON sample text follows the locked source setter operand and can fail typed checking. The sample generator does not silently turn that text into valid JSON. Initialize JSON models with explicit typed values when sample generation reports an error. Complete quoted JSON-literal source generation also has a retained compiler failure; no complete sample parity is advertised for that configuration.
Deletion: `entity.markForDeletion()` records deletion in the ledger and returns the model for chaining. Save through the audited save API; marking alone does not execute SQL. Versioned SQLite models use the runtime soft-delete protocol.
Projection shortcuts: `selectSelfFields()` and `selectSelfWithoutParent()` both select native fields, including inherited fields as the original generator does. `selectAll()` also selects forward and reverse to-one relations; `selectChildren()` adds reverse collections, and `selectAny()` aliases it. These do not replace `enhanceChildrenIfNeeded()` for polymorphic descendants.
Runtime attachment: `entity.attachRuntimeStateRecursive(targetState)` adopts pending mutations and preserves the entity's loaded snapshot. It recursively attaches stored forward objects; it does not load relations or traverse graph lookups/reverse collections. Both states must belong to the same Context; cross-domain retention requires an explicit destination copy.
## Business IDs
## Model relation evaluation
Model evalX() and E.getX() share the runtime relation rules. Loaded empty lists are values, selected empty to-one relations are Null, and missing views are NotLoaded. Returned entity/list values are checked Context borrows. Reverse XHandle() methods expose the immutable query-installed view; existing mutable list getters remain explicit model-local graph construction adapters. No getter performs implicit SQL.
## Checker and behavior assembly
`configureRuntime(user_context, filename)` installs generated metadata, checkers and behaviors, the SQLite provider, scoped persistent IDs and the application executor. It does not create schema. `setup(user_context, filename)` performs that configuration and waits for `user_context.ensure_schema().wait()`; its default filename is `:memory:`.
Use `module(user_context)` for metadata-only registration, or `moduleWithCheckers(user_context)`, `moduleWithBehaviors(user_context)` or `moduleWithBehaviorsAndCheckers(user_context)` for explicit assembly. Keep handwritten logic outside generated files.
All four module factories register schema bootstrap. Schema initialization prepares the generated Domain Root and active-root binding; reuse that root instead of inserting a second ID 1. Repeated initialization preserves existing root values. Bootstrap errors propagate; do not treat a failed `setup` as a usable initialized Context.
- Extend `PlatformCheckerLogic` and override `checkAndFixPlatform(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `PlatformChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `PlatformBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `SampleRecordCheckerLogic` and override `checkAndFixSampleRecord(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `SampleRecordChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `SampleRecordBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `SampleLinkCheckerLogic` and override `checkAndFixSampleLink(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `SampleLinkChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `SampleLinkBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
