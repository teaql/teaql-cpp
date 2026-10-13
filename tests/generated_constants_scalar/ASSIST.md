# Generated C++ model-aware API

Namespace: `cpp_constants_scalar`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

Qualification-only constants/candidates output: generated root/constant recipes are under qualification; normal constant generation remains guarded.

## Priority
Entries: `Q.priorities()` and `Q.prioritiesMinimal()`. Evaluation: `E::priority(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.prioritiesWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByPlatformAs(name, child)` defaults to all facets; `facetByPlatformAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Priority. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Candidate filters: `withIdIsValue1()` and `withIdIsNotValue1()`; exact value `1`.
- Candidate filters: `withIdIsValue22()` and `withIdIsNotValue22()`; exact value `22`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `platformId()`; Context-root assignment is managed through UserContext; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `withPlatformIs`, `withPlatformIsNot`, `withPlatformGreaterThan`, `withPlatformGreaterThanOrEqualTo`, `withPlatformLessThan`, `withPlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPlatformBetween(lower, upper)`, `withPlatformIn(std::span<const std::uint64_t>)`, `withPlatformNotIn(std::span<const std::uint64_t>)`, `withPlatformIsNull()`, `withPlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*; Context-root assignment is managed through UserContext; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `withPlatform(PlatformRequest<P>)`; expression `getPlatform()`.
- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Candidate filters: `withNameIsBasic()` and `withNameIsNotBasic()`; exact value `Basic`.
- Candidate filters: `withNameIsExtended()` and `withNameIsNotExtended()`; exact value `Extended`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `sumName`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `avgName`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `minName`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `maxName`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
- Model: `code()` / `updateCode(std::string_view)`; changed `changedCode()`; evaluation `evalCode()`; expression `getCode()`.
- Candidate filters: `withCodeIsBasiC()` and `withCodeIsNotBasiC()`; exact value `BASIC`.
- Candidate filters: `withCodeIsExtendeD()` and `withCodeIsNotExtendeD()`; exact value `EXTENDED`.
- Scalar predicates: `withCodeIs`, `withCodeIsNot`, `withCodeGreaterThan`, `withCodeGreaterThanOrEqualTo`, `withCodeLessThan`, `withCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withCodeBetween(lower, upper)`, `withCodeIn(std::span<const String>)`, `withCodeNotIn(std::span<const String>)`, `withCodeIsNull()`, `withCodeIsNotNull()`.
- Projection/order/group: `selectCode()`, `orderByCodeAsc()`, `orderByCodeDesc()`, `groupByCode()`.
- Group/count: `groupByCode()`, `groupByCodeAs(alias)`, `groupByCodeWithFunction(alias, function)`, `countCode()` and `countCodeAs(alias)`.
- Scalar aggregate: `sumCode()` uses alias `sumCode`; `sumCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCode()` uses alias `avgCode`; `avgCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCode()` uses alias `minCode`; `minCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCode()` uses alias `maxCode`; `maxCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withCodeContaining(text)`, `withCodeStartingWith(text)`, `withCodeEndingWith(text)`.
- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Candidate filters: `withActiveIsFalse()` and `withActiveIsNotFalse()`; exact value `false`.
- Candidate filters: `withActiveIsTrue()` and `withActiveIsNotTrue()`; exact value `true`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `rank()` / `updateRank(const std::int64_t&)`; changed `changedRank()`; evaluation `evalRank()`; expression `getRank()`.
- Candidate filters: `withRankIsValue7()` and `withRankIsNotValue7()`; exact value `-7`.
- Candidate filters: `withRankIsValue9()` and `withRankIsNotValue9()`; exact value `9`.
- Scalar predicates: `withRankIs`, `withRankIsNot`, `withRankGreaterThan`, `withRankGreaterThanOrEqualTo`, `withRankLessThan`, `withRankLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withRankBetween(lower, upper)`, `withRankIn(std::span<const std::int64_t>)`, `withRankNotIn(std::span<const std::int64_t>)`, `withRankIsNull()`, `withRankIsNotNull()`.
- Projection/order/group: `selectRank()`, `orderByRankAsc()`, `orderByRankDesc()`, `groupByRank()`.
- Group/count: `groupByRank()`, `groupByRankAs(alias)`, `groupByRankWithFunction(alias, function)`, `countRank()` and `countRankAs(alias)`.
- Scalar aggregate: `sumRank()` uses alias `sumRank`; `sumRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRank()` uses alias `avgRank`; `avgRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRank()` uses alias `minRank`; `minRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRank()` uses alias `maxRank`; `maxRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationRank()` and `standardDeviationRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationRank()` and `squareRootOfPopulationStandardDeviationRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceRank()` and `sampleVarianceRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceRank()` and `samplePopulationVarianceRankAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `weight()` / `updateWeight(const double&)`; changed `changedWeight()`; evaluation `evalWeight()`; expression `getWeight()`.
- Candidate filters: `withWeightIsValue25()` and `withWeightIsNotValue25()`; exact value `2.5`.
- Candidate filters: `withWeightIsValue475()` and `withWeightIsNotValue475()`; exact value `-4.75`.
- Scalar predicates: `withWeightIs`, `withWeightIsNot`, `withWeightGreaterThan`, `withWeightGreaterThanOrEqualTo`, `withWeightLessThan`, `withWeightLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withWeightBetween(lower, upper)`, `withWeightIn(std::span<const double>)`, `withWeightNotIn(std::span<const double>)`, `withWeightIsNull()`, `withWeightIsNotNull()`.
- Projection/order/group: `selectWeight()`, `orderByWeightAsc()`, `orderByWeightDesc()`, `groupByWeight()`.
- Group/count: `groupByWeight()`, `groupByWeightAs(alias)`, `groupByWeightWithFunction(alias, function)`, `countWeight()` and `countWeightAs(alias)`.
- Scalar aggregate: `sumWeight()` uses alias `sumWeight`; `sumWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgWeight()` uses alias `avgWeight`; `avgWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minWeight()` uses alias `minWeight`; `minWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxWeight()` uses alias `maxWeight`; `maxWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationWeight()` and `standardDeviationWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationWeight()` and `squareRootOfPopulationStandardDeviationWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceWeight()` and `sampleVarianceWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceWeight()` and `samplePopulationVarianceWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `ratio()` / `updateRatio(const float&)`; changed `changedRatio()`; evaluation `evalRatio()`; expression `getRatio()`.
- Candidate filters: `withRatioIsValue125f()` and `withRatioIsNotValue125f()`; exact value `1.25f`.
- Candidate filters: `withRatioIsValue05f()` and `withRatioIsNotValue05f()`; exact value `-0.5f`.
- Scalar predicates: `withRatioIs`, `withRatioIsNot`, `withRatioGreaterThan`, `withRatioGreaterThanOrEqualTo`, `withRatioLessThan`, `withRatioLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withRatioBetween(lower, upper)`, `withRatioIn(std::span<const float>)`, `withRatioNotIn(std::span<const float>)`, `withRatioIsNull()`, `withRatioIsNotNull()`.
- Projection/order/group: `selectRatio()`, `orderByRatioAsc()`, `orderByRatioDesc()`, `groupByRatio()`.
- Group/count: `groupByRatio()`, `groupByRatioAs(alias)`, `groupByRatioWithFunction(alias, function)`, `countRatio()` and `countRatioAs(alias)`.
- Scalar aggregate: `sumRatio()` uses alias `sumRatio`; `sumRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRatio()` uses alias `avgRatio`; `avgRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRatio()` uses alias `minRatio`; `minRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRatio()` uses alias `maxRatio`; `maxRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `cost()` / `updateCost(const Decimal&)`; changed `changedCost()`; evaluation `evalCost()`; expression `getCost()`.
- Candidate filters: `withCostIsValue1250()` and `withCostIsNotValue1250()`; exact value `12.50`.
- Candidate filters: `withCostIsValue3125()` and `withCostIsNotValue3125()`; exact value `-3.125`.
- Scalar predicates: `withCostIs`, `withCostIsNot`, `withCostGreaterThan`, `withCostGreaterThanOrEqualTo`, `withCostLessThan`, `withCostLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withCostBetween(lower, upper)`, `withCostIn(std::span<const Decimal>)`, `withCostNotIn(std::span<const Decimal>)`, `withCostIsNull()`, `withCostIsNotNull()`.
- Projection/order/group: `selectCost()`, `orderByCostAsc()`, `orderByCostDesc()`, `groupByCost()`.
- Group/count: `groupByCost()`, `groupByCostAs(alias)`, `groupByCostWithFunction(alias, function)`, `countCost()` and `countCostAs(alias)`.
- Scalar aggregate: `sumCost()` uses alias `sumCost`; `sumCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCost()` uses alias `avgCost`; `avgCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCost()` uses alias `minCost`; `minCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCost()` uses alias `maxCost`; `maxCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationCost()` and `standardDeviationCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationCost()` and `squareRootOfPopulationStandardDeviationCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceCost()` and `sampleVarianceCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceCost()` and `samplePopulationVarianceCostAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `day()` / `updateDay(const Date&)`; changed `changedDay()`; evaluation `evalDay()`; expression `getDay()`.
- Candidate filters: `withDayIsValue20240201()` and `withDayIsNotValue20240201()`; exact value `2024-02-01`.
- Candidate filters: `withDayIsValue20200409()` and `withDayIsNotValue20200409()`; exact value `2020-04-09`.
- Scalar predicates: `withDayIs`, `withDayIsNot`, `withDayGreaterThan`, `withDayGreaterThanOrEqualTo`, `withDayLessThan`, `withDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDayBetween(lower, upper)`, `withDayIn(std::span<const Date>)`, `withDayNotIn(std::span<const Date>)`, `withDayIsNull()`, `withDayIsNotNull()`.
- Projection/order/group: `selectDay()`, `orderByDayAsc()`, `orderByDayDesc()`, `groupByDay()`.
- Group/count: `groupByDay()`, `groupByDayAs(alias)`, `groupByDayWithFunction(alias, function)`, `countDay()` and `countDayAs(alias)`.
- Scalar aggregate: `sumDay()` uses alias `sumDay`; `sumDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDay()` uses alias `avgDay`; `avgDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDay()` uses alias `minDay`; `minDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDay()` uses alias `maxDay`; `maxDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `payload()` / `updatePayload(const std::optional<Json>&)`; changed `changedPayload()`; evaluation `evalPayload()`; expression `getPayload()`.
- Candidate filters: `withPayloadIsX1()` and `withPayloadIsNotX1()`; exact value `{"x":1}`.
- Candidate filters: `withPayloadIsX2()` and `withPayloadIsNotX2()`; exact value `{"x":2}`.
- Scalar predicates: `withPayloadIs`, `withPayloadIsNot`, `withPayloadGreaterThan`, `withPayloadGreaterThanOrEqualTo`, `withPayloadLessThan`, `withPayloadLessThanOrEqualTo`. Arguments: `const Json&`.
- Range/set/null predicates: `withPayloadBetween(lower, upper)`, `withPayloadIn(std::span<const Json>)`, `withPayloadNotIn(std::span<const Json>)`, `withPayloadIsNull()`, `withPayloadIsNotNull()`.
- Projection/order/group: `selectPayload()`, `orderByPayloadAsc()`, `orderByPayloadDesc()`, `groupByPayload()`.
- Group/count: `groupByPayload()`, `groupByPayloadAs(alias)`, `groupByPayloadWithFunction(alias, function)`, `countPayload()` and `countPayloadAs(alias)`.
- Scalar aggregate: `sumPayload()` uses alias `sumPayload`; `sumPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPayload()` uses alias `avgPayload`; `avgPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPayload()` uses alias `minPayload`; `minPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPayload()` uses alias `maxPayload`; `maxPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Candidate filters: `withNoteIsFirstNote()` and `withNoteIsNotFirstNote()`; exact value `First note`.
- Candidate filters: `withNoteIsSecondNote()` and `withNoteIsNotSecondNote()`; exact value `Second note`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `sumNote`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `avgNote`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `minNote`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `maxNote`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.
- Model: `optionalRank()` / `updateOptionalRank(const std::optional<std::int64_t>&)`; changed `changedOptionalRank()`; evaluation `evalOptionalRank()`; expression `getOptionalRank()`.
- Candidate filters: `withOptionalRankIsValue0()` and `withOptionalRankIsNotValue0()`; exact value `0`.
- Candidate filters: `withOptionalRankIsValue5()` and `withOptionalRankIsNotValue5()`; exact value `5`.
- Scalar predicates: `withOptionalRankIs`, `withOptionalRankIsNot`, `withOptionalRankGreaterThan`, `withOptionalRankGreaterThanOrEqualTo`, `withOptionalRankLessThan`, `withOptionalRankLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalRankBetween(lower, upper)`, `withOptionalRankIn(std::span<const std::int64_t>)`, `withOptionalRankNotIn(std::span<const std::int64_t>)`, `withOptionalRankIsNull()`, `withOptionalRankIsNotNull()`.
- Projection/order/group: `selectOptionalRank()`, `orderByOptionalRankAsc()`, `orderByOptionalRankDesc()`, `groupByOptionalRank()`.
- Group/count: `groupByOptionalRank()`, `groupByOptionalRankAs(alias)`, `groupByOptionalRankWithFunction(alias, function)`, `countOptionalRank()` and `countOptionalRankAs(alias)`.
- Scalar aggregate: `sumOptionalRank()` uses alias `sumOptionalRank`; `sumOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalRank()` uses alias `avgOptionalRank`; `avgOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalRank()` uses alias `minOptionalRank`; `minOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalRank()` uses alias `maxOptionalRank`; `maxOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalRank()` and `standardDeviationOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalRank()` and `squareRootOfPopulationStandardDeviationOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalRank()` and `sampleVarianceOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalRank()` and `samplePopulationVarianceOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
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
- Model relation evaluation: `evalLabelList()`; query view handle: `labelListHandle()`.
- Reverse relation: `labelList()` returns SmartList<Label>&; query `selectLabelList()`, `selectLabelListWith(LabelRequest<P>)`, `withLabelList(LabelRequest<P>)`; expression `getLabelList()`.
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
- Model relation evaluation: `evalPriorityList()`; query view handle: `priorityListHandle()`.
- Reverse relation: `priorityList()` returns SmartList<Priority>&; query `selectPriorityList()`, `selectPriorityListWith(PriorityRequest<P>)`, `withPriorityList(PriorityRequest<P>)`; expression `getPriorityList()`.
- Model relation evaluation: `evalLabelList()`; query view handle: `labelListHandle()`.
- Reverse relation: `labelList()` returns SmartList<Label>&; query `selectLabelList()`, `selectLabelListWith(LabelRequest<P>)`, `withLabelList(LabelRequest<P>)`; expression `getLabelList()`.
## Label
Entries: `Q.labels()` and `Q.labelsMinimal()`. Evaluation: `E::label(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.labelsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByPlatformAs(name, child)` defaults to all facets; `facetByPlatformAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Facet: `facetByPriorityAs(name, child)` defaults to all facets; `facetByPriorityAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Label. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Candidate filters: `withIdIsValue31()` and `withIdIsNotValue31()`; exact value `31`.
- Candidate filters: `withIdIsValue32()` and `withIdIsNotValue32()`; exact value `32`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `platformId()`; Context-root assignment is managed through UserContext; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `withPlatformIs`, `withPlatformIsNot`, `withPlatformGreaterThan`, `withPlatformGreaterThanOrEqualTo`, `withPlatformLessThan`, `withPlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPlatformBetween(lower, upper)`, `withPlatformIn(std::span<const std::uint64_t>)`, `withPlatformNotIn(std::span<const std::uint64_t>)`, `withPlatformIsNull()`, `withPlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*; Context-root assignment is managed through UserContext; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `withPlatform(PlatformRequest<P>)`; expression `getPlatform()`.
- Model: `priorityId()`; constant ID assignment is internal; changed `changedPriorityId()`; evaluation `evalPriorityId()`; expression `getPriorityId()`.
- Candidate filters: `withPriorityIsBasic()` and `withPriorityIsNotBasic()`; exact value `1`.
- Candidate model update: `updatePriorityToBasic()`; model and E check: `priorityIsBasic()`. E preserves NotLoaded and never performs SQL.
- Candidate filters: `withPriorityIsExtended()` and `withPriorityIsNotExtended()`; exact value `22`.
- Candidate model update: `updatePriorityToExtended()`; model and E check: `priorityIsExtended()`. E preserves NotLoaded and never performs SQL.
- Scalar predicates: `withPriorityIs`, `withPriorityIsNot`, `withPriorityGreaterThan`, `withPriorityGreaterThanOrEqualTo`, `withPriorityLessThan`, `withPriorityLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPriorityBetween(lower, upper)`, `withPriorityIn(std::span<const std::uint64_t>)`, `withPriorityNotIn(std::span<const std::uint64_t>)`, `withPriorityIsNull()`, `withPriorityIsNotNull()`.
- Projection/order/group: `selectPriorityId()`, `orderByPriorityIdAsc()`, `orderByPriorityIdDesc()`, `groupByPriority()`.
- Group/count: `groupByPriority()`, `groupByPriorityAs(alias)`, `groupByPriorityWithFunction(alias, function)`, `countPriority()` and `countPriorityAs(alias)`.
- Forward relation: `priority()` returns const Priority*; constant assignment uses candidate update methods; query `selectPriority()`, `selectPriorityWith(PriorityRequest<P>)`, `withPriority(PriorityRequest<P>)`; expression `getPriority()`.
- Model: `code()` / `updateCode(std::string_view)`; changed `changedCode()`; evaluation `evalCode()`; expression `getCode()`.
- Candidate filters: `withCodeIsPubliC()` and `withCodeIsNotPubliC()`; exact value `PUBLIC`.
- Candidate filters: `withCodeIsPrivatE()` and `withCodeIsNotPrivatE()`; exact value `PRIVATE`.
- Scalar predicates: `withCodeIs`, `withCodeIsNot`, `withCodeGreaterThan`, `withCodeGreaterThanOrEqualTo`, `withCodeLessThan`, `withCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withCodeBetween(lower, upper)`, `withCodeIn(std::span<const String>)`, `withCodeNotIn(std::span<const String>)`, `withCodeIsNull()`, `withCodeIsNotNull()`.
- Projection/order/group: `selectCode()`, `orderByCodeAsc()`, `orderByCodeDesc()`, `groupByCode()`.
- Group/count: `groupByCode()`, `groupByCodeAs(alias)`, `groupByCodeWithFunction(alias, function)`, `countCode()` and `countCodeAs(alias)`.
- Scalar aggregate: `sumCode()` uses alias `sumCode`; `sumCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCode()` uses alias `avgCode`; `avgCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCode()` uses alias `minCode`; `minCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCode()` uses alias `maxCode`; `maxCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withCodeContaining(text)`, `withCodeStartingWith(text)`, `withCodeEndingWith(text)`.
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
- Model relation evaluation: `evalPriority()` returns a checked entity borrow or Null/NotLoaded.

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
- Extend `PriorityCheckerLogic` and override `checkAndFixPriority(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `PriorityChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `PriorityBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `PlatformCheckerLogic` and override `checkAndFixPlatform(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `PlatformChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `PlatformBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `LabelCheckerLogic` and override `checkAndFixLabel(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `LabelChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `LabelBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
