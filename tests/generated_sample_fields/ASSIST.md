# Generated C++ model-aware API

Namespace: `cpp_sample_fields`. Create `auto Q = user_context.queries<query_factory>();`. Allocate with UserContext. Execute only after purpose and comment.

Runtime support: [Schema](assist/runtime/schema.md), [Customization](assist/runtime/runtime_custom.md), [Diagnostics](assist/runtime/debug.md), [Tool availability](assist/runtime/tool_api.md).

Standalone registries: `repositoryRegistry(user_context)`, `behaviorRegistry(user_context)` and `checkerRegistry(user_context)` each allocate an independent Context-owned registry using every generated model's canonical name. They do not install themselves into the Context or execute SQL. Runtime registry lookup/replacement APIs retain snake_case; generated defaults are managed objects and checked callback borrows expire with their owner. Same-domain clones share those checked callbacks; cross-domain retention requires separately allocated destination callbacks, not an implicit shared owner.
`user_context.make<LocalSchemaProvider>()` provides the separate data-service schema lookup SPI. `get_entity(canonical_name)` returns a fresh Context-owned descriptor or null for an unknown/case-mismatched name. It always uses the generated static model descriptor, independently of mutable Context metadata and the runtime schema-initialization provider. Ordinary configuration binds the SQL executor to this provider. DataServiceDialect, DataServiceMutationExecutor, DataServiceMutationError and DataServiceIdGenerator expose the corresponding SQLite provider types; DataServiceConnection remains a checked Context-bound connection reference.
Runtime configuration: `ServiceRuntimeConfig::fromEnv(user_context)` reads the model-derived `DATABASE_URL_ENV` without caching it. `databaseUrl()` exposes the retained URL. Configuration and typed error payloads use the supplied Context; destroy value wrappers before Context close or explicitly copy into the destination scope.
`serviceRuntimeFromEnv(user_context)` and `serviceRuntime(user_context, config)` prepare a SQLite URL. These entries and `serviceRuntimeFromConnection(user_context, connection)` return a Result containing a checked executor reference; check the Result before business execution. The connection entry takes a checked `DataServiceConnection` (`Ref<SqliteConnection>`), including an explicitly rebound Global connection. Expired and foreign Context references are rejected; configuration from another domain requires an explicit copy. These entries register metadata, Checker/Behavior, IDs, provider and executor but leave schema initialization explicit. `configureRuntime(user_context, filename)` retains its direct filename and manual typed-key contract; `setup` additionally ensures schema. Typed ServiceRuntimeError preserves MissingEnv, ConnectionError, Configuration, provider and runtime payloads; ordinary C++ binding/allocation exceptions retain their established behavior. URL preparation preserves source slash-pair stripping and query removal; it does not enforce SQLite URI open modes.
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

- Model relation evaluation: `evalSampleRecordList()`; query view handle: `sampleRecordListHandle()`.
- Reverse relation: `sampleRecordList()` returns SmartList<SampleRecord>&; query `selectSampleRecordList()`, `selectSampleRecordListWith(SampleRecordRequest<P>)`, `withSampleRecordList(SampleRecordRequest<P>)`; expression `getSampleRecordList()`.
Field Assist: [platform.sample_record_list](assist/platform/fields/sample_record_list.md).

## SampleRecord
Progressive workflows: [Create](assist/sample_record/create.md), [Query](assist/sample_record/query.md), [Update](assist/sample_record/update.md), [Soft delete](assist/sample_record/delete.md), [List page](assist/sample_record/list_page.md), [Stream](assist/sample_record/stream.md), [E reads](assist/sample_record/expression.md).

Entries: `Q.sampleRecords()` and `Q.sampleRecordsMinimal()`. Evaluation: `E::sampleRecord(entity)`.

Matching and scalar relation statistics: with...Matching/without...Matching, rollUpTo...With and field-statistic ...As methods accept unpurposed generated requests, QuerySelection or SelectQuery values. Matching embeds the bare query but retains complete filter metadata; its descriptor always comes from the declared relationship target. Roll-up combines matching and object grouping. Field statistics materialize the selection before adding the aggregate, preserving the original fresh-selection options boundary. SQLite errors for unsupported statistical functions remain errors. All inputs retain checked Context and explicit destination-copy requirements.
Relation analysis inputs: facetBy...As/AsWithOptions, groupBy...With/WithDetailsFrom, count...With, statsFrom.../As and reverse groupBy...WithDetails accept unpurposed generated requests, QuerySelection and SelectQuery values. Capture preserves complete selection metadata and snapshots without executing a child executor. Requests must share the checked Context binding; all inputs require the destination allocation domain or an explicit destination copy, including User-to-Global capture. Facet children need a nonblank comment at execution. Matching, roll-up and field-statistic methods retain separate conversion contracts.
Relation selection: `select...With(child)` accepts unpurposed requests from the same or another generated domain, or explicit QuerySelection/SelectQuery values. The runtime captures the materialized selection, including its options, without invoking a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Value inputs are kernel borrows and must remain alive through capture. Target-specific request overloads delegate to the same runtime capture and lifetime checks.
Child enhancement: `enhanceChild(child)` accepts an unpurposed request from the same or another generated domain, or explicit QuerySelection/SelectQuery values. It snapshots the complete selection and options without executing a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Ordinary wrapper captures preserve binding checks; explicit core values follow the kernel borrow contract.
Inheritance/query metadata: `matchTypes(types)` accepts Value spans and constrained input ranges, snapshots values into the bound Context and consumes single-pass ranges once. `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.sampleRecordsWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

Entity helpers: `SampleRecord::withId(user_context, id)` constructs a U64 Value in the explicit UserContext domain. `isLoaded(field)` reads scalar/relation availability; `setLoadState(state)` copies into the model's allocation domain and validates its indexed layout before replacement. An incompatible layout throws and preserves the old state; changing availability does not change field values or stage mutations.

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

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<SampleRecord>(value)` or `user_context.decode_json_entities<SampleRecord>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `_orderBy`, `_start`, `_size`, `_pageSize` and `_page`. Read the [value and control rules](#dynamic-json-value-and-control-rules) before composing filters: array shape and first-element type select the operator, and unknown fields add no restriction.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates SampleRecord. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

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
Field Assist: [sample_record.id](assist/sample_record/fields/id.md).

- Model: `platformId()` / `updatePlatformId(const std::uint64_t&)`; changed `changedPlatformId()`; evaluation `evalPlatformId()`; expression `getPlatformId()`.
- Scalar predicates: `withPlatformIs`, `withPlatformIsNot`, `withPlatformGreaterThan`, `withPlatformGreaterThanOrEqualTo`, `withPlatformLessThan`, `withPlatformLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withPlatformBetween(lower, upper)`, `withPlatformIn(std::span<const std::uint64_t>)`, `withPlatformNotIn(std::span<const std::uint64_t>)`, `withPlatformIsNull()`, `withPlatformIsNotNull()`.
- Projection/order/group: `selectPlatformId()`, `orderByPlatformIdAsc()`, `orderByPlatformIdDesc()`, `groupByPlatform()`.
- Group/count: `groupByPlatform()`, `groupByPlatformAs(alias)`, `groupByPlatformWithFunction(alias, function)`, `countPlatform()` and `countPlatformAs(alias)`.
- Forward relation: `platform()` returns const Platform*, `updatePlatform(const Platform&)`; query `selectPlatform()`, `selectPlatformWith(PlatformRequest<P>)`, `withPlatform(PlatformRequest<P>)`; expression `getPlatform()`.
Field Assist: [sample_record.platform](assist/sample_record/fields/platform.md).

- Model: `label()` / `updateLabel(std::string_view)`; changed `changedLabel()`; evaluation `evalLabel()`; expression `getLabel()`.
- Scalar predicates: `withLabelIs`, `withLabelIsNot`, `withLabelGreaterThan`, `withLabelGreaterThanOrEqualTo`, `withLabelLessThan`, `withLabelLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withLabelBetween(lower, upper)`, `withLabelIn(std::span<const String>)`, `withLabelNotIn(std::span<const String>)`, `withLabelIsNull()`, `withLabelIsNotNull()`.
- Projection/order/group: `selectLabel()`, `orderByLabelAsc()`, `orderByLabelDesc()`, `groupByLabel()`.
- Group/count: `groupByLabel()`, `groupByLabelAs(alias)`, `groupByLabelWithFunction(alias, function)`, `countLabel()` and `countLabelAs(alias)`.
- Scalar aggregate: `sumLabel()` uses alias `"sum_label"`; `sumLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgLabel()` uses alias `"avg_label"`; `avgLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minLabel()` uses alias `"min_label"`; `minLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxLabel()` uses alias `"max_label"`; `maxLabelAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withLabelContaining(text)`, `withLabelStartingWith(text)`, `withLabelEndingWith(text)`.
Field Assist: [sample_record.label](assist/sample_record/fields/label.md).

- Model: `quantity()` / `updateQuantity(const std::int64_t&)`; changed `changedQuantity()`; evaluation `evalQuantity()`; expression `getQuantity()`.
- Scalar predicates: `withQuantityIs`, `withQuantityIsNot`, `withQuantityGreaterThan`, `withQuantityGreaterThanOrEqualTo`, `withQuantityLessThan`, `withQuantityLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withQuantityBetween(lower, upper)`, `withQuantityIn(std::span<const std::int64_t>)`, `withQuantityNotIn(std::span<const std::int64_t>)`, `withQuantityIsNull()`, `withQuantityIsNotNull()`.
- Projection/order/group: `selectQuantity()`, `orderByQuantityAsc()`, `orderByQuantityDesc()`, `groupByQuantity()`.
- Group/count: `groupByQuantity()`, `groupByQuantityAs(alias)`, `groupByQuantityWithFunction(alias, function)`, `countQuantity()` and `countQuantityAs(alias)`.
- Scalar aggregate: `sumQuantity()` uses alias `"sum_quantity"`; `sumQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgQuantity()` uses alias `"avg_quantity"`; `avgQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minQuantity()` uses alias `"min_quantity"`; `minQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxQuantity()` uses alias `"max_quantity"`; `maxQuantityAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationQuantity()` and `standardDeviationQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationQuantity()` and `squareRootOfPopulationStandardDeviationQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceQuantity()` and `sampleVarianceQuantityAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceQuantity()` and `samplePopulationVarianceQuantityAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.quantity](assist/sample_record/fields/quantity.md).

- Model: `zeroBound()` / `updateZeroBound(const std::int64_t&)`; changed `changedZeroBound()`; evaluation `evalZeroBound()`; expression `getZeroBound()`.
- Scalar predicates: `withZeroBoundIs`, `withZeroBoundIsNot`, `withZeroBoundGreaterThan`, `withZeroBoundGreaterThanOrEqualTo`, `withZeroBoundLessThan`, `withZeroBoundLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withZeroBoundBetween(lower, upper)`, `withZeroBoundIn(std::span<const std::int64_t>)`, `withZeroBoundNotIn(std::span<const std::int64_t>)`, `withZeroBoundIsNull()`, `withZeroBoundIsNotNull()`.
- Projection/order/group: `selectZeroBound()`, `orderByZeroBoundAsc()`, `orderByZeroBoundDesc()`, `groupByZeroBound()`.
- Group/count: `groupByZeroBound()`, `groupByZeroBoundAs(alias)`, `groupByZeroBoundWithFunction(alias, function)`, `countZeroBound()` and `countZeroBoundAs(alias)`.
- Scalar aggregate: `sumZeroBound()` uses alias `"sum_zero_bound"`; `sumZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgZeroBound()` uses alias `"avg_zero_bound"`; `avgZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minZeroBound()` uses alias `"min_zero_bound"`; `minZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxZeroBound()` uses alias `"max_zero_bound"`; `maxZeroBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationZeroBound()` and `standardDeviationZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationZeroBound()` and `squareRootOfPopulationStandardDeviationZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceZeroBound()` and `sampleVarianceZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceZeroBound()` and `samplePopulationVarianceZeroBoundAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.zero_bound](assist/sample_record/fields/zero_bound.md).

- Model: `negativeMax()` / `updateNegativeMax(const std::int64_t&)`; changed `changedNegativeMax()`; evaluation `evalNegativeMax()`; expression `getNegativeMax()`.
- Scalar predicates: `withNegativeMaxIs`, `withNegativeMaxIsNot`, `withNegativeMaxGreaterThan`, `withNegativeMaxGreaterThanOrEqualTo`, `withNegativeMaxLessThan`, `withNegativeMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withNegativeMaxBetween(lower, upper)`, `withNegativeMaxIn(std::span<const std::int64_t>)`, `withNegativeMaxNotIn(std::span<const std::int64_t>)`, `withNegativeMaxIsNull()`, `withNegativeMaxIsNotNull()`.
- Projection/order/group: `selectNegativeMax()`, `orderByNegativeMaxAsc()`, `orderByNegativeMaxDesc()`, `groupByNegativeMax()`.
- Group/count: `groupByNegativeMax()`, `groupByNegativeMaxAs(alias)`, `groupByNegativeMaxWithFunction(alias, function)`, `countNegativeMax()` and `countNegativeMaxAs(alias)`.
- Scalar aggregate: `sumNegativeMax()` uses alias `"sum_negative_max"`; `sumNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNegativeMax()` uses alias `"avg_negative_max"`; `avgNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNegativeMax()` uses alias `"min_negative_max"`; `minNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNegativeMax()` uses alias `"max_negative_max"`; `maxNegativeMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationNegativeMax()` and `standardDeviationNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationNegativeMax()` and `squareRootOfPopulationStandardDeviationNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceNegativeMax()` and `sampleVarianceNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceNegativeMax()` and `samplePopulationVarianceNegativeMaxAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.negative_max](assist/sample_record/fields/negative_max.md).

- Model: `signedMax()` / `updateSignedMax(const std::int64_t&)`; changed `changedSignedMax()`; evaluation `evalSignedMax()`; expression `getSignedMax()`.
- Scalar predicates: `withSignedMaxIs`, `withSignedMaxIsNot`, `withSignedMaxGreaterThan`, `withSignedMaxGreaterThanOrEqualTo`, `withSignedMaxLessThan`, `withSignedMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withSignedMaxBetween(lower, upper)`, `withSignedMaxIn(std::span<const std::int64_t>)`, `withSignedMaxNotIn(std::span<const std::int64_t>)`, `withSignedMaxIsNull()`, `withSignedMaxIsNotNull()`.
- Projection/order/group: `selectSignedMax()`, `orderBySignedMaxAsc()`, `orderBySignedMaxDesc()`, `groupBySignedMax()`.
- Group/count: `groupBySignedMax()`, `groupBySignedMaxAs(alias)`, `groupBySignedMaxWithFunction(alias, function)`, `countSignedMax()` and `countSignedMaxAs(alias)`.
- Scalar aggregate: `sumSignedMax()` uses alias `"sum_signed_max"`; `sumSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgSignedMax()` uses alias `"avg_signed_max"`; `avgSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minSignedMax()` uses alias `"min_signed_max"`; `minSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxSignedMax()` uses alias `"max_signed_max"`; `maxSignedMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationSignedMax()` and `standardDeviationSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationSignedMax()` and `squareRootOfPopulationStandardDeviationSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceSignedMax()` and `sampleVarianceSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceSignedMax()` and `samplePopulationVarianceSignedMaxAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.signed_max](assist/sample_record/fields/signed_max.md).

- Model: `wideMax()` / `updateWideMax(const std::int64_t&)`; changed `changedWideMax()`; evaluation `evalWideMax()`; expression `getWideMax()`.
- Scalar predicates: `withWideMaxIs`, `withWideMaxIsNot`, `withWideMaxGreaterThan`, `withWideMaxGreaterThanOrEqualTo`, `withWideMaxLessThan`, `withWideMaxLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withWideMaxBetween(lower, upper)`, `withWideMaxIn(std::span<const std::int64_t>)`, `withWideMaxNotIn(std::span<const std::int64_t>)`, `withWideMaxIsNull()`, `withWideMaxIsNotNull()`.
- Projection/order/group: `selectWideMax()`, `orderByWideMaxAsc()`, `orderByWideMaxDesc()`, `groupByWideMax()`.
- Group/count: `groupByWideMax()`, `groupByWideMaxAs(alias)`, `groupByWideMaxWithFunction(alias, function)`, `countWideMax()` and `countWideMaxAs(alias)`.
- Scalar aggregate: `sumWideMax()` uses alias `"sum_wide_max"`; `sumWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgWideMax()` uses alias `"avg_wide_max"`; `avgWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minWideMax()` uses alias `"min_wide_max"`; `minWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxWideMax()` uses alias `"max_wide_max"`; `maxWideMaxAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationWideMax()` and `standardDeviationWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationWideMax()` and `squareRootOfPopulationStandardDeviationWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceWideMax()` and `sampleVarianceWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceWideMax()` and `samplePopulationVarianceWideMaxAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.wide_max](assist/sample_record/fields/wide_max.md).

- Model: `negativeBound()` / `updateNegativeBound(std::string_view)`; changed `changedNegativeBound()`; evaluation `evalNegativeBound()`; expression `getNegativeBound()`.
- Scalar predicates: `withNegativeBoundIs`, `withNegativeBoundIsNot`, `withNegativeBoundGreaterThan`, `withNegativeBoundGreaterThanOrEqualTo`, `withNegativeBoundLessThan`, `withNegativeBoundLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNegativeBoundBetween(lower, upper)`, `withNegativeBoundIn(std::span<const String>)`, `withNegativeBoundNotIn(std::span<const String>)`, `withNegativeBoundIsNull()`, `withNegativeBoundIsNotNull()`.
- Projection/order/group: `selectNegativeBound()`, `orderByNegativeBoundAsc()`, `orderByNegativeBoundDesc()`, `groupByNegativeBound()`.
- Group/count: `groupByNegativeBound()`, `groupByNegativeBoundAs(alias)`, `groupByNegativeBoundWithFunction(alias, function)`, `countNegativeBound()` and `countNegativeBoundAs(alias)`.
- Scalar aggregate: `sumNegativeBound()` uses alias `"sum_negative_bound"`; `sumNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgNegativeBound()` uses alias `"avg_negative_bound"`; `avgNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minNegativeBound()` uses alias `"min_negative_bound"`; `minNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxNegativeBound()` uses alias `"max_negative_bound"`; `maxNegativeBoundAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNegativeBoundContaining(text)`, `withNegativeBoundStartingWith(text)`, `withNegativeBoundEndingWith(text)`.
Field Assist: [sample_record.negative_bound](assist/sample_record/fields/negative_bound.md).

- Model: `amount()` / `updateAmount(const Decimal&)`; changed `changedAmount()`; evaluation `evalAmount()`; expression `getAmount()`.
- Scalar predicates: `withAmountIs`, `withAmountIsNot`, `withAmountGreaterThan`, `withAmountGreaterThanOrEqualTo`, `withAmountLessThan`, `withAmountLessThanOrEqualTo`. Arguments: `const Decimal&`.
- Range/set/null predicates: `withAmountBetween(lower, upper)`, `withAmountIn(std::span<const Decimal>)`, `withAmountNotIn(std::span<const Decimal>)`, `withAmountIsNull()`, `withAmountIsNotNull()`.
- Projection/order/group: `selectAmount()`, `orderByAmountAsc()`, `orderByAmountDesc()`, `groupByAmount()`.
- Group/count: `groupByAmount()`, `groupByAmountAs(alias)`, `groupByAmountWithFunction(alias, function)`, `countAmount()` and `countAmountAs(alias)`.
- Scalar aggregate: `sumAmount()` uses alias `"sum_amount"`; `sumAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgAmount()` uses alias `"avg_amount"`; `avgAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minAmount()` uses alias `"min_amount"`; `minAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxAmount()` uses alias `"max_amount"`; `maxAmountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationAmount()` and `standardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationAmount()` and `squareRootOfPopulationStandardDeviationAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceAmount()` and `sampleVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceAmount()` and `samplePopulationVarianceAmountAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.amount](assist/sample_record/fields/amount.md).

- Model: `height()` / `updateHeight(const double&)`; changed `changedHeight()`; evaluation `evalHeight()`; expression `getHeight()`.
- Scalar predicates: `withHeightIs`, `withHeightIsNot`, `withHeightGreaterThan`, `withHeightGreaterThanOrEqualTo`, `withHeightLessThan`, `withHeightLessThanOrEqualTo`. Arguments: `const double&`.
- Range/set/null predicates: `withHeightBetween(lower, upper)`, `withHeightIn(std::span<const double>)`, `withHeightNotIn(std::span<const double>)`, `withHeightIsNull()`, `withHeightIsNotNull()`.
- Projection/order/group: `selectHeight()`, `orderByHeightAsc()`, `orderByHeightDesc()`, `groupByHeight()`.
- Group/count: `groupByHeight()`, `groupByHeightAs(alias)`, `groupByHeightWithFunction(alias, function)`, `countHeight()` and `countHeightAs(alias)`.
- Scalar aggregate: `sumHeight()` uses alias `"sum_height"`; `sumHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgHeight()` uses alias `"avg_height"`; `avgHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minHeight()` uses alias `"min_height"`; `minHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxHeight()` uses alias `"max_height"`; `maxHeightAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationHeight()` and `standardDeviationHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationHeight()` and `squareRootOfPopulationStandardDeviationHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceHeight()` and `sampleVarianceHeightAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceHeight()` and `samplePopulationVarianceHeightAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.height](assist/sample_record/fields/height.md).

- Model: `optionalCount()` / `updateOptionalCount(const std::optional<std::int64_t>&)`; changed `changedOptionalCount()`; evaluation `evalOptionalCount()`; expression `getOptionalCount()`.
- Scalar predicates: `withOptionalCountIs`, `withOptionalCountIsNot`, `withOptionalCountGreaterThan`, `withOptionalCountGreaterThanOrEqualTo`, `withOptionalCountLessThan`, `withOptionalCountLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withOptionalCountBetween(lower, upper)`, `withOptionalCountIn(std::span<const std::int64_t>)`, `withOptionalCountNotIn(std::span<const std::int64_t>)`, `withOptionalCountIsNull()`, `withOptionalCountIsNotNull()`.
- Projection/order/group: `selectOptionalCount()`, `orderByOptionalCountAsc()`, `orderByOptionalCountDesc()`, `groupByOptionalCount()`.
- Group/count: `groupByOptionalCount()`, `groupByOptionalCountAs(alias)`, `groupByOptionalCountWithFunction(alias, function)`, `countOptionalCount()` and `countOptionalCountAs(alias)`.
- Scalar aggregate: `sumOptionalCount()` uses alias `"sum_optional_count"`; `sumOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalCount()` uses alias `"avg_optional_count"`; `avgOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalCount()` uses alias `"min_optional_count"`; `minOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalCount()` uses alias `"max_optional_count"`; `maxOptionalCountAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Statistical aggregate: `standardDeviationOptionalCount()` and `standardDeviationOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `squareRootOfPopulationStandardDeviationOptionalCount()` and `squareRootOfPopulationStandardDeviationOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `sampleVarianceOptionalCount()` and `sampleVarianceOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
- Statistical aggregate: `samplePopulationVarianceOptionalCount()` and `samplePopulationVarianceOptionalCountAs(alias)`; SQLite requires the corresponding SQL function.
Field Assist: [sample_record.optional_count](assist/sample_record/fields/optional_count.md).

- Model: `active()` / `updateActive(const bool&)`; changed `changedActive()`; evaluation `evalActive()`; expression `getActive()`.
- Scalar predicates: `withActiveIs`, `withActiveIsNot`, `withActiveGreaterThan`, `withActiveGreaterThanOrEqualTo`, `withActiveLessThan`, `withActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withActiveBetween(lower, upper)`, `withActiveIn(std::span<const bool>)`, `withActiveNotIn(std::span<const bool>)`, `withActiveIsNull()`, `withActiveIsNotNull()`.
- Projection/order/group: `selectActive()`, `orderByActiveAsc()`, `orderByActiveDesc()`, `groupByActive()`.
- Group/count: `groupByActive()`, `groupByActiveAs(alias)`, `groupByActiveWithFunction(alias, function)`, `countActive()` and `countActiveAs(alias)`.
- Scalar aggregate: `sumActive()` uses alias `"sum_active"`; `sumActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgActive()` uses alias `"avg_active"`; `avgActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minActive()` uses alias `"min_active"`; `minActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxActive()` uses alias `"max_active"`; `maxActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreActive()`, `whichAreNotActive()`.
Field Assist: [sample_record.active](assist/sample_record/fields/active.md).

- Model: `disabled()` / `updateDisabled(const bool&)`; changed `changedDisabled()`; evaluation `evalDisabled()`; expression `getDisabled()`.
- Scalar predicates: `withDisabledIs`, `withDisabledIsNot`, `withDisabledGreaterThan`, `withDisabledGreaterThanOrEqualTo`, `withDisabledLessThan`, `withDisabledLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withDisabledBetween(lower, upper)`, `withDisabledIn(std::span<const bool>)`, `withDisabledNotIn(std::span<const bool>)`, `withDisabledIsNull()`, `withDisabledIsNotNull()`.
- Projection/order/group: `selectDisabled()`, `orderByDisabledAsc()`, `orderByDisabledDesc()`, `groupByDisabled()`.
- Group/count: `groupByDisabled()`, `groupByDisabledAs(alias)`, `groupByDisabledWithFunction(alias, function)`, `countDisabled()` and `countDisabledAs(alias)`.
- Scalar aggregate: `sumDisabled()` uses alias `"sum_disabled"`; `sumDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDisabled()` uses alias `"avg_disabled"`; `avgDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDisabled()` uses alias `"min_disabled"`; `minDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDisabled()` uses alias `"max_disabled"`; `maxDisabledAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreDisabled()`, `whichAreNotDisabled()`.
Field Assist: [sample_record.disabled](assist/sample_record/fields/disabled.md).

- Model: `optionalActive()` / `updateOptionalActive(const std::optional<bool>&)`; changed `changedOptionalActive()`; evaluation `evalOptionalActive()`; expression `getOptionalActive()`.
- Scalar predicates: `withOptionalActiveIs`, `withOptionalActiveIsNot`, `withOptionalActiveGreaterThan`, `withOptionalActiveGreaterThanOrEqualTo`, `withOptionalActiveLessThan`, `withOptionalActiveLessThanOrEqualTo`. Arguments: `const bool&`.
- Range/set/null predicates: `withOptionalActiveBetween(lower, upper)`, `withOptionalActiveIn(std::span<const bool>)`, `withOptionalActiveNotIn(std::span<const bool>)`, `withOptionalActiveIsNull()`, `withOptionalActiveIsNotNull()`.
- Projection/order/group: `selectOptionalActive()`, `orderByOptionalActiveAsc()`, `orderByOptionalActiveDesc()`, `groupByOptionalActive()`.
- Group/count: `groupByOptionalActive()`, `groupByOptionalActiveAs(alias)`, `groupByOptionalActiveWithFunction(alias, function)`, `countOptionalActive()` and `countOptionalActiveAs(alias)`.
- Scalar aggregate: `sumOptionalActive()` uses alias `"sum_optional_active"`; `sumOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalActive()` uses alias `"avg_optional_active"`; `avgOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalActive()` uses alias `"min_optional_active"`; `minOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalActive()` uses alias `"max_optional_active"`; `maxOptionalActiveAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Boolean predicates: `whichAreOptionalActive()`, `whichAreNotOptionalActive()`.
Field Assist: [sample_record.optional_active](assist/sample_record/fields/optional_active.md).

- Model: `day()` / `updateDay(const Date&)`; changed `changedDay()`; evaluation `evalDay()`; expression `getDay()`.
- Scalar predicates: `withDayIs`, `withDayIsNot`, `withDayGreaterThan`, `withDayGreaterThanOrEqualTo`, `withDayLessThan`, `withDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withDayBetween(lower, upper)`, `withDayIn(std::span<const Date>)`, `withDayNotIn(std::span<const Date>)`, `withDayIsNull()`, `withDayIsNotNull()`.
- Projection/order/group: `selectDay()`, `orderByDayAsc()`, `orderByDayDesc()`, `groupByDay()`.
- Group/count: `groupByDay()`, `groupByDayAs(alias)`, `groupByDayWithFunction(alias, function)`, `countDay()` and `countDayAs(alias)`.
- Scalar aggregate: `sumDay()` uses alias `"sum_day"`; `sumDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgDay()` uses alias `"avg_day"`; `avgDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minDay()` uses alias `"min_day"`; `minDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxDay()` uses alias `"max_day"`; `maxDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [sample_record.day](assist/sample_record/fields/day.md).

- Model: `optionalDay()` / `updateOptionalDay(const std::optional<Date>&)`; changed `changedOptionalDay()`; evaluation `evalOptionalDay()`; expression `getOptionalDay()`.
- Scalar predicates: `withOptionalDayIs`, `withOptionalDayIsNot`, `withOptionalDayGreaterThan`, `withOptionalDayGreaterThanOrEqualTo`, `withOptionalDayLessThan`, `withOptionalDayLessThanOrEqualTo`. Arguments: `const Date&`.
- Range/set/null predicates: `withOptionalDayBetween(lower, upper)`, `withOptionalDayIn(std::span<const Date>)`, `withOptionalDayNotIn(std::span<const Date>)`, `withOptionalDayIsNull()`, `withOptionalDayIsNotNull()`.
- Projection/order/group: `selectOptionalDay()`, `orderByOptionalDayAsc()`, `orderByOptionalDayDesc()`, `groupByOptionalDay()`.
- Group/count: `groupByOptionalDay()`, `groupByOptionalDayAs(alias)`, `groupByOptionalDayWithFunction(alias, function)`, `countOptionalDay()` and `countOptionalDayAs(alias)`.
- Scalar aggregate: `sumOptionalDay()` uses alias `"sum_optional_day"`; `sumOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalDay()` uses alias `"avg_optional_day"`; `avgOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalDay()` uses alias `"min_optional_day"`; `minOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalDay()` uses alias `"max_optional_day"`; `maxOptionalDayAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [sample_record.optional_day](assist/sample_record/fields/optional_day.md).

- Model: `recordedAt()` / `updateRecordedAt(const Timestamp&)`; changed `changedRecordedAt()`; evaluation `evalRecordedAt()`; expression `getRecordedAt()`.
- Scalar predicates: `withRecordedAtIs`, `withRecordedAtIsNot`, `withRecordedAtGreaterThan`, `withRecordedAtGreaterThanOrEqualTo`, `withRecordedAtLessThan`, `withRecordedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withRecordedAtBetween(lower, upper)`, `withRecordedAtIn(std::span<const Timestamp>)`, `withRecordedAtNotIn(std::span<const Timestamp>)`, `withRecordedAtIsNull()`, `withRecordedAtIsNotNull()`.
- Projection/order/group: `selectRecordedAt()`, `orderByRecordedAtAsc()`, `orderByRecordedAtDesc()`, `groupByRecordedAt()`.
- Group/count: `groupByRecordedAt()`, `groupByRecordedAtAs(alias)`, `groupByRecordedAtWithFunction(alias, function)`, `countRecordedAt()` and `countRecordedAtAs(alias)`.
- Scalar aggregate: `sumRecordedAt()` uses alias `"sum_recorded_at"`; `sumRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgRecordedAt()` uses alias `"avg_recorded_at"`; `avgRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minRecordedAt()` uses alias `"min_recorded_at"`; `minRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxRecordedAt()` uses alias `"max_recorded_at"`; `maxRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [sample_record.recorded_at](assist/sample_record/fields/recorded_at.md).

- Model: `optionalRecordedAt()` / `updateOptionalRecordedAt(const std::optional<Timestamp>&)`; changed `changedOptionalRecordedAt()`; evaluation `evalOptionalRecordedAt()`; expression `getOptionalRecordedAt()`.
- Scalar predicates: `withOptionalRecordedAtIs`, `withOptionalRecordedAtIsNot`, `withOptionalRecordedAtGreaterThan`, `withOptionalRecordedAtGreaterThanOrEqualTo`, `withOptionalRecordedAtLessThan`, `withOptionalRecordedAtLessThanOrEqualTo`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `withOptionalRecordedAtBetween(lower, upper)`, `withOptionalRecordedAtIn(std::span<const Timestamp>)`, `withOptionalRecordedAtNotIn(std::span<const Timestamp>)`, `withOptionalRecordedAtIsNull()`, `withOptionalRecordedAtIsNotNull()`.
- Projection/order/group: `selectOptionalRecordedAt()`, `orderByOptionalRecordedAtAsc()`, `orderByOptionalRecordedAtDesc()`, `groupByOptionalRecordedAt()`.
- Group/count: `groupByOptionalRecordedAt()`, `groupByOptionalRecordedAtAs(alias)`, `groupByOptionalRecordedAtWithFunction(alias, function)`, `countOptionalRecordedAt()` and `countOptionalRecordedAtAs(alias)`.
- Scalar aggregate: `sumOptionalRecordedAt()` uses alias `"sum_optional_recorded_at"`; `sumOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalRecordedAt()` uses alias `"avg_optional_recorded_at"`; `avgOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalRecordedAt()` uses alias `"min_optional_recorded_at"`; `minOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalRecordedAt()` uses alias `"max_optional_recorded_at"`; `maxOptionalRecordedAtAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [sample_record.optional_recorded_at](assist/sample_record/fields/optional_recorded_at.md).

- Model: `clockText()` / `updateClockText(std::string_view)`; changed `changedClockText()`; evaluation `evalClockText()`; expression `getClockText()`.
- Scalar predicates: `withClockTextIs`, `withClockTextIsNot`, `withClockTextGreaterThan`, `withClockTextGreaterThanOrEqualTo`, `withClockTextLessThan`, `withClockTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withClockTextBetween(lower, upper)`, `withClockTextIn(std::span<const String>)`, `withClockTextNotIn(std::span<const String>)`, `withClockTextIsNull()`, `withClockTextIsNotNull()`.
- Projection/order/group: `selectClockText()`, `orderByClockTextAsc()`, `orderByClockTextDesc()`, `groupByClockText()`.
- Group/count: `groupByClockText()`, `groupByClockTextAs(alias)`, `groupByClockTextWithFunction(alias, function)`, `countClockText()` and `countClockTextAs(alias)`.
- Scalar aggregate: `sumClockText()` uses alias `"sum_clock_text"`; `sumClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgClockText()` uses alias `"avg_clock_text"`; `avgClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minClockText()` uses alias `"min_clock_text"`; `minClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxClockText()` uses alias `"max_clock_text"`; `maxClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withClockTextContaining(text)`, `withClockTextStartingWith(text)`, `withClockTextEndingWith(text)`.
Field Assist: [sample_record.clock_text](assist/sample_record/fields/clock_text.md).

- Model: `optionalClockText()` / `updateOptionalClockText(const std::optional<String>&)`; changed `changedOptionalClockText()`; evaluation `evalOptionalClockText()`; expression `getOptionalClockText()`.
- Scalar predicates: `withOptionalClockTextIs`, `withOptionalClockTextIsNot`, `withOptionalClockTextGreaterThan`, `withOptionalClockTextGreaterThanOrEqualTo`, `withOptionalClockTextLessThan`, `withOptionalClockTextLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withOptionalClockTextBetween(lower, upper)`, `withOptionalClockTextIn(std::span<const String>)`, `withOptionalClockTextNotIn(std::span<const String>)`, `withOptionalClockTextIsNull()`, `withOptionalClockTextIsNotNull()`.
- Projection/order/group: `selectOptionalClockText()`, `orderByOptionalClockTextAsc()`, `orderByOptionalClockTextDesc()`, `groupByOptionalClockText()`.
- Group/count: `groupByOptionalClockText()`, `groupByOptionalClockTextAs(alias)`, `groupByOptionalClockTextWithFunction(alias, function)`, `countOptionalClockText()` and `countOptionalClockTextAs(alias)`.
- Scalar aggregate: `sumOptionalClockText()` uses alias `"sum_optional_clock_text"`; `sumOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgOptionalClockText()` uses alias `"avg_optional_clock_text"`; `avgOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minOptionalClockText()` uses alias `"min_optional_clock_text"`; `minOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxOptionalClockText()` uses alias `"max_optional_clock_text"`; `maxOptionalClockTextAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withOptionalClockTextContaining(text)`, `withOptionalClockTextStartingWith(text)`, `withOptionalClockTextEndingWith(text)`.
Field Assist: [sample_record.optional_clock_text](assist/sample_record/fields/optional_clock_text.md).

- Model: `ratio()` / `updateRatio(const std::optional<float>&)`; changed `changedRatio()`; evaluation `evalRatio()`; expression `getRatio()`.
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
Field Assist: [sample_record.ratio](assist/sample_record/fields/ratio.md).

- Model: `payload()` / `updatePayload(std::string_view)`; changed `changedPayload()`; evaluation `evalPayload()`; expression `getPayload()`.
- Scalar predicates: `withPayloadIs`, `withPayloadIsNot`, `withPayloadGreaterThan`, `withPayloadGreaterThanOrEqualTo`, `withPayloadLessThan`, `withPayloadLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withPayloadBetween(lower, upper)`, `withPayloadIn(std::span<const String>)`, `withPayloadNotIn(std::span<const String>)`, `withPayloadIsNull()`, `withPayloadIsNotNull()`.
- Projection/order/group: `selectPayload()`, `orderByPayloadAsc()`, `orderByPayloadDesc()`, `groupByPayload()`.
- Group/count: `groupByPayload()`, `groupByPayloadAs(alias)`, `groupByPayloadWithFunction(alias, function)`, `countPayload()` and `countPayloadAs(alias)`.
- Scalar aggregate: `sumPayload()` uses alias `"sum_payload"`; `sumPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgPayload()` uses alias `"avg_payload"`; `avgPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minPayload()` uses alias `"min_payload"`; `minPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxPayload()` uses alias `"max_payload"`; `maxPayloadAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withPayloadContaining(text)`, `withPayloadStartingWith(text)`, `withPayloadEndingWith(text)`.
Field Assist: [sample_record.payload](assist/sample_record/fields/payload.md).

- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `"sum_version"`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `"avg_version"`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `"min_version"`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `"max_version"`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [sample_record.version](assist/sample_record/fields/version.md).

- Model relation evaluation: `evalPlatform()` returns a checked entity borrow or Null/NotLoaded.
- Model relation evaluation: `evalFirstLinks()`; query view handle: `firstLinksHandle()`.
- Reverse relation: `firstLinks()` returns SmartList<SampleLink>&; query `selectFirstLinks()`, `selectFirstLinksWith(SampleLinkRequest<P>)`, `withFirstLinks(SampleLinkRequest<P>)`; expression `getFirstLinks()`.
Field Assist: [sample_record.first_links](assist/sample_record/fields/first_links.md).

- Model relation evaluation: `evalSecondLinks()`; query view handle: `secondLinksHandle()`.
- Reverse relation: `secondLinks()` returns SmartList<SampleLink>&; query `selectSecondLinks()`, `selectSecondLinksWith(SampleLinkRequest<P>)`, `withSecondLinks(SampleLinkRequest<P>)`; expression `getSecondLinks()`.
Field Assist: [sample_record.second_links](assist/sample_record/fields/second_links.md).

## SampleLink
Progressive workflows: [Create](assist/sample_link/create.md), [Query](assist/sample_link/query.md), [Update](assist/sample_link/update.md), [Soft delete](assist/sample_link/delete.md), [List page](assist/sample_link/list_page.md), [Stream](assist/sample_link/stream.md), [E reads](assist/sample_link/expression.md).

Entries: `Q.sampleLinks()` and `Q.sampleLinksMinimal()`. Evaluation: `E::sampleLink(entity)`.

Matching and scalar relation statistics: with...Matching/without...Matching, rollUpTo...With and field-statistic ...As methods accept unpurposed generated requests, QuerySelection or SelectQuery values. Matching embeds the bare query but retains complete filter metadata; its descriptor always comes from the declared relationship target. Roll-up combines matching and object grouping. Field statistics materialize the selection before adding the aggregate, preserving the original fresh-selection options boundary. SQLite errors for unsupported statistical functions remain errors. All inputs retain checked Context and explicit destination-copy requirements.
Relation analysis inputs: facetBy...As/AsWithOptions, groupBy...With/WithDetailsFrom, count...With, statsFrom.../As and reverse groupBy...WithDetails accept unpurposed generated requests, QuerySelection and SelectQuery values. Capture preserves complete selection metadata and snapshots without executing a child executor. Requests must share the checked Context binding; all inputs require the destination allocation domain or an explicit destination copy, including User-to-Global capture. Facet children need a nonblank comment at execution. Matching, roll-up and field-statistic methods retain separate conversion contracts.
Relation selection: `select...With(child)` accepts unpurposed requests from the same or another generated domain, or explicit QuerySelection/SelectQuery values. The runtime captures the materialized selection, including its options, without invoking a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Value inputs are kernel borrows and must remain alive through capture. Target-specific request overloads delegate to the same runtime capture and lifetime checks.
Child enhancement: `enhanceChild(child)` accepts an unpurposed request from the same or another generated domain, or explicit QuerySelection/SelectQuery values. It snapshots the complete selection and options without executing a child executor. Requests must share the checked Context binding; foreign values require an explicit destination copy. Ordinary wrapper captures preserve binding checks; explicit core values follow the kernel borrow contract.
Inheritance/query metadata: `matchTypes(types)` accepts Value spans and constrained input ranges, snapshots values into the bound Context and consumes single-pass ranges once. `withTypeGroup()`, `enhanceChild(request)`, `enhanceChildrenIfNeeded()` and `Q.sampleLinksWithChildren()`. The with-children entry clears slicing, includes self fields and descendants, and does not apply the default live-version filter.

Entity helpers: `SampleLink::withId(user_context, id)` constructs a U64 Value in the explicit UserContext domain. `isLoaded(field)` reads scalar/relation availability; `setLoadState(state)` copies into the model's allocation domain and validates its indexed layout before replacement. An incompatible layout throws and preserves the old state; changing availability does not change field values or stage mutations.

General query: `withDeletedRows()`, `deletedRowsOnly()`, `appendSearchCriteria(expr)` and `filterProperty(left, FieldOperator, right)` preserve the source filter rules. `pageOffset(offset, size)`, `pageNumber(number, size)`, `pageDefault(number)`, `offsetOnly(offset)` and `top(size)` are available before/after purpose. Field `unselect...` methods preserve independent aliases; raw SQL methods follow the source explicit trusted-segment API and require application-controlled SQL.

Generic scalar criteria: the field's predicate prefix accepts `(FieldOperator, inputs)` where inputs is a range of values convertible to core Value. `create...Criteria(op, inputs)` is a const method on a bound request and returns `Ref<Expr>` in its allocation scope; append it with `appendSearchCriteria(criteria)` without another Context argument. Foreign UserContext IDs are rejected; use an explicit destination copy. Before/After and BetweenRange helpers retain source comparison and inclusive range semantics. These helpers are not generated for version, Boolean, object-reference or Business-ID fields where the original template omits them.

Scalar membership: source-supported `...In(inputs)` and `...NotIn(inputs)` accept input ranges whose elements convert to core Value, including arrays, vectors, spans and single-pass ranges. Values are copied into the bound request's allocator. Existing typed span overloads remain. The generic range overloads are omitted for Boolean, version, object-reference and Business-ID fields; Business-ID methods keep their distinct typed values. Neither route adds another Context argument.
Scalar comparisons: ordinary scalar Is/IsNot and non-ID ordered/Between methods also accept values constructible as core Value. Between can use independently typed lower/upper operands. Existing field-typed overloads remain, and the new overloads copy payloads into the bound allocator. Boolean/version/object-reference/Business-ID fields retain their separate contracts; unsupported values fail in constraints.
Text predicates: source-selected sound-like methods accept Value-constructible operands as well as text. Contains/start/end variants keep their text-only inputs. Generic sound-like overloads are omitted for large-text/password/Business-ID fields. Optional values are not implicitly converted to Value; use an explicit supported Value NULL or TypedNull when the intended criterion needs NULL. Numeric select...WithFunction helpers construct aggregate projections, and unselect removes the plain/raw field projection while preserving independent aliases and aggregates. SQLite rejects unsupported aggregate functions. The current default GBK compiler emits its argument unchanged, so the GBK marker does not establish GBK collation.
Request state before purpose: `coreQuery()` returns an independent bare core snapshot corresponding to the original query(&self). The existing `query()` keeps the integrated C++ runtime representation. Destroy the snapshot before its allocator closes or explicitly copy it into a destination scope. `relationSelections()`, `relationFilters()` and `childEnhancements()` return read-only Context borrows. `queryOptions()` returns an independent allocator-aware snapshot; destroy it before its owning Context closes or copy it explicitly into a destination scope. `std::move(request).intoQuery()` consumes the request and returns only its core SelectQuery, omitting option metadata and child enhancements as in the original direct into_query method. `std::move(request.selection()).into_query()` is a separate core conversion that applies options and enhancements. Neither conversion grants execution authority or extends Context lifetime. A consumed request rejects later access.
Relationship analysis: forward `groupBy...With(child)`, `groupBy...WithDetails()`, `groupBy...WithDetailsFrom(child)` and `rollUpTo...With(child)` use matching object queries. Forward/reverse `have...`, `haveNo...`, `with...Matching(child)` and `without...Matching(child)` preserve source membership semantics. Reverse collection `count...`, `count...As(alias)`, `count...With(alias, child)`, `statsFrom...As(alias, child)` and centrally named field aggregates attach results during executeForList/One/Page; executeForRows retains the source raw compact-row behavior.

Facet: `facetByFirstRecordAs(name, child)` defaults to all facets; `facetByFirstRecordAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Facet: `facetBySecondRecordAs(name, child)` defaults to all facets; `facetBySecondRecordAsWithOptions(name, child, includeAll)` controls outer-query restriction. Matching child relation aggregates receive the outer predicate. Facets attach to list/page results; one executes them before returning the first entity. Rows/count/exists/stream do not execute them. Child facet metadata does not imply recursive generated facet execution.
Aggregation: `groupBy(field)`, `count()`, `countAs(alias)`, `aggregateCountField(field, alias)`, `aggregateWithFunction(field, alias, function)` and `aggregateSum/Avg/Min/Max/Stddev/StddevPop/VarSamp/VarPop/BitAnd/BitOr/BitXor(field, alias)` build source-shaped aggregate queries. `enableAggregationCache()`, `enableAggregationCacheFor(duration)` and `propagateAggregationCache(duration)` retain cache metadata. Read compact aggregate rows with `executeForRows(user_context)`; SQLite may reject statistical and bitwise functions, matching the source backend.

Execution: `executeForCount(user_context)` returns an unsliced count; `executeForExists(user_context)` retains the query offset and limits to one compact row; `executeForFirst(user_context)` has the same first-or-empty contract as `executeForOne`. `executeForPage(user_context, offset, limit)` returns a typed list with total count. These methods require purpose and nonblank comment; only UserContext supplies runtime dependencies.

Typed JSON presentation: after `module(user_context).apply_to(user_context)`, decode with `user_context.decode_json_entity<SampleLink>(value)` or `user_context.decode_json_entities<SampleLink>(array)`. Both return Result and delegate to the shared installed-metadata runtime. Read back with `std::move(entity).into_json(user_context.memory_resource())`; nested loaded views preserve omitted fields, NULL and empty lists. Input cannot authorize a save or replace authoritative database loading. Persistent # fields and incoming runtime state are rejected.

Dynamic JSON: `filterWithJson(text)` applies model-aware self/foreign-key filters, dotted forward/reverse relation membership, `_orderBy`, `_start`, `_size`, `_pageSize` and `_page`. Read the [value and control rules](#dynamic-json-value-and-control-rules) before composing filters: array shape and first-element type select the operator, and unknown fields add no restriction.

Result adaptation: `returnType<Model>()` changes decoding and keeps the source query identity, projections and Context. `newEntity(user_context)` still creates SampleLink. Result fields require appropriate projections/enhancements; adaptation does not load fields or issue SQL.

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
Field Assist: [sample_link.id](assist/sample_link/fields/id.md).

- Model: `firstRecordId()` / `updateFirstRecordId(const std::uint64_t&)`; changed `changedFirstRecordId()`; evaluation `evalFirstRecordId()`; expression `getFirstRecordId()`.
- Scalar predicates: `withFirstRecordIs`, `withFirstRecordIsNot`, `withFirstRecordGreaterThan`, `withFirstRecordGreaterThanOrEqualTo`, `withFirstRecordLessThan`, `withFirstRecordLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withFirstRecordBetween(lower, upper)`, `withFirstRecordIn(std::span<const std::uint64_t>)`, `withFirstRecordNotIn(std::span<const std::uint64_t>)`, `withFirstRecordIsNull()`, `withFirstRecordIsNotNull()`.
- Projection/order/group: `selectFirstRecordId()`, `orderByFirstRecordIdAsc()`, `orderByFirstRecordIdDesc()`, `groupByFirstRecord()`.
- Group/count: `groupByFirstRecord()`, `groupByFirstRecordAs(alias)`, `groupByFirstRecordWithFunction(alias, function)`, `countFirstRecord()` and `countFirstRecordAs(alias)`.
- Forward relation: `firstRecord()` returns const SampleRecord*, `updateFirstRecord(const SampleRecord&)`; query `selectFirstRecord()`, `selectFirstRecordWith(SampleRecordRequest<P>)`, `withFirstRecord(SampleRecordRequest<P>)`; expression `getFirstRecord()`.
Field Assist: [sample_link.first_record](assist/sample_link/fields/first_record.md).

- Model: `secondRecordId()` / `updateSecondRecordId(const std::uint64_t&)`; changed `changedSecondRecordId()`; evaluation `evalSecondRecordId()`; expression `getSecondRecordId()`.
- Scalar predicates: `withSecondRecordIs`, `withSecondRecordIsNot`, `withSecondRecordGreaterThan`, `withSecondRecordGreaterThanOrEqualTo`, `withSecondRecordLessThan`, `withSecondRecordLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withSecondRecordBetween(lower, upper)`, `withSecondRecordIn(std::span<const std::uint64_t>)`, `withSecondRecordNotIn(std::span<const std::uint64_t>)`, `withSecondRecordIsNull()`, `withSecondRecordIsNotNull()`.
- Projection/order/group: `selectSecondRecordId()`, `orderBySecondRecordIdAsc()`, `orderBySecondRecordIdDesc()`, `groupBySecondRecord()`.
- Group/count: `groupBySecondRecord()`, `groupBySecondRecordAs(alias)`, `groupBySecondRecordWithFunction(alias, function)`, `countSecondRecord()` and `countSecondRecordAs(alias)`.
- Forward relation: `secondRecord()` returns const SampleRecord*, `updateSecondRecord(const SampleRecord&)`; query `selectSecondRecord()`, `selectSecondRecordWith(SampleRecordRequest<P>)`, `withSecondRecord(SampleRecordRequest<P>)`; expression `getSecondRecord()`.
Field Assist: [sample_link.second_record](assist/sample_link/fields/second_record.md).

- Model: `name()` / `updateName(std::string_view)`; changed `changedName()`; evaluation `evalName()`; expression `getName()`.
- Scalar predicates: `withNameIs`, `withNameIsNot`, `withNameGreaterThan`, `withNameGreaterThanOrEqualTo`, `withNameLessThan`, `withNameLessThanOrEqualTo`. Arguments: `std::string_view`.
- Range/set/null predicates: `withNameBetween(lower, upper)`, `withNameIn(std::span<const String>)`, `withNameNotIn(std::span<const String>)`, `withNameIsNull()`, `withNameIsNotNull()`.
- Projection/order/group: `selectName()`, `orderByNameAsc()`, `orderByNameDesc()`, `groupByName()`.
- Group/count: `groupByName()`, `groupByNameAs(alias)`, `groupByNameWithFunction(alias, function)`, `countName()` and `countNameAs(alias)`.
- Scalar aggregate: `sumName()` uses alias `"sum_name"`; `sumNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgName()` uses alias `"avg_name"`; `avgNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minName()` uses alias `"min_name"`; `minNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxName()` uses alias `"max_name"`; `maxNameAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Text predicates: `withNameContaining(text)`, `withNameStartingWith(text)`, `withNameEndingWith(text)`.
Field Assist: [sample_link.name](assist/sample_link/fields/name.md).

- Model: `version()` / `updateVersion(const std::int64_t&)`; changed `changedVersion()`; evaluation `evalVersion()`; expression `getVersion()`.
- Scalar predicates: `withVersionIs`, `withVersionIsNot`, `withVersionGreaterThan`, `withVersionGreaterThanOrEqualTo`, `withVersionLessThan`, `withVersionLessThanOrEqualTo`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `withVersionBetween(lower, upper)`, `withVersionIn(std::span<const std::int64_t>)`, `withVersionNotIn(std::span<const std::int64_t>)`, `withVersionIsNull()`, `withVersionIsNotNull()`.
- Projection/order/group: `selectVersion()`, `orderByVersionAsc()`, `orderByVersionDesc()`, `groupByVersion()`.
- Group/count: `groupByVersion()`, `groupByVersionAs(alias)`, `groupByVersionWithFunction(alias, function)`, `countVersion()` and `countVersionAs(alias)`.
- Scalar aggregate: `sumVersion()` uses alias `"sum_version"`; `sumVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `avgVersion()` uses alias `"avg_version"`; `avgVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `minVersion()` uses alias `"min_version"`; `minVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
- Scalar aggregate: `maxVersion()` uses alias `"max_version"`; `maxVersionAs(alias)` sets an explicit result alias. Use executeForRows for aggregate outputs.
Field Assist: [sample_link.version](assist/sample_link/fields/version.md).

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
- Extend `PlatformCheckerLogic` and override `checkAndFixPlatform(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `PlatformChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `PlatformBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `SampleRecordCheckerLogic` and override `checkAndFixSampleRecord(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `SampleRecordChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `SampleRecordBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
- Extend `SampleLinkCheckerLogic` and override `checkAndFixSampleLink(user_context, entity, status, location, results)`. Allocate logic in an explicit Context, create `SampleLinkChecker` with that logic and register its `TypedEntityChecker` adapter. The built-in Context/time fixes and predefined update-load checks run before custom logic.
- `SampleLinkBehavior` preserves the EntityDataServiceBehavior SPI; runtime hook names retain snake_case.
Aggregate composition: `parent.includePendingMutationsFrom(child)` copies the child's pending additions, updates and deletions into the parent's Context for its next audited Save. Set modeled relationships before composition. It returns a checked Result; it does not load a relation, run SQL or save either object. The source retains its pending ledger for retry; original-version conflicts reject composition without partially merging. Cross-domain composition is an explicit destination copy: nested values and snapshots belong to the receiving Context, and closing the source does not invalidate them. It does not rebind the source object or extend either Context lifetime. The runtime owns ledger merging and Context validation.
