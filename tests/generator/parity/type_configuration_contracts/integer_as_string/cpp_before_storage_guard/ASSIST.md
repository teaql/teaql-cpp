# Generated C++ model-aware API

Namespace: `cpp_type_configuration_fixture`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

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

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<Priority>(value)` or `user_context.decode_json_entities<Priority>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

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
- Model: `value()` / `updateValue(const std::int64_t&)`; changed `changedValue()`; evaluation `evalValue()`; expression `getValue()`.
- Candidate filters: `withValueIsValue7()` and `withValueIsNotValue7()`; exact value `7`.
- Candidate filters: `withValueIsValue9()` and `withValueIsNotValue9()`; exact value `9`.
- Scalar predicates: `withValueIs`, `withValueIsNot`, `withValueGreaterThan`, `withValueGreaterThanOrEqualTo`, `withValueLessThan`, `withValueLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withValueBetween(lower, upper)`, `withValueIn(std::span<const std::int64_t>)`, `withValueNotIn(std::span<const std::int64_t>)`, `withValueIsNull()`, `withValueIsNotNull()`.
- Projection/order/group: `selectValue()`, `orderByValueAsc()`, `orderByValueDesc()`, `groupByValue()`.
- Group/count: `groupByValue()`, `groupByValueAs(alias)`, `groupByValueWithFunction(alias, function)`, `countValue()` and `countValueAs(alias)`.
- Scalar aggregate: `sumValue()` uses alias `sumValue`; `sumValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgValue()` uses alias `avgValue`; `avgValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minValue()` uses alias `minValue`; `minValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxValue()` uses alias `maxValue`; `maxValueAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationValue()` and `standardDeviationValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationValue()` and `squareRootOfPopulationStandardDeviationValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceValue()` and `sampleVarianceValueAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceValue()` and `samplePopulationVarianceValueAs(alias)`; SQLite requires the corresponding SQL function.
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
## Platform
Entries: `Q.platforms()` and `Q.platformsMinimal()`. Evaluation: `E::platform(entity)`.

Inheritance/query metadata: `matchTypes(std::span<const Value>)`, `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.platformsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<Platform>(value)` or `user_context.decode_json_entities<Platform>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

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
