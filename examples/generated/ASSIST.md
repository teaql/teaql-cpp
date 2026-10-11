# Generated C++ model-aware API

Namespace: `cpp_examples`. Allocate with UserContext. Execute only after purpose and comment.

## Product
Entries: `Q::products(user)` and `Q::products_minimal(user)`. Evaluation: `E::product(entity)`.

- Model: `id()` / `update_id(const std::uint64_t&)`; expression `get_id()`.
- Scalar predicates: `with_id_is`, `with_id_is_not`, `with_id_greater_than`, `with_id_greater_than_or_equal_to`, `with_id_less_than`, `with_id_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `with_id_between(lower, upper)`, `with_id_in(std::span<const std::uint64_t>)`, `with_id_not_in(std::span<const std::uint64_t>)`, `with_id_is_null()`, `with_id_is_not_null()`.
- Projection/order/group: `select_id()`, `order_by_id_asc()`, `order_by_id_desc()`, `group_by_id()`.
- Model: `name()` / `update_name(std::string_view)`; expression `get_name()`.
- Scalar predicates: `with_name_is`, `with_name_is_not`, `with_name_greater_than`, `with_name_greater_than_or_equal_to`, `with_name_less_than`, `with_name_less_than_or_equal_to`. Arguments: `std::string_view`.
- Range/set/null predicates: `with_name_between(lower, upper)`, `with_name_in(std::span<const String>)`, `with_name_not_in(std::span<const String>)`, `with_name_is_null()`, `with_name_is_not_null()`.
- Projection/order/group: `select_name()`, `order_by_name_asc()`, `order_by_name_desc()`, `group_by_name()`.
- Scalar aggregates: `sum_name_as(alias)`, `avg_name_as(alias)`, `min_name_as(alias)`, `max_name_as(alias)`. Use execute_for_rows for aggregate outputs.
- Text predicates: `with_name_containing(text)`, `with_name_starting_with(text)`, `with_name_ending_with(text)`.
- Model: `version()` / `update_version(const std::int64_t&)`; expression `get_version()`.
- Scalar predicates: `with_version_is`, `with_version_is_not`, `with_version_greater_than`, `with_version_greater_than_or_equal_to`, `with_version_less_than`, `with_version_less_than_or_equal_to`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `with_version_between(lower, upper)`, `with_version_in(std::span<const std::int64_t>)`, `with_version_not_in(std::span<const std::int64_t>)`, `with_version_is_null()`, `with_version_is_not_null()`.
- Projection/order/group: `select_version()`, `order_by_version_asc()`, `order_by_version_desc()`, `group_by_version()`.
- Reverse relation: `order_line_list()` returns SmartList<OrderLine>&; query `select_order_line_list()`, `select_order_line_list_with(OrderLineRequest<P>)`, `with_order_line_list(OrderLineRequest<P>)`; expression `get_order_line_list()`.
## Order
Entries: `Q::orders(user)` and `Q::orders_minimal(user)`. Evaluation: `E::order(entity)`.

- Model: `id()` / `update_id(const std::uint64_t&)`; expression `get_id()`.
- Scalar predicates: `with_id_is`, `with_id_is_not`, `with_id_greater_than`, `with_id_greater_than_or_equal_to`, `with_id_less_than`, `with_id_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `with_id_between(lower, upper)`, `with_id_in(std::span<const std::uint64_t>)`, `with_id_not_in(std::span<const std::uint64_t>)`, `with_id_is_null()`, `with_id_is_not_null()`.
- Projection/order/group: `select_id()`, `order_by_id_asc()`, `order_by_id_desc()`, `group_by_id()`.
- Model: `name()` / `update_name(std::string_view)`; expression `get_name()`.
- Scalar predicates: `with_name_is`, `with_name_is_not`, `with_name_greater_than`, `with_name_greater_than_or_equal_to`, `with_name_less_than`, `with_name_less_than_or_equal_to`. Arguments: `std::string_view`.
- Range/set/null predicates: `with_name_between(lower, upper)`, `with_name_in(std::span<const String>)`, `with_name_not_in(std::span<const String>)`, `with_name_is_null()`, `with_name_is_not_null()`.
- Projection/order/group: `select_name()`, `order_by_name_asc()`, `order_by_name_desc()`, `group_by_name()`.
- Scalar aggregates: `sum_name_as(alias)`, `avg_name_as(alias)`, `min_name_as(alias)`, `max_name_as(alias)`. Use execute_for_rows for aggregate outputs.
- Text predicates: `with_name_containing(text)`, `with_name_starting_with(text)`, `with_name_ending_with(text)`.
- Model: `version()` / `update_version(const std::int64_t&)`; expression `get_version()`.
- Scalar predicates: `with_version_is`, `with_version_is_not`, `with_version_greater_than`, `with_version_greater_than_or_equal_to`, `with_version_less_than`, `with_version_less_than_or_equal_to`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `with_version_between(lower, upper)`, `with_version_in(std::span<const std::int64_t>)`, `with_version_not_in(std::span<const std::int64_t>)`, `with_version_is_null()`, `with_version_is_not_null()`.
- Projection/order/group: `select_version()`, `order_by_version_asc()`, `order_by_version_desc()`, `group_by_version()`.
- Reverse relation: `lines()` returns SmartList<OrderLine>&; query `select_lines()`, `select_lines_with(OrderLineRequest<P>)`, `with_lines(OrderLineRequest<P>)`; expression `get_lines()`.
## OrderLine
Entries: `Q::order_lines(user)` and `Q::order_lines_minimal(user)`. Evaluation: `E::order_line(entity)`.

- Model: `id()` / `update_id(const std::uint64_t&)`; expression `get_id()`.
- Scalar predicates: `with_id_is`, `with_id_is_not`, `with_id_greater_than`, `with_id_greater_than_or_equal_to`, `with_id_less_than`, `with_id_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `with_id_between(lower, upper)`, `with_id_in(std::span<const std::uint64_t>)`, `with_id_not_in(std::span<const std::uint64_t>)`, `with_id_is_null()`, `with_id_is_not_null()`.
- Projection/order/group: `select_id()`, `order_by_id_asc()`, `order_by_id_desc()`, `group_by_id()`.
- Model: `name()` / `update_name(std::string_view)`; expression `get_name()`.
- Scalar predicates: `with_name_is`, `with_name_is_not`, `with_name_greater_than`, `with_name_greater_than_or_equal_to`, `with_name_less_than`, `with_name_less_than_or_equal_to`. Arguments: `std::string_view`.
- Range/set/null predicates: `with_name_between(lower, upper)`, `with_name_in(std::span<const String>)`, `with_name_not_in(std::span<const String>)`, `with_name_is_null()`, `with_name_is_not_null()`.
- Projection/order/group: `select_name()`, `order_by_name_asc()`, `order_by_name_desc()`, `group_by_name()`.
- Scalar aggregates: `sum_name_as(alias)`, `avg_name_as(alias)`, `min_name_as(alias)`, `max_name_as(alias)`. Use execute_for_rows for aggregate outputs.
- Text predicates: `with_name_containing(text)`, `with_name_starting_with(text)`, `with_name_ending_with(text)`.
- Model: `order_id()` / `update_order_id(const std::uint64_t&)`; expression `get_order_id()`.
- Scalar predicates: `with_order_is`, `with_order_is_not`, `with_order_greater_than`, `with_order_greater_than_or_equal_to`, `with_order_less_than`, `with_order_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `with_order_between(lower, upper)`, `with_order_in(std::span<const std::uint64_t>)`, `with_order_not_in(std::span<const std::uint64_t>)`, `with_order_is_null()`, `with_order_is_not_null()`.
- Projection/order/group: `select_order_id()`, `order_by_order_id_asc()`, `order_by_order_id_desc()`, `group_by_order_id()`.
- Scalar aggregates: `sum_order_id_as(alias)`, `avg_order_id_as(alias)`, `min_order_id_as(alias)`, `max_order_id_as(alias)`. Use execute_for_rows for aggregate outputs.
- Forward relation: `order()` returns const Order*, `update_order(const Order&)`; query `select_order()`, `select_order_with(OrderRequest<P>)`, `with_order(OrderRequest<P>)`; expression `get_order()`.
- Model: `product_id()` / `update_product_id(const std::uint64_t&)`; expression `get_product_id()`.
- Scalar predicates: `with_product_is`, `with_product_is_not`, `with_product_greater_than`, `with_product_greater_than_or_equal_to`, `with_product_less_than`, `with_product_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `with_product_between(lower, upper)`, `with_product_in(std::span<const std::uint64_t>)`, `with_product_not_in(std::span<const std::uint64_t>)`, `with_product_is_null()`, `with_product_is_not_null()`.
- Projection/order/group: `select_product_id()`, `order_by_product_id_asc()`, `order_by_product_id_desc()`, `group_by_product_id()`.
- Scalar aggregates: `sum_product_id_as(alias)`, `avg_product_id_as(alias)`, `min_product_id_as(alias)`, `max_product_id_as(alias)`. Use execute_for_rows for aggregate outputs.
- Forward relation: `product()` returns const Product*, `update_product(const Product&)`; query `select_product()`, `select_product_with(ProductRequest<P>)`, `with_product(ProductRequest<P>)`; expression `get_product()`.
- Model: `version()` / `update_version(const std::int64_t&)`; expression `get_version()`.
- Scalar predicates: `with_version_is`, `with_version_is_not`, `with_version_greater_than`, `with_version_greater_than_or_equal_to`, `with_version_less_than`, `with_version_less_than_or_equal_to`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `with_version_between(lower, upper)`, `with_version_in(std::span<const std::int64_t>)`, `with_version_not_in(std::span<const std::int64_t>)`, `with_version_is_null()`, `with_version_is_not_null()`.
- Projection/order/group: `select_version()`, `order_by_version_asc()`, `order_by_version_desc()`, `group_by_version()`.

Common request methods: `comment(text)`, `purpose(text)`, `filter(expr)`, `and_filter(expr)`, `or_filter(expr)`, `select_self()`, `select_self_fields()`, `limit(n)`, `offset(n)`, `skip(n)`, `hard_limit(n)`, `search_with_text(text)`. Purpose stages an executable type; modifiers remain available.
Executable methods (exactly one UserContext argument): `execute_for_list(user)`, `execute_for_entity_list(user)`, `execute(user)`, `execute_for_entity(user)`, `execute_for_rows(user)`, `new_entity(user)`. Pollable results use `.wait()`; only new_entity returns a managed model directly.
Expression methods: `eval()`, `unwrap()`, `is_null()`, `is_not_null()`, `or_if_null(fallback)`, `or_else_if_null(callback)`, `or_default_if_null()`. List expressions add `size()` (scalar expression), `first()` and `get(index)` (entity expressions). NotLoaded is never a NULL fallback.
Save: `AuditedSaveExt<Model>::save(std::move(entity).audit_as("reason"), user).wait()`. Allocate requests/models with UserContext and call setup(user) once at bootstrap. Use explicit destination copies before closing the owning context. Unknown fields are compile errors.
