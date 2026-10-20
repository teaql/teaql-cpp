# Field Assist: status.category

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model: `categoryId()`; constant ID assignment is internal; changed `changedCategoryId()`; evaluation `evalCategoryId()`; expression `getCategoryId()`.
- Candidate filters: `withCategoryIsC1()` and `withCategoryIsNotC1()`; exact value `1`.
- Candidate model update: `updateCategoryToC1()`; model and E check: `categoryIsC1()`. E preserves NotLoaded and never performs SQL.
- Candidate filters: `withCategoryIsC22()` and `withCategoryIsNotC22()`; exact value `22`.
- Candidate model update: `updateCategoryToC22()`; model and E check: `categoryIsC22()`. E preserves NotLoaded and never performs SQL.
- Scalar predicates: `withCategoryIs`, `withCategoryIsNot`, `withCategoryGreaterThan`, `withCategoryGreaterThanOrEqualTo`, `withCategoryLessThan`, `withCategoryLessThanOrEqualTo`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `withCategoryBetween(lower, upper)`, `withCategoryIn(std::span<const std::uint64_t>)`, `withCategoryNotIn(std::span<const std::uint64_t>)`, `withCategoryIsNull()`, `withCategoryIsNotNull()`.
- Projection/order/group: `selectCategoryId()`, `orderByCategoryIdAsc()`, `orderByCategoryIdDesc()`, `groupByCategory()`.
- Group/count: `groupByCategory()`, `groupByCategoryAs(alias)`, `groupByCategoryWithFunction(alias, function)`, `countCategory()` and `countCategoryAs(alias)`.
- Forward relation: `category()` returns const Category*; constant assignment uses candidate update methods; query `selectCategory()`, `selectCategoryWith(CategoryRequest<P>)`, `withCategory(CategoryRequest<P>)`; expression `getCategory()`.

Null alias: `withCategoryIsKnown()` selects SQL non-NULL, not E load availability.

Null alias: `withCategoryIsUnknown()` selects SQL NULL, not E load availability.

A candidate predicate occupying a Null/Known name retains its literal candidate meaning; use the exact candidate entries above.

Relation evaluation: `evalCategory()`. Matching: `withCategoryMatching(child)`, `withoutCategoryMatching(child)`. Presence: `haveCategory()`, `haveNoCategory()`. Facets: `facetByCategoryAs(name, child)`, `facetByCategoryAsWithOptions(name, child, include_all_facets)`.

Grouping/details: `groupByCategoryWith(child)`, `groupByCategoryWithDetails()`, `groupByCategoryWithDetailsFrom(child)`. Roll-up: `rollUpToCategory()`, `rollUpToCategoryWith(child)`.

[Model/action index](../../../ASSIST.md)
