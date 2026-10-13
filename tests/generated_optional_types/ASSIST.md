# Generated C++ model-aware API

Namespace: `cpp_optional_types`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

## Reading
Entries: `Q.readings()` and `Q.readingsMinimal()`. Evaluation: `E::reading(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.readingsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `Orderby`, `Start`, `Size`, `Pagesize` and `Page`. Unknown fields are ignored as in the frozen Rust generator; malformed JSON throws a Context-borrowed diagnostic. Strings become contains predicates; numeric arrays of length two become ranges. Child queries keep their live-row filter even when the leaf is unknown.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Reading. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `sumId`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `avgId`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `minId`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `maxId`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `text()` / `updateText(const std::optional<String>&)`; changed `changedText()`; evaluation `evalText()`; expression `getText()`.
- Candidate filters: `withTextIsString()` and `withTextIsNotString()`; exact value `string()`.
- Scalar predicates: `withTextIs`, `withTextIsNot`, `withTextGreaterThan`, `withTextGreaterThanOrEqualTo`, `withTextLessThan`, `withTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withTextBetween(lower, upper)`, `withTextIn(std::span<const String>)`, `withTextNotIn(std::span<const String>)`, `withTextIsNull()`, `withTextIsNotNull()`.
- Projection/order/group: `selectText()`, `orderByTextAsc()`, `orderByTextDesc()`, `groupByText()`.
- Group/count: `groupByText()`, `groupByTextAs(alias)`, `groupByTextWithFunction(alias, function)`, `countText()` and `countTextAs(alias)`.
- Scalar aggregate: `sumText()` uses alias `sumText`; `sumTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgText()` uses alias `avgText`; `avgTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minText()` uses alias `minText`; `minTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxText()` uses alias `maxText`; `maxTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withTextContaining(text)`, `withTextStartingWith(text)`, `withTextEndingWith(text)`.
- Model: `active()` / `updateActive(const std::optional<bool>&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Candidate filters: `withActiveIsBool()` and `withActiveIsNotBool()`; exact value `bool()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `sumActive`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `avgActive`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `minActive`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `maxActive`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
- Model: `balance()` / `updateBalance(const std::optional<Decimal>&)`; changed `changedBalance()`; evaluation `evalBalance()`; expression `getBalance()`.
- Candidate filters: `withBalanceIsNumber()` and `withBalanceIsNotNumber()`; exact value `number()`.
- Scalar predicates: `withBalanceIs`, `withBalanceIsNot`, `withBalanceGreaterThan`, `withBalanceGreaterThanOrEqualTo`, `withBalanceLessThan`, `withBalanceLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withBalanceBetween(lower, upper)`, `withBalanceIn(std::span<const Decimal>)`, `withBalanceNotIn(std::span<const Decimal>)`, `withBalanceIsNull()`, `withBalanceIsNotNull()`.
- Projection/order/group: `selectBalance()`, `orderByBalanceAsc()`, `orderByBalanceDesc()`, `groupByBalance()`.
- Group/count: `groupByBalance()`, `groupByBalanceAs(alias)`, `groupByBalanceWithFunction(alias, function)`, `countBalance()` and `countBalanceAs(alias)`.
- Scalar aggregate: `sumBalance()` uses alias `sumBalance`; `sumBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgBalance()` uses alias `avgBalance`; `avgBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minBalance()` uses alias `minBalance`; `minBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxBalance()` uses alias `maxBalance`; `maxBalanceAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationBalance()` and `standardDeviationBalanceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationBalance()` and `squareRootOfPopulationStandardDeviationBalanceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceBalance()` and `sampleVarianceBalanceAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceBalance()` and `samplePopulationVarianceBalanceAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `height()` / `updateHeight(const std::optional<double>&)`; changed `changedHeight()`; evaluation `evalHeight()`; expression `getHeight()`.
- Candidate filters: `withHeightIsDouble()` and `withHeightIsNotDouble()`; exact value `double()`.
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
- Model: `ratio()` / `updateRatio(const std::optional<float>&)`; changed `changedRatio()`; evaluation `evalRatio()`; expression `getRatio()`.
- Candidate filters: `withRatioIsFloat()` and `withRatioIsNotFloat()`; exact value `float()`.
- Scalar predicates: `withRatioIs`, `withRatioIsNot`, `withRatioGreaterThan`, `withRatioGreaterThanOrEqualTo`, `withRatioLessThan`, `withRatioLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withRatioBetween(lower, upper)`, `withRatioIn(std::span<const float>)`, `withRatioNotIn(std::span<const float>)`, `withRatioIsNull()`, `withRatioIsNotNull()`.
- Projection/order/group: `selectRatio()`, `orderByRatioAsc()`, `orderByRatioDesc()`, `groupByRatio()`.
- Group/count: `groupByRatio()`, `groupByRatioAs(alias)`, `groupByRatioWithFunction(alias, function)`, `countRatio()` and `countRatioAs(alias)`.
- Scalar aggregate: `sumRatio()` uses alias `sumRatio`; `sumRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRatio()` uses alias `avgRatio`; `avgRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRatio()` uses alias `minRatio`; `minRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRatio()` uses alias `maxRatio`; `maxRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `count()` / `updateCount(const std::optional<std::int64_t>&)`; changed `changedCount()`; evaluation `evalCount()`; expression `getCount()`.
- Candidate filters: `withCountIsInteger()` and `withCountIsNotInteger()`; exact value `integer()`.
- Scalar predicates: `withCountIs`, `withCountIsNot`, `withCountGreaterThan`, `withCountGreaterThanOrEqualTo`, `withCountLessThan`, `withCountLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withCountBetween(lower, upper)`, `withCountIn(std::span<const std::int64_t>)`, `withCountNotIn(std::span<const std::int64_t>)`, `withCountIsNull()`, `withCountIsNotNull()`.
- Projection/order/group: `selectCount()`, `orderByCountAsc()`, `orderByCountDesc()`, `groupByCount()`.
- Group/count: `groupByCount()`, `groupByCountAs(alias)`, `groupByCountWithFunction(alias, function)`, `countCount()` and `countCountAs(alias)`.
- Scalar aggregate: `sumCount()` uses alias `sumCount`; `sumCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCount()` uses alias `avgCount`; `avgCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCount()` uses alias `minCount`; `minCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCount()` uses alias `maxCount`; `maxCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationCount()` and `standardDeviationCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationCount()` and `squareRootOfPopulationStandardDeviationCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceCount()` and `sampleVarianceCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceCount()` and `samplePopulationVarianceCountAs(alias)`; SQLite requires the corresponding SQL function.
- Model: `day()` / `updateDay(const std::optional<Date>&)`; changed `changedDay()`; evaluation `evalDay()`; expression `getDay()`.
- Candidate filters: `withDayIsDate()` and `withDayIsNotDate()`; exact value `date()`.
- Scalar predicates: `withDayIs`, `withDayIsNot`, `withDayGreaterThan`, `withDayGreaterThanOrEqualTo`, `withDayLessThan`, `withDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDayBetween(lower, upper)`, `withDayIn(std::span<const Date>)`, `withDayNotIn(std::span<const Date>)`, `withDayIsNull()`, `withDayIsNotNull()`.
- Projection/order/group: `selectDay()`, `orderByDayAsc()`, `orderByDayDesc()`, `groupByDay()`.
- Group/count: `groupByDay()`, `groupByDayAs(alias)`, `groupByDayWithFunction(alias, function)`, `countDay()` and `countDayAs(alias)`.
- Scalar aggregate: `sumDay()` uses alias `sumDay`; `sumDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDay()` uses alias `avgDay`; `avgDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDay()` uses alias `minDay`; `minDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDay()` uses alias `maxDay`; `maxDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `recordedAt()` / `updateRecordedAt(const std::optional<Timestamp>&)`; changed `changedRecordedAt()`; evaluation `evalRecordedAt()`; expression `getRecordedAt()`.
- Candidate filters: `withRecordedAtIsDateTime()` and `withRecordedAtIsNotDateTime()`; exact value `dateTime()`.
- Scalar predicates: `withRecordedAtIs`, `withRecordedAtIsNot`, `withRecordedAtGreaterThan`, `withRecordedAtGreaterThanOrEqualTo`, `withRecordedAtLessThan`, `withRecordedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withRecordedAtBetween(lower, upper)`, `withRecordedAtIn(std::span<const Timestamp>)`, `withRecordedAtNotIn(std::span<const Timestamp>)`, `withRecordedAtIsNull()`, `withRecordedAtIsNotNull()`.
- Projection/order/group: `selectRecordedAt()`, `orderByRecordedAtAsc()`, `orderByRecordedAtDesc()`, `groupByRecordedAt()`.
- Group/count: `groupByRecordedAt()`, `groupByRecordedAtAs(alias)`, `groupByRecordedAtWithFunction(alias, function)`, `countRecordedAt()` and `countRecordedAtAs(alias)`.
- Scalar aggregate: `sumRecordedAt()` uses alias `sumRecordedAt`; `sumRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRecordedAt()` uses alias `avgRecordedAt`; `avgRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRecordedAt()` uses alias `minRecordedAt`; `minRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRecordedAt()` uses alias `maxRecordedAt`; `maxRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Model: `payload()` / `updatePayload(const std::optional<Json>&)`; changed `changedPayload()`; evaluation `evalPayload()`; expression `getPayload()`.
- Candidate filters: `withPayloadIsJsonMe()` and `withPayloadIsNotJsonMe()`; exact value `jsonMe()`.
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
- Extend `ReadingCheckerLogic` and override `checkAndFixReading(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `ReadingChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `ReadingBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
