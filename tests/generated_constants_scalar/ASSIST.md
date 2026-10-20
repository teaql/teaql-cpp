# Generated C++ model-aware API

Namespace: `cpp_constants_scalar`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

Runtime support: [Schema](assist/runtime/schema.md), [Customization](assist/runtime/runtime_custom.md), [Diagnostics](assist/runtime/debug.md), [Tool availability](assist/runtime/tool_api.md).

Standalone registries: `repositoryRegistry(user_context)`, `behaviorRegistry(user_context)` and `checkerRegistry(user_context)` each allocate an independent Context-owned registry using every generated model's canonical name. They do not install themselves into the Context or execute SQL. Runtime registry lookup/replacement APIs retain snake_case; generated defaults are managed objects and checked callback borrows expire with their owner. Same-domain clones share those checked callbacks; cross-domain retention requires separately allocated destination callbacks, not an implicit shared owner.
`user_context.make<LocalSchemaProvider>()` provides the separate data-service schema lookup SPI. `get_entity(canonical_name)` returns a fresh Context-owned descriptor or null for an unknown/case-mismatched name. It always uses the generated static model descriptor, independently of mutable Context metadata and the runtime schema-initialization provider. Ordinary configuration binds the SQL executor to this provider. DataServiceDialect, DataServiceMutationExecutor, DataServiceMutationError and DataServiceIdGenerator expose the corresponding SQLite provider types; DataServiceConnection remains a checked Context-bound connection reference.
Runtime configuration: `ServiceRuntimeConfig::fromEnv(user_context)` reads the model-derived `DATABASE_URL_ENV` without caching it. `databaseUrl()` exposes the retained URL. Configuration and typed error payloads use the supplied Context; destroy value wrappers before Context close or explicitly copy into the destination scope.
`serviceRuntimeFromEnv(user_context)` and `serviceRuntime(user_context, config)` prepare a SQLite URL. These entries and `serviceRuntimeFromConnection(user_context, connection)` return a Result containing a checked executor reference; check the Result before business execution. The connection entry takes a checked `DataServiceConnection` (`Ref<SqliteConnection>`), including an explicitly rebound Global connection. Expired and foreign Context references are rejected; configuration from another domain requires an explicit copy. These entries register metadata, Checker/Behavior, IDs, provider and executor but leave schema initialization explicit. `configureRuntime(user_context, filename)` retains its direct filename and manual typed-key contract; `setup` additionally ensures schema. Typed ServiceRuntimeError preserves MissingEnv, ConnectionError, Configuration, provider and runtime payloads; ordinary C++ binding/allocation exceptions retain their established behavior. URL preparation preserves source slash-pair stripping and query removal; it does not enforce SQLite URI open modes.
Constant bootstrap uses normal generation. `user_context.ensure_schema()` creates/reconciles the ordinary Domain Root and constants through the shared runtime, governed saves and configured Checker/Policy. Constants retain declaration order and object seeds use identity 1; late targets can fail foreign-key checks. Fixed zero identities retain source allocation behavior and are not idempotent. Bootstrap retries source-defined conflicts up to five attempts; configured audit delivery errors may follow a committed write. Negative identities, candidate naming collisions, invalid roots and unsupported configured scalar carriers reject before output. C++ boolean/JSON and optional Timestamp/reference bindings have independent compiler controls where the original generated Rust crate cannot compile; they are not paired runtime evidence for those broken artifacts.

Internationalization: the generated `"teaql-i18n.json"` application catalog retains canonical model vocabulary and fifteen supported locales. Read it through application-owned I/O, then call `I18nCatalog::from_json(user_context.memory_resource(), source)` and check the Result before installation. After successful parsing, `user_context.make_ref<I18nCatalog>(std::move(parsed).value())` creates the managed catalog reference; install it with `user_context.set_i18n_catalog(catalog)`. Select language with `user_context.set_locale_code(code)` before Checker execution and check its Result: UnsupportedLocale does not change the active locale. The runtime does not read the catalog file implicitly or infer a request locale from the process environment. Lookup tries application selected locale, built-in selected locale, application default locale, built-in default locale, then the stable key. Keep rule IDs unchanged and translate display messages only. Parsed value wrappers must be destroyed before Context close; installed catalogs are checked Context borrows. Global retention requires an explicit destination copy and rebind, never shared ownership.

## Dynamic JSON value and control rules

Each entity's `filterWithJson(text)` entry below uses these rules. Use canonical modeled field and relationship names, not C++ accessor spellings. Operators are inferred from JSON values; an object is not a custom operator dictionary.

| JSON field value | Inferred condition |
| --- | --- |
| The exact strings `"__is_null__"` or `"__is_not_null__"` | IS NULL or IS NOT NULL; ASCII case-insensitive, before trimming. Surrounding spaces make an ordinary contains string. |
| Any other string | Contains, after trimming the string value. |
| A number or Boolean | Equality. |
| A nonempty array whose first element is a string or object | IN, including two-element arrays. |
| Any other two-element array | BETWEEN; this is a shape rule, not a numeric-type check. |
| Other values and arrays | Equality with the converted value, or the first converted array element. An empty array fails because equality has no first value. |

First-string/first-object IN takes precedence over two-element BETWEEN: `["a", "b"]` is IN, while `[1, 2]` and `[true, false]` are BETWEEN. Other multi-element arrays do not automatically mean IN. Objects recursively convert their `id` value; a missing id converts to NULL. For example, `{"id": {"id": 7}}` converts to 7. Strings are trimmed during conversion and nested arrays remain list values. Conversion does not validate that an operand suits a modeled field or SQLite operator.

Ordering: a bare self-field string orders descending. An object uses `field` and Boolean `useAsc` (default false); an array processes only object entries. Unknown fields and dotted ordering paths are ignored. Orders append to existing orders.

Paging: nonnegative integer `_start` sets the offset and `_size` sets the limit; `_pageSize` overrides `_size`. A positive `_page` then sets offset to (page - 1) times the explicit page size, existing limit or default 10, in that order; arithmetic saturates. Page zero and wrong control types are ignored. Controls run in this fixed order regardless of JSON key order.

Composition: filters and orders accumulate across calls; a non-object input is a no-op. Keys starting with an underscore are reserved; other unknown fields add no restriction. A recognized dotted relation retains its live-row child filter even if the leaf is unknown. Malformed JSON throws a Context-borrowed diagnostic. Query input does not install trusted metadata, supply request intent or authorize writes; validate the application's allowed filter surface.

## Priority
Progressive workflows: [Create](assist/priority/create.md), [Query](assist/priority/query.md), [Update](assist/priority/update.md), [Soft delete](assist/priority/delete.md), [List page](assist/priority/list_page.md), [Stream](assist/priority/stream.md), [E reads](assist/priority/expression.md).

Entries: `Q.priorities()` and `Q.prioritiesMinimal()`. Evaluation: `E::priority(entity)`.

Matching and scalar relation statistics: with...Matching/without...Matching, rollUpTo...With and field-statistic ...As methods accept unpurposed generated requests, QuerySelection or SelectQuery values. Matching embeds the bare query but retains complete filter metadata; its descriptor always comes from the declared relationship target. Roll-up combines matching and object grouping. Field statistics materialize the selection before adding the aggregate, preserving the original fresh-selection options boundary. SQLite errors for unsupported statistical functions remain errors. All inputs retain checked Context and explicit destination-copy requirements.
Relation analysis inputs: facetBy...As/AsWithOptions, groupBy...With/WithDetailsFrom, count...With, statsFrom.../As and reverse groupBy...WithDetails accept unpurposed generated requests, QuerySelection and SelectQuery values. Capture preserves complete selection metadata and snapshots without executing a child executor. Requests must share the checked Context binding; all inputs require the destination allocation domain or an explicit destination copy, including User-to-Global capture. Facet children need a nonblank comment at execution. Matching, roll-up and field-statistic methods retain separate conversion contracts.
Relation selection: `select...With(child)` accepts unpurposed requests from the same or another generated domain, or explicit QuerySelection/SelectQuery values. The runtime captures the materialized selection, including its options, without invoking a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Value inputs are kernel borrows and must remain alive through capture. Target-specific request overloads delegate to the same runtime capture and lifetime checks.
Child enhancement: `enhanceChild(child)` accepts an unpurposed request from the same or another generated domain, or explicit QuerySelection/SelectQuery values. It snapshots the complete selection and options without executing a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Ordinary wrapper captures preserve binding checks; explicit core values follow the kernel borrow contract.
Inheritance/query metadata: `matchTypes(types)` accepts Value spans and constrained input ranges, snapshots values into the bound Context and consumes single-pass ranges once. `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.prioritiesWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

Entity helpers: `Priority::withId(user_context, id)` constructs a U64 Value in the explicit UserContext domain. `isLoaded(field)` reads scalar/relation availability; `setLoadState(state)` copies into the model's allocation domain and validates its indexed layout before replacement. An incompatible layout throws and preserves the old state; changing availability does not change field values or stage mutations.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Scalar membership: source-supported `...In(inputs)` and `...NotIn(inputs)` accept input ranges whose elements convert to core Value, including arrays, vectors, spans and single-pass ranges. Values are copied into the bound request's allocator. Existing typed span overloads remain. The generic range overloads are omitted for Boolean, version, object-reference and Business-ID fields; Business-ID methods keep their distinct typed values. Neither route adds another Context argument.
Scalar comparisons: ordinary scalar Is/IsNot and non-ID ordered/Between methods also accept values constructible as core Value. Between can use independently typed lower/upper operands. Existing field-typed overloads remain, and the new overloads copy payloads into the bound allocator. Boolean/version/object-reference/Business-ID fields retain their separate contracts; unsupported values fail in constraints.
Text predicates: source-selected sound-like methods accept Value-constructible operands as well as text. Contains/start/end variants keep their text-only inputs. Generic sound-like overloads are omitted for large-text/password/Business-ID fields. Optional values are not implicitly converted to Value; use an explicit supported Value NULL or TypedNull when the intended criterion needs NULL. Numeric select...WithFunction helpers construct aggregate projections, and unselect removes the plain/raw field projection while preserving independent aliases and aggregates. SQLite rejects unsupported aggregate functions. The current default GBK compiler emits its argument unchanged, so the GBK marker does not establish GBK collation.
Request state before purpose: `coreQuery()` returns an independent bare core snapshot corresponding to the original query(&self). The existing `query()` keeps the integrated C++ runtime representation. Destroy the snapshot before its allocator closes or explicitly copy it into a destination scope. `relationSelections()`, `relationFilters()` and `childEnhancements()` return read-only Context borrows. `queryOptions()` returns an independent allocator-aware snapshot; destroy it before its owning Context closes or copy it explicitly into a destination scope. `std::move(request).intoQuery()` consumes the request and returns only its core SelectQuery, omitting option metadata and child enhancements as in the original direct into_query method. `std::move(request.selection()).into_query()` is a separate core conversion that applies options and enhancements. Neither conversion grants execution authority or extends Context lifetime. A consumed request rejects later access.
Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByPlatformAs(name, child)` defaults to all facets; `facetByPlatformAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<Priority>(value)` or `user_context.decode_json_entities<Priority>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `_orderBy`, `_start`, `_size`, `_pageSize` and `_page`. Read the [value and control rules](#dynamic-json-value-and-control-rules) before composing filters: array shape and first-element type select the operator, and unknown fields add no restriction.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Priority. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

Request result markers: `returnType<T>()` accepts a non-Entity marker for request construction, inspection and composition without allocating a T. All execution methods, including compact rows/count/exists, and `newEntity(user_context)` require an Entity result. Restore an Entity marker before purpose/execution; source identity, Context binding and explicit destination-copy rules remain unchanged.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Candidate filters: `withIdIsValue1()` and `withIdIsNotValue1()`; exact value `1`.
- Candidate filters: `withIdIsValue22()` and `withIdIsNotValue22()`; exact value `22`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `"sum_id"`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `"avg_id"`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `"min_id"`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `"max_id"`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [priority.id](assist/priority/fields/id.md).

- Model: `platformId()`; Context-root assignment is managed through UserContext; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `withPlatformIs`, `withPlatformIsNot`, `withPlatformGreaterThan`, `withPlatformGreaterThanOrEqualTo`, `withPlatformLessThan`, `withPlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPlatformBetween(lower, upper)`, `withPlatformIn(std::span<const std::uint64_t>)`, `withPlatformNotIn(std::span<const std::uint64_t>)`, `withPlatformIsNull()`, `withPlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*; Context-root assignment is managed through UserContext; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `withPlatform(PlatformRequest<P>)`; expression `getPlatform()`.
Field Assist: [priority.platform](assist/priority/fields/platform.md).

- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Candidate filters: `withNameIsBasic()` and `withNameIsNotBasic()`; exact value `Basic`.
- Candidate filters: `withNameIsExtended()` and `withNameIsNotExtended()`; exact value `Extended`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `"sum_name"`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `"avg_name"`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `"min_name"`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `"max_name"`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
Field Assist: [priority.name](assist/priority/fields/name.md).

- Model: `code()` / `updateCode(std::string_view)`; changed `changedCode()`; evaluation `evalCode()`; expression `getCode()`.
- Candidate filters: `withCodeIsBasiC()` and `withCodeIsNotBasiC()`; exact value `BASIC`.
- Candidate filters: `withCodeIsExtendeD()` and `withCodeIsNotExtendeD()`; exact value `EXTENDED`.
- Scalar predicates: `withCodeIs`, `withCodeIsNot`, `withCodeGreaterThan`, `withCodeGreaterThanOrEqualTo`, `withCodeLessThan`, `withCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withCodeBetween(lower, upper)`, `withCodeIn(std::span<const String>)`, `withCodeNotIn(std::span<const String>)`, `withCodeIsNull()`, `withCodeIsNotNull()`.
- Projection/order/group: `selectCode()`, `orderByCodeAsc()`, `orderByCodeDesc()`, `groupByCode()`.
- Group/count: `groupByCode()`, `groupByCodeAs(alias)`, `groupByCodeWithFunction(alias, function)`, `countCode()` and `countCodeAs(alias)`.
- Scalar aggregate: `sumCode()` uses alias `"sum_code"`; `sumCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCode()` uses alias `"avg_code"`; `avgCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCode()` uses alias `"min_code"`; `minCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCode()` uses alias `"max_code"`; `maxCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withCodeContaining(text)`, `withCodeStartingWith(text)`, `withCodeEndingWith(text)`.
Field Assist: [priority.code](assist/priority/fields/code.md).

- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Candidate filters: `withActiveIsFalse()` and `withActiveIsNotFalse()`; exact value `false`.
- Candidate filters: `withActiveIsTrue()` and `withActiveIsNotTrue()`; exact value `true`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `"sum_active"`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `"avg_active"`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `"min_active"`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `"max_active"`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
Field Assist: [priority.active](assist/priority/fields/active.md).

- Model: `rank()` / `updateRank(const std::int64_t&)`; changed `changedRank()`; evaluation `evalRank()`; expression `getRank()`.
- Candidate filters: `withRankIsValue7()` and `withRankIsNotValue7()`; exact value `-7`.
- Candidate filters: `withRankIsValue9()` and `withRankIsNotValue9()`; exact value `9`.
- Scalar predicates: `withRankIs`, `withRankIsNot`, `withRankGreaterThan`, `withRankGreaterThanOrEqualTo`, `withRankLessThan`, `withRankLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withRankBetween(lower, upper)`, `withRankIn(std::span<const std::int64_t>)`, `withRankNotIn(std::span<const std::int64_t>)`, `withRankIsNull()`, `withRankIsNotNull()`.
- Projection/order/group: `selectRank()`, `orderByRankAsc()`, `orderByRankDesc()`, `groupByRank()`.
- Group/count: `groupByRank()`, `groupByRankAs(alias)`, `groupByRankWithFunction(alias, function)`, `countRank()` and `countRankAs(alias)`.
- Scalar aggregate: `sumRank()` uses alias `"sum_rank"`; `sumRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRank()` uses alias `"avg_rank"`; `avgRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRank()` uses alias `"min_rank"`; `minRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRank()` uses alias `"max_rank"`; `maxRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationRank()` and `standardDeviationRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationRank()` and `squareRootOfPopulationStandardDeviationRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceRank()` and `sampleVarianceRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceRank()` and `samplePopulationVarianceRankAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [priority.rank](assist/priority/fields/rank.md).

- Model: `weight()` / `updateWeight(const double&)`; changed `changedWeight()`; evaluation `evalWeight()`; expression `getWeight()`.
- Candidate filters: `withWeightIsValue25()` and `withWeightIsNotValue25()`; exact value `2.5`.
- Candidate filters: `withWeightIsValue475()` and `withWeightIsNotValue475()`; exact value `-4.75`.
- Scalar predicates: `withWeightIs`, `withWeightIsNot`, `withWeightGreaterThan`, `withWeightGreaterThanOrEqualTo`, `withWeightLessThan`, `withWeightLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withWeightBetween(lower, upper)`, `withWeightIn(std::span<const double>)`, `withWeightNotIn(std::span<const double>)`, `withWeightIsNull()`, `withWeightIsNotNull()`.
- Projection/order/group: `selectWeight()`, `orderByWeightAsc()`, `orderByWeightDesc()`, `groupByWeight()`.
- Group/count: `groupByWeight()`, `groupByWeightAs(alias)`, `groupByWeightWithFunction(alias, function)`, `countWeight()` and `countWeightAs(alias)`.
- Scalar aggregate: `sumWeight()` uses alias `"sum_weight"`; `sumWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgWeight()` uses alias `"avg_weight"`; `avgWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minWeight()` uses alias `"min_weight"`; `minWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxWeight()` uses alias `"max_weight"`; `maxWeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationWeight()` and `standardDeviationWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationWeight()` and `squareRootOfPopulationStandardDeviationWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceWeight()` and `sampleVarianceWeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceWeight()` and `samplePopulationVarianceWeightAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [priority.weight](assist/priority/fields/weight.md).

- Model: `ratio()` / `updateRatio(const float&)`; changed `changedRatio()`; evaluation `evalRatio()`; expression `getRatio()`.
- Candidate filters: `withRatioIsValue125f()` and `withRatioIsNotValue125f()`; exact value `1.25f`.
- Candidate filters: `withRatioIsValue05f()` and `withRatioIsNotValue05f()`; exact value `-0.5f`.
- Scalar predicates: `withRatioIs`, `withRatioIsNot`, `withRatioGreaterThan`, `withRatioGreaterThanOrEqualTo`, `withRatioLessThan`, `withRatioLessThanOrEqualTo`. Arguments: `const float&`.
- Range/set/null predicates: `withRatioBetween(lower, upper)`, `withRatioIn(std::span<const float>)`, `withRatioNotIn(std::span<const float>)`, `withRatioIsNull()`, `withRatioIsNotNull()`.
- Projection/order/group: `selectRatio()`, `orderByRatioAsc()`, `orderByRatioDesc()`, `groupByRatio()`.
- Group/count: `groupByRatio()`, `groupByRatioAs(alias)`, `groupByRatioWithFunction(alias, function)`, `countRatio()` and `countRatioAs(alias)`.
- Scalar aggregate: `sumRatio()` uses alias `"sum_ratio"`; `sumRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRatio()` uses alias `"avg_ratio"`; `avgRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRatio()` uses alias `"min_ratio"`; `minRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRatio()` uses alias `"max_ratio"`; `maxRatioAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationRatio()` and `standardDeviationRatioAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationRatio()` and `squareRootOfPopulationStandardDeviationRatioAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceRatio()` and `sampleVarianceRatioAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceRatio()` and `samplePopulationVarianceRatioAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [priority.ratio](assist/priority/fields/ratio.md).

- Model: `cost()` / `updateCost(const Decimal&)`; changed `changedCost()`; evaluation `evalCost()`; expression `getCost()`.
- Candidate filters: `withCostIsValue1250()` and `withCostIsNotValue1250()`; exact value `12.50`.
- Candidate filters: `withCostIsValue3125()` and `withCostIsNotValue3125()`; exact value `-3.125`.
- Scalar predicates: `withCostIs`, `withCostIsNot`, `withCostGreaterThan`, `withCostGreaterThanOrEqualTo`, `withCostLessThan`, `withCostLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withCostBetween(lower, upper)`, `withCostIn(std::span<const Decimal>)`, `withCostNotIn(std::span<const Decimal>)`, `withCostIsNull()`, `withCostIsNotNull()`.
- Projection/order/group: `selectCost()`, `orderByCostAsc()`, `orderByCostDesc()`, `groupByCost()`.
- Group/count: `groupByCost()`, `groupByCostAs(alias)`, `groupByCostWithFunction(alias, function)`, `countCost()` and `countCostAs(alias)`.
- Scalar aggregate: `sumCost()` uses alias `"sum_cost"`; `sumCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCost()` uses alias `"avg_cost"`; `avgCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCost()` uses alias `"min_cost"`; `minCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCost()` uses alias `"max_cost"`; `maxCostAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationCost()` and `standardDeviationCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationCost()` and `squareRootOfPopulationStandardDeviationCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceCost()` and `sampleVarianceCostAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceCost()` and `samplePopulationVarianceCostAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [priority.cost](assist/priority/fields/cost.md).

- Model: `day()` / `updateDay(const Date&)`; changed `changedDay()`; evaluation `evalDay()`; expression `getDay()`.
- Candidate filters: `withDayIsValue20240201()` and `withDayIsNotValue20240201()`; exact value `2024-02-01`.
- Candidate filters: `withDayIsValue20200409()` and `withDayIsNotValue20200409()`; exact value `2020-04-09`.
- Scalar predicates: `withDayIs`, `withDayIsNot`, `withDayGreaterThan`, `withDayGreaterThanOrEqualTo`, `withDayLessThan`, `withDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDayBetween(lower, upper)`, `withDayIn(std::span<const Date>)`, `withDayNotIn(std::span<const Date>)`, `withDayIsNull()`, `withDayIsNotNull()`.
- Projection/order/group: `selectDay()`, `orderByDayAsc()`, `orderByDayDesc()`, `groupByDay()`.
- Group/count: `groupByDay()`, `groupByDayAs(alias)`, `groupByDayWithFunction(alias, function)`, `countDay()` and `countDayAs(alias)`.
- Scalar aggregate: `sumDay()` uses alias `"sum_day"`; `sumDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDay()` uses alias `"avg_day"`; `avgDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDay()` uses alias `"min_day"`; `minDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDay()` uses alias `"max_day"`; `maxDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [priority.day](assist/priority/fields/day.md).

- Model: `payload()` / `updatePayload(const std::optional<Json>&)`; changed `changedPayload()`; evaluation `evalPayload()`; expression `getPayload()`.
- Candidate filters: `withPayloadIsX1()` and `withPayloadIsNotX1()`; exact value `{"x":1}`.
- Candidate filters: `withPayloadIsX2()` and `withPayloadIsNotX2()`; exact value `{"x":2}`.
- Scalar predicates: `withPayloadIs`, `withPayloadIsNot`, `withPayloadGreaterThan`, `withPayloadGreaterThanOrEqualTo`, `withPayloadLessThan`, `withPayloadLessThanOrEqualTo`. Arguments: `const Json&`.
- Range/set/null predicates: `withPayloadBetween(lower, upper)`, `withPayloadIn(std::span<const Json>)`, `withPayloadNotIn(std::span<const Json>)`, `withPayloadIsNull()`, `withPayloadIsNotNull()`.
- Projection/order/group: `selectPayload()`, `orderByPayloadAsc()`, `orderByPayloadDesc()`, `groupByPayload()`.
- Group/count: `groupByPayload()`, `groupByPayloadAs(alias)`, `groupByPayloadWithFunction(alias, function)`, `countPayload()` and `countPayloadAs(alias)`.
- Scalar aggregate: `sumPayload()` uses alias `"sum_payload"`; `sumPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPayload()` uses alias `"avg_payload"`; `avgPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPayload()` uses alias `"min_payload"`; `minPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPayload()` uses alias `"max_payload"`; `maxPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [priority.payload](assist/priority/fields/payload.md).

- Model: `note()` / `updateNote(const std::optional<String>&)`; changed `changedNote()`; evaluation `evalNote()`; expression `getNote()`.
- Candidate filters: `withNoteIsFirstNote()` and `withNoteIsNotFirstNote()`; exact value `First note`.
- Candidate filters: `withNoteIsSecondNote()` and `withNoteIsNotSecondNote()`; exact value `Second note`.
- Scalar predicates: `withNoteIs`, `withNoteIsNot`, `withNoteGreaterThan`, `withNoteGreaterThanOrEqualTo`, `withNoteLessThan`, `withNoteLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNoteBetween(lower, upper)`, `withNoteIn(std::span<const String>)`, `withNoteNotIn(std::span<const String>)`, `withNoteIsNull()`, `withNoteIsNotNull()`.
- Projection/order/group: `selectNote()`, `orderByNoteAsc()`, `orderByNoteDesc()`, `groupByNote()`.
- Group/count: `groupByNote()`, `groupByNoteAs(alias)`, `groupByNoteWithFunction(alias, function)`, `countNote()` and `countNoteAs(alias)`.
- Scalar aggregate: `sumNote()` uses alias `"sum_note"`; `sumNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNote()` uses alias `"avg_note"`; `avgNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNote()` uses alias `"min_note"`; `minNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNote()` uses alias `"max_note"`; `maxNoteAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNoteContaining(text)`, `withNoteStartingWith(text)`, `withNoteEndingWith(text)`.
Field Assist: [priority.note](assist/priority/fields/note.md).

- Model: `optionalRank()` / `updateOptionalRank(const std::optional<std::int64_t>&)`; changed `changedOptionalRank()`; evaluation `evalOptionalRank()`; expression `getOptionalRank()`.
- Candidate filters: `withOptionalRankIsValue0()` and `withOptionalRankIsNotValue0()`; exact value `0`.
- Candidate filters: `withOptionalRankIsValue5()` and `withOptionalRankIsNotValue5()`; exact value `5`.
- Scalar predicates: `withOptionalRankIs`, `withOptionalRankIsNot`, `withOptionalRankGreaterThan`, `withOptionalRankGreaterThanOrEqualTo`, `withOptionalRankLessThan`, `withOptionalRankLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalRankBetween(lower, upper)`, `withOptionalRankIn(std::span<const std::int64_t>)`, `withOptionalRankNotIn(std::span<const std::int64_t>)`, `withOptionalRankIsNull()`, `withOptionalRankIsNotNull()`.
- Projection/order/group: `selectOptionalRank()`, `orderByOptionalRankAsc()`, `orderByOptionalRankDesc()`, `groupByOptionalRank()`.
- Group/count: `groupByOptionalRank()`, `groupByOptionalRankAs(alias)`, `groupByOptionalRankWithFunction(alias, function)`, `countOptionalRank()` and `countOptionalRankAs(alias)`.
- Scalar aggregate: `sumOptionalRank()` uses alias `"sum_optional_rank"`; `sumOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalRank()` uses alias `"avg_optional_rank"`; `avgOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalRank()` uses alias `"min_optional_rank"`; `minOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalRank()` uses alias `"max_optional_rank"`; `maxOptionalRankAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalRank()` and `standardDeviationOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalRank()` and `squareRootOfPopulationStandardDeviationOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalRank()` and `sampleVarianceOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalRank()` and `samplePopulationVarianceOptionalRankAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [priority.optional_rank](assist/priority/fields/optional_rank.md).

- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `"sum_version"`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `"avg_version"`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `"min_version"`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `"max_version"`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [priority.version](assist/priority/fields/version.md).

- Model relation evaluation: `evalPlatform()` returns a checked entity borrow or Null/NotLoaded.
- Model relation evaluation: `evalLabelList()`; query view handle: `labelListHandle()`.
- Reverse relation: `labelList()` returns SmartList<Label>&; query `selectLabelList()`, `selectLabelListWith(LabelRequest<P>)`, `withLabelList(LabelRequest<P>)`; expression `getLabelList()`.
Field Assist: [priority.label_list](assist/priority/fields/label_list.md).

## Platform
Progressive workflows: [Create](assist/platform/create.md), [Query](assist/platform/query.md), [Update](assist/platform/update.md), [Soft delete](assist/platform/delete.md), [List page](assist/platform/list_page.md), [Stream](assist/platform/stream.md), [E reads](assist/platform/expression.md).

Entries: `Q.platforms()` and `Q.platformsMinimal()`. Evaluation: `E::platform(entity)`.

Matching and scalar relation statistics: with...Matching/without...Matching, rollUpTo...With and field-statistic ...As methods accept unpurposed generated requests, QuerySelection or SelectQuery values. Matching embeds the bare query but retains complete filter metadata; its descriptor always comes from the declared relationship target. Roll-up combines matching and object grouping. Field statistics materialize the selection before adding the aggregate, preserving the original fresh-selection options boundary. SQLite errors for unsupported statistical functions remain errors. All inputs retain checked Context and explicit destination-copy requirements.
Relation analysis inputs: facetBy...As/AsWithOptions, groupBy...With/WithDetailsFrom, count...With, statsFrom.../As and reverse groupBy...WithDetails accept unpurposed generated requests, QuerySelection and SelectQuery values. Capture preserves complete selection metadata and snapshots without executing a child executor. Requests must share the checked Context binding; all inputs require the destination allocation domain or an explicit destination copy, including User-to-Global capture. Facet children need a nonblank comment at execution. Matching, roll-up and field-statistic methods retain separate conversion contracts.
Relation selection: `select...With(child)` accepts unpurposed requests from the same or another generated domain, or explicit QuerySelection/SelectQuery values. The runtime captures the materialized selection, including its options, without invoking a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Value inputs are kernel borrows and must remain alive through capture. Target-specific request overloads delegate to the same runtime capture and lifetime checks.
Child enhancement: `enhanceChild(child)` accepts an unpurposed request from the same or another generated domain, or explicit QuerySelection/SelectQuery values. It snapshots the complete selection and options without executing a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Ordinary wrapper captures preserve binding checks; explicit core values follow the kernel borrow contract.
Inheritance/query metadata: `matchTypes(types)` accepts Value spans and constrained input ranges, snapshots values into the bound Context and consumes single-pass ranges once. `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.platformsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

Entity helpers: `Platform::withId(user_context, id)` constructs a U64 Value in the explicit UserContext domain. `isLoaded(field)` reads scalar/relation availability; `setLoadState(state)` copies into the model's allocation domain and validates its indexed layout before replacement. An incompatible layout throws and preserves the old state; changing availability does not change field values or stage mutations.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Scalar membership: source-supported `...In(inputs)` and `...NotIn(inputs)` accept input ranges whose elements convert to core Value, including arrays, vectors, spans and single-pass ranges. Values are copied into the bound request's allocator. Existing typed span overloads remain. The generic range overloads are omitted for Boolean, version, object-reference and Business-ID fields; Business-ID methods keep their distinct typed values. Neither route adds another Context argument.
Scalar comparisons: ordinary scalar Is/IsNot and non-ID ordered/Between methods also accept values constructible as core Value. Between can use independently typed lower/upper operands. Existing field-typed overloads remain, and the new overloads copy payloads into the bound allocator. Boolean/version/object-reference/Business-ID fields retain their separate contracts; unsupported values fail in constraints.
Text predicates: source-selected sound-like methods accept Value-constructible operands as well as text. Contains/start/end variants keep their text-only inputs. Generic sound-like overloads are omitted for large-text/password/Business-ID fields. Optional values are not implicitly converted to Value; use an explicit supported Value NULL or TypedNull when the intended criterion needs NULL. Numeric select...WithFunction helpers construct aggregate projections, and unselect removes the plain/raw field projection while preserving independent aliases and aggregates. SQLite rejects unsupported aggregate functions. The current default GBK compiler emits its argument unchanged, so the GBK marker does not establish GBK collation.
Request state before purpose: `coreQuery()` returns an independent bare core snapshot corresponding to the original query(&self). The existing `query()` keeps the integrated C++ runtime representation. Destroy the snapshot before its allocator closes or explicitly copy it into a destination scope. `relationSelections()`, `relationFilters()` and `childEnhancements()` return read-only Context borrows. `queryOptions()` returns an independent allocator-aware snapshot; destroy it before its owning Context closes or copy it explicitly into a destination scope. `std::move(request).intoQuery()` consumes the request and returns only its core SelectQuery, omitting option metadata and child enhancements as in the original direct into_query method. `std::move(request.selection()).into_query()` is a separate core conversion that applies options and enhancements. Neither conversion grants execution authority or extends Context lifetime. A consumed request rejects later access.
Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<Platform>(value)` or `user_context.decode_json_entities<Platform>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `_orderBy`, `_start`, `_size`, `_pageSize` and `_page`. Read the [value and control rules](#dynamic-json-value-and-control-rules) before composing filters: array shape and first-element type select the operator, and unknown fields add no restriction.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Platform. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

Request result markers: `returnType<T>()` accepts a non-Entity marker for request construction, inspection and composition without allocating a T. All execution methods, including compact rows/count/exists, and `newEntity(user_context)` require an Entity result. Restore an Entity marker before purpose/execution; source identity, Context binding and explicit destination-copy rules remain unchanged.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `"sum_id"`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `"avg_id"`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `"min_id"`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `"max_id"`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [platform.id](assist/platform/fields/id.md).

- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Candidate filters: `withNameIsString()` and `withNameIsNotString()`; exact value `string()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `"sum_name"`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `"avg_name"`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `"min_name"`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `"max_name"`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
Field Assist: [platform.name](assist/platform/fields/name.md).

- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `"sum_version"`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `"avg_version"`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `"min_version"`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `"max_version"`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [platform.version](assist/platform/fields/version.md).

- Model relation evaluation: `evalPriorityList()`; query view handle: `priorityListHandle()`.
- Reverse relation: `priorityList()` returns SmartList<Priority>&; query `selectPriorityList()`, `selectPriorityListWith(PriorityRequest<P>)`, `withPriorityList(PriorityRequest<P>)`; expression `getPriorityList()`.
Field Assist: [platform.priority_list](assist/platform/fields/priority_list.md).

- Model relation evaluation: `evalLabelList()`; query view handle: `labelListHandle()`.
- Reverse relation: `labelList()` returns SmartList<Label>&; query `selectLabelList()`, `selectLabelListWith(LabelRequest<P>)`, `withLabelList(LabelRequest<P>)`; expression `getLabelList()`.
Field Assist: [platform.label_list](assist/platform/fields/label_list.md).

## Label
Progressive workflows: [Create](assist/label/create.md), [Query](assist/label/query.md), [Update](assist/label/update.md), [Soft delete](assist/label/delete.md), [List page](assist/label/list_page.md), [Stream](assist/label/stream.md), [E reads](assist/label/expression.md).

Entries: `Q.labels()` and `Q.labelsMinimal()`. Evaluation: `E::label(entity)`.

Matching and scalar relation statistics: with...Matching/without...Matching, rollUpTo...With and field-statistic ...As methods accept unpurposed generated requests, QuerySelection or SelectQuery values. Matching embeds the bare query but retains complete filter metadata; its descriptor always comes from the declared relationship target. Roll-up combines matching and object grouping. Field statistics materialize the selection before adding the aggregate, preserving the original fresh-selection options boundary. SQLite errors for unsupported statistical functions remain errors. All inputs retain checked Context and explicit destination-copy requirements.
Relation analysis inputs: facetBy...As/AsWithOptions, groupBy...With/WithDetailsFrom, count...With, statsFrom.../As and reverse groupBy...WithDetails accept unpurposed generated requests, QuerySelection and SelectQuery values. Capture preserves complete selection metadata and snapshots without executing a child executor. Requests must share the checked Context binding; all inputs require the destination allocation domain or an explicit destination copy, including User-to-Global capture. Facet children need a nonblank comment at execution. Matching, roll-up and field-statistic methods retain separate conversion contracts.
Relation selection: `select...With(child)` accepts unpurposed requests from the same or another generated domain, or explicit QuerySelection/SelectQuery values. The runtime captures the materialized selection, including its options, without invoking a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Value inputs are kernel borrows and must remain alive through capture. Target-specific request overloads delegate to the same runtime capture and lifetime checks.
Child enhancement: `enhanceChild(child)` accepts an unpurposed request from the same or another generated domain, or explicit QuerySelection/SelectQuery values. It snapshots the complete selection and options without executing a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Ordinary wrapper captures preserve binding checks; explicit core values follow the kernel borrow contract.
Inheritance/query metadata: `matchTypes(types)` accepts Value spans and constrained input ranges, snapshots values into the bound Context and consumes single-pass ranges once. `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.labelsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

Entity helpers: `Label::withId(user_context, id)` constructs a U64 Value in the explicit UserContext domain. `isLoaded(field)` reads scalar/relation availability; `setLoadState(state)` copies into the model's allocation domain and validates its indexed layout before replacement. An incompatible layout throws and preserves the old state; changing availability does not change field values or stage mutations.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Scalar membership: source-supported `...In(inputs)` and `...NotIn(inputs)` accept input ranges whose elements convert to core Value, including arrays, vectors, spans and single-pass ranges. Values are copied into the bound request's allocator. Existing typed span overloads remain. The generic range overloads are omitted for Boolean, version, object-reference and Business-ID fields; Business-ID methods keep their distinct typed values. Neither route adds another Context argument.
Scalar comparisons: ordinary scalar Is/IsNot and non-ID ordered/Between methods also accept values constructible as core Value. Between can use independently typed lower/upper operands. Existing field-typed overloads remain, and the new overloads copy payloads into the bound allocator. Boolean/version/object-reference/Business-ID fields retain their separate contracts; unsupported values fail in constraints.
Text predicates: source-selected sound-like methods accept Value-constructible operands as well as text. Contains/start/end variants keep their text-only inputs. Generic sound-like overloads are omitted for large-text/password/Business-ID fields. Optional values are not implicitly converted to Value; use an explicit supported Value NULL or TypedNull when the intended criterion needs NULL. Numeric select...WithFunction helpers construct aggregate projections, and unselect removes the plain/raw field projection while preserving independent aliases and aggregates. SQLite rejects unsupported aggregate functions. The current default GBK compiler emits its argument unchanged, so the GBK marker does not establish GBK collation.
Request state before purpose: `coreQuery()` returns an independent bare core snapshot corresponding to the original query(&self). The existing `query()` keeps the integrated C++ runtime representation. Destroy the snapshot before its allocator closes or explicitly copy it into a destination scope. `relationSelections()`, `relationFilters()` and `childEnhancements()` return read-only Context borrows. `queryOptions()` returns an independent allocator-aware snapshot; destroy it before its owning Context closes or copy it explicitly into a destination scope. `std::move(request).intoQuery()` consumes the request and returns only its core SelectQuery, omitting option metadata and child enhancements as in the original direct into_query method. `std::move(request.selection()).into_query()` is a separate core conversion that applies options and enhancements. Neither conversion grants execution authority or extends Context lifetime. A consumed request rejects later access.
Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByPlatformAs(name, child)` defaults to all facets; `facetByPlatformAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Facet: `facetByPriorityAs(name, child)` defaults to all facets; `facetByPriorityAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<Label>(value)` or `user_context.decode_json_entities<Label>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `_orderBy`, `_start`, `_size`, `_pageSize` and `_page`. Read the [value and control rules](#dynamic-json-value-and-control-rules) before composing filters: array shape and first-element type select the operator, and unknown fields add no restriction.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates Label. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

Request result markers: `returnType<T>()` accepts a non-Entity marker for request construction, inspection and composition without allocating a T. All execution methods, including compact rows/count/exists, and `newEntity(user_context)` require an Entity result. Restore an Entity marker before purpose/execution; source identity, Context binding and explicit destination-copy rules remain unchanged.

- Model: `id()` / `updateId(const std::uint64_t&)`; changed `changedId()`; evaluation `evalId()`; expression `getId()`.
- Candidate filters: `withIdIsValue31()` and `withIdIsNotValue31()`; exact value `31`.
- Candidate filters: `withIdIsValue32()` and `withIdIsNotValue32()`; exact value `32`.
- Scalar predicates: `withIdIs`, `withIdIsNot`, `withIdGreaterThan`, `withIdGreaterThanOrEqualTo`, `withIdLessThan`, `withIdLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withIdBetween(lower, upper)`, `withIdIn(std::span<const std::uint64_t>)`, `withIdNotIn(std::span<const std::uint64_t>)`, `withIdIsNull()`, `withIdIsNotNull()`.
- Projection/order/group: `selectId()`, `orderByIdAsc()`, `orderByIdDesc()`, `groupById()`.
- Group/count: `groupById()`, `groupByIdAs(alias)`, `groupByIdWithFunction(alias, function)`, `countId()` and `countIdAs(alias)`.
- Scalar aggregate: `sumId()` uses alias `"sum_id"`; `sumIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgId()` uses alias `"avg_id"`; `avgIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minId()` uses alias `"min_id"`; `minIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxId()` uses alias `"max_id"`; `maxIdAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [label.id](assist/label/fields/id.md).

- Model: `platformId()`; Context-root assignment is managed through UserContext; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `withPlatformIs`, `withPlatformIsNot`, `withPlatformGreaterThan`, `withPlatformGreaterThanOrEqualTo`, `withPlatformLessThan`, `withPlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPlatformBetween(lower, upper)`, `withPlatformIn(std::span<const std::uint64_t>)`, `withPlatformNotIn(std::span<const std::uint64_t>)`, `withPlatformIsNull()`, `withPlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*; Context-root assignment is managed through UserContext; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `withPlatform(PlatformRequest<P>)`; expression `getPlatform()`.
Field Assist: [label.platform](assist/label/fields/platform.md).

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
Field Assist: [label.priority](assist/label/fields/priority.md).

- Model: `code()` / `updateCode(std::string_view)`; changed `changedCode()`; evaluation `evalCode()`; expression `getCode()`.
- Candidate filters: `withCodeIsPubliC()` and `withCodeIsNotPubliC()`; exact value `PUBLIC`.
- Candidate filters: `withCodeIsPrivatE()` and `withCodeIsNotPrivatE()`; exact value `PRIVATE`.
- Scalar predicates: `withCodeIs`, `withCodeIsNot`, `withCodeGreaterThan`, `withCodeGreaterThanOrEqualTo`, `withCodeLessThan`, `withCodeLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withCodeBetween(lower, upper)`, `withCodeIn(std::span<const String>)`, `withCodeNotIn(std::span<const String>)`, `withCodeIsNull()`, `withCodeIsNotNull()`.
- Projection/order/group: `selectCode()`, `orderByCodeAsc()`, `orderByCodeDesc()`, `groupByCode()`.
- Group/count: `groupByCode()`, `groupByCodeAs(alias)`, `groupByCodeWithFunction(alias, function)`, `countCode()` and `countCodeAs(alias)`.
- Scalar aggregate: `sumCode()` uses alias `"sum_code"`; `sumCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgCode()` uses alias `"avg_code"`; `avgCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minCode()` uses alias `"min_code"`; `minCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxCode()` uses alias `"max_code"`; `maxCodeAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withCodeContaining(text)`, `withCodeStartingWith(text)`, `withCodeEndingWith(text)`.
Field Assist: [label.code](assist/label/fields/code.md).

- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `"sum_version"`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `"avg_version"`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `"min_version"`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `"max_version"`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [label.version](assist/label/fields/version.md).

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
Default NULL fallback is available only for scalar payloads with a default value. Strong Business ID payloads have no public default: use a typed `orIfNull(fallback)` or `orElseIfNull(callback)` instead. Missing-load errors never invoke a fallback.
Missing-load diagnostics: catch `ExpressionNotLoadedError` and inspect `error.diagnostic()` for the captured root, access_path, failed_node, missing_preload, suggested_fix, optional nested_fix, severity and English human_message. Suggestions use generated C++ camelCase selection names. The error borrows its Context; diagnostic access fails after close. `what()` is static and remains safe. Retention requires an explicit destination copy before closing the source Context.
Diagnostic repair scope: a loaded relationship can leave a leaf-only diagnostic path. Apply the selection to that relationship's child request and retain the enclosing selection. Nested C++ selections use `select...With(child)`; leaf selections use `select...()`. Renderer entry placeholders and captured id=... expressions are explanatory pseudocode. Use current object/field Assist to resolve the actual Q entry, child type and selection names before executing a repair.
Expression composition: `map(callback)`, `andThen(node, callback)` and owning scalar `copyTo(user_context)`. Entity/list bindings are checked borrows; temporary entity entries are rejected. Scalar access is an eager snapshot. All Q/E wrappers expire with their scope; Global diagnostics must outlive surviving wrappers.
Save: `AuditedSaveExt<Model>::save(std::move(entity).audit_as("reason"), user_context).wait()`. Allocate requests/models with UserContext and call setup(user_context) once at bootstrap. Use explicit destination copies before closing the owning context. Unknown fields are compile errors.
Sample data: `generateSampleData(user_context, SampleDataPlan{SampleDataScale::Tiny, seed}).wait()` uses Context-bound queries and audited saves. It loads roots/constants, skips missing required dependencies, and generates business rows in depth/name order. Run setup first. Reference selection and numeric values follow the seed/index rules; temporal fields use the wall clock. Repeated calls create additional business rows. Reports and errors belong to UserContext; copy explicitly before close.
Sample failure: generation stops at the first save error; earlier successful saves remain committed. A failed invocation returns an error, and a later invocation may create another business batch.
Sample progress: register an application-owned `SampleDataLogSink` through `user_context.insert_resource(owner, static_cast<SampleDataLogSink&>(sink))`. Its synchronous `info(user_context, message)` receives the original sample Info messages in execution order. Allocate the sink in the declared Global/User owner; `memory_resource()` must identify that owner. Missing or disabled sinks do not format messages. Messages are callback borrows; retaining them requires an explicit copy into the sink owner's allocator. Pending re-polls do not repeat start; cancellation and save failure emit no successful completion. A callback must not close its active Context, and exceptions propagate as operation failure. This sample-specific sink does not replace SQL diagnostics or audit sinks.
Deletion: `entity.markForDeletion()` records deletion in the ledger and returns the model for chaining. Save through the audited save API; marking alone does not execute SQL. Versioned SQLite models use the runtime soft-delete protocol.
Projection shortcuts: `selectSelfFields()` and `selectSelfWithoutParent()` both select native fields, including inherited fields as the original generator does. `selectAll()` also selects forward and reverse to-one relations; `selectChildren()` adds reverse collections, and `selectAny()` aliases it. These do not replace `enhanceChildrenIfNeeded()` for polymorphic descendants. Named relation selections can include soft-deleted child rows; they do not automatically apply a child Q entry's default live-row filter. Use an explicit select...With(child_request) to carry the child's filters and projection.
Runtime attachment: `entity.attachRuntimeStateRecursive(target_state)` adopts pending mutations and preserves the entity's loaded snapshot. It recursively attaches stored forward objects; it does not load relations or traverse graph lookups/reverse collections. Both states must belong to the same Context; cross-domain retention requires an explicit destination copy.
## Business IDs
## Model relation evaluation
Model evalX() and E.getX() share the runtime relation rules. Loaded empty lists are values, selected empty to-one relations are Null, and missing views are NotLoaded. Returned entity/list values are checked Context borrows. Reverse XHandle() methods expose the immutable query-installed view; existing mutable list getters remain explicit model-local graph construction adapters. No getter performs implicit SQL.
## Checker and behavior assembly
Wire metadata: `generatedWireFieldMappings(user_context)` returns Context-owned typed maps from canonical model field names to the selected JSON wire spelling. Like the original template, each entity entry lists its own fields; it does not flatten inherited fields or rename relation keys to FK storage keys. `generatedWireFieldAliases(user_context)` returns the separately defined empty legacy-alias map. These functions do not install metadata, authorize client input or provide a transport endpoint. Explicit destination copying is required to retain the maps after Context close.
`configureRuntime(user_context, filename)` installs generated metadata, checkers and behaviors, the SQLite provider, scoped persistent IDs and the application executor. It does not create schema. `setup(user_context, filename)` performs that configuration and waits for `user_context.ensure_schema().wait()`; its default filename is `:memory:`.
Use `module(user_context)` for metadata-only registration, or `moduleWithCheckers(user_context)`, `moduleWithBehaviors(user_context)` or `moduleWithBehaviorsAndCheckers(user_context)` for explicit assembly. Keep handwritten logic outside generated files.
For custom assembly, call `configureRuntime(user_context, filename)` first, then register application-owned Checker/Behavior overrides with a module and apply it before `user_context.ensure_schema().wait()` or business execution. Calling `configureRuntime` or `setup` again installs defaults and replaces those registrations.
All four module factories register schema bootstrap. Schema initialization prepares the generated Domain Root and active-root binding; reuse that root instead of inserting a second ID 1. Repeated initialization preserves existing root values. Bootstrap errors propagate; do not treat a failed `setup` as a usable initialized Context.
- Extend `PriorityCheckerLogic` and override `checkAndFixPriority(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `PriorityChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `PriorityBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `PlatformCheckerLogic` and override `checkAndFixPlatform(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `PlatformChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `PlatformBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `LabelCheckerLogic` and override `checkAndFixLabel(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `LabelChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `LabelBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
Aggregate composition: `parent.includePendingMutationsFrom(child)` copies the child's pending additions, updates and deletions into the parent's Context for its next audited Save. Set modeled relationships before composition. It returns a checked Result; it does not load a relation, run SQL or save either object. The source retains its pending ledger for retry; original-version conflicts reject composition without partially merging. Cross-domain composition is an explicit destination copy: nested values and snapshots belong to the receiving Context, and closing the source does not invalidate them. It does not rebind the source object or extend either Context lifetime. The runtime owns ledger merging and Context validation.
