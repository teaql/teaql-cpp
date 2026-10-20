# Field Assist: category.category_list

Use the exact generated names below. SDK setters are not a client-writable allow-list. Do not accept internal IDs, versions, Context-root assignments or readonly/generated server fields from untrusted input. Choose writable business fields explicitly. Load complete scalar state before modifying a persisted entity. E reads preserve NULL and NotLoaded without SQL.

- Model relation evaluation: `evalCategoryList()`; query view handle: `categoryListHandle()`.
- Reverse relation: `categoryList()` returns SmartList<Category>&; query `selectCategoryList()`, `selectCategoryListWith(CategoryRequest<P>)`, `withCategoryList(CategoryRequest<P>)`; expression `getCategoryList()`.

Presence: `haveCategories()`, `haveNoCategories()`. Matching: `withCategoryListMatching(child)`, `withoutCategoryListMatching(child)`.

Count: `countCategories()`, `countCategoriesAs(alias)`, `countCategoriesWith(alias, child)`. Statistics/details: `statsFromCategories(child)`, `statsFromCategoriesAs(alias, child)`, `groupByCategoriesWithDetails(child)`.

[Model/action index](../../../ASSIST.md)
