# Generated C++ model-aware API

Namespace: `cpp_types`. Allocate with UserContext. Execute only after purpose and comment.

## Person
Entries: `Q::people(user)` and `Q::people_minimal(user)`. Evaluation: `E::person(entity)`.

- Model: `id()` / `update_id(const std::uint64_t&)`; expression `get_id()`.
- Scalar predicates: `whose_id_is`, `whose_id_is_not`, `whose_id_greater_than`, `whose_id_greater_than_or_equal_to`, `whose_id_less_than`, `whose_id_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `whose_id_between(lower, upper)`, `whose_id_in(std::span<const std::uint64_t>)`, `whose_id_not_in(std::span<const std::uint64_t>)`, `whose_id_is_null()`, `whose_id_is_not_null()`.
- Projection/order/group: `select_id()`, `order_by_id_asc()`, `order_by_id_desc()`, `group_by_id()`.
- Model: `email()` / `update_email(std::string_view)`; expression `get_email()`.
- Scalar predicates: `whose_email_is`, `whose_email_is_not`, `whose_email_greater_than`, `whose_email_greater_than_or_equal_to`, `whose_email_less_than`, `whose_email_less_than_or_equal_to`. Arguments: `std::string_view`.
- Range/set/null predicates: `whose_email_between(lower, upper)`, `whose_email_in(std::span<const String>)`, `whose_email_not_in(std::span<const String>)`, `whose_email_is_null()`, `whose_email_is_not_null()`.
- Projection/order/group: `select_email()`, `order_by_email_asc()`, `order_by_email_desc()`, `group_by_email()`.
- Scalar aggregates: `sum_email_as(alias)`, `avg_email_as(alias)`, `min_email_as(alias)`, `max_email_as(alias)`. Use execute_for_rows for aggregate outputs.
- Text predicates: `whose_email_contains(text)`, `whose_email_starts_with(text)`, `whose_email_ends_with(text)`.
- Model: `active()` / `update_active(const bool&)`; expression `get_active()`.
- Scalar predicates: `whose_active_is`, `whose_active_is_not`, `whose_active_greater_than`, `whose_active_greater_than_or_equal_to`, `whose_active_less_than`, `whose_active_less_than_or_equal_to`. Arguments: `const bool&`.
- Range/set/null predicates: `whose_active_between(lower, upper)`, `whose_active_in(std::span<const bool>)`, `whose_active_not_in(std::span<const bool>)`, `whose_active_is_null()`, `whose_active_is_not_null()`.
- Projection/order/group: `select_active()`, `order_by_active_asc()`, `order_by_active_desc()`, `group_by_active()`.
- Scalar aggregates: `sum_active_as(alias)`, `avg_active_as(alias)`, `min_active_as(alias)`, `max_active_as(alias)`. Use execute_for_rows for aggregate outputs.
- Boolean predicates: `who_are_active()`, `who_are_not_active()`.
- Model: `nickname()` / `update_nickname(const std::optional<String>&)`; expression `get_nickname()`.
- Scalar predicates: `whose_nickname_is`, `whose_nickname_is_not`, `whose_nickname_greater_than`, `whose_nickname_greater_than_or_equal_to`, `whose_nickname_less_than`, `whose_nickname_less_than_or_equal_to`. Arguments: `std::string_view`.
- Range/set/null predicates: `whose_nickname_between(lower, upper)`, `whose_nickname_in(std::span<const String>)`, `whose_nickname_not_in(std::span<const String>)`, `whose_nickname_is_null()`, `whose_nickname_is_not_null()`.
- Projection/order/group: `select_nickname()`, `order_by_nickname_asc()`, `order_by_nickname_desc()`, `group_by_nickname()`.
- Scalar aggregates: `sum_nickname_as(alias)`, `avg_nickname_as(alias)`, `min_nickname_as(alias)`, `max_nickname_as(alias)`. Use execute_for_rows for aggregate outputs.
- Text predicates: `whose_nickname_contains(text)`, `whose_nickname_starts_with(text)`, `whose_nickname_ends_with(text)`.
- Model: `balance()` / `update_balance(const Decimal&)`; expression `get_balance()`.
- Scalar predicates: `whose_balance_is`, `whose_balance_is_not`, `whose_balance_greater_than`, `whose_balance_greater_than_or_equal_to`, `whose_balance_less_than`, `whose_balance_less_than_or_equal_to`. Arguments: `const Decimal&`.
- Range/set/null predicates: `whose_balance_between(lower, upper)`, `whose_balance_in(std::span<const Decimal>)`, `whose_balance_not_in(std::span<const Decimal>)`, `whose_balance_is_null()`, `whose_balance_is_not_null()`.
- Projection/order/group: `select_balance()`, `order_by_balance_asc()`, `order_by_balance_desc()`, `group_by_balance()`.
- Scalar aggregates: `sum_balance_as(alias)`, `avg_balance_as(alias)`, `min_balance_as(alias)`, `max_balance_as(alias)`. Use execute_for_rows for aggregate outputs.
- Model: `height()` / `update_height(const double&)`; expression `get_height()`.
- Scalar predicates: `whose_height_is`, `whose_height_is_not`, `whose_height_greater_than`, `whose_height_greater_than_or_equal_to`, `whose_height_less_than`, `whose_height_less_than_or_equal_to`. Arguments: `const double&`.
- Range/set/null predicates: `whose_height_between(lower, upper)`, `whose_height_in(std::span<const double>)`, `whose_height_not_in(std::span<const double>)`, `whose_height_is_null()`, `whose_height_is_not_null()`.
- Projection/order/group: `select_height()`, `order_by_height_asc()`, `order_by_height_desc()`, `group_by_height()`.
- Scalar aggregates: `sum_height_as(alias)`, `avg_height_as(alias)`, `min_height_as(alias)`, `max_height_as(alias)`. Use execute_for_rows for aggregate outputs.
- Model: `ratio()` / `update_ratio(const float&)`; expression `get_ratio()`.
- Scalar predicates: `whose_ratio_is`, `whose_ratio_is_not`, `whose_ratio_greater_than`, `whose_ratio_greater_than_or_equal_to`, `whose_ratio_less_than`, `whose_ratio_less_than_or_equal_to`. Arguments: `const float&`.
- Range/set/null predicates: `whose_ratio_between(lower, upper)`, `whose_ratio_in(std::span<const float>)`, `whose_ratio_not_in(std::span<const float>)`, `whose_ratio_is_null()`, `whose_ratio_is_not_null()`.
- Projection/order/group: `select_ratio()`, `order_by_ratio_asc()`, `order_by_ratio_desc()`, `group_by_ratio()`.
- Scalar aggregates: `sum_ratio_as(alias)`, `avg_ratio_as(alias)`, `min_ratio_as(alias)`, `max_ratio_as(alias)`. Use execute_for_rows for aggregate outputs.
- Model: `age()` / `update_age(const std::int64_t&)`; expression `get_age()`.
- Scalar predicates: `whose_age_is`, `whose_age_is_not`, `whose_age_greater_than`, `whose_age_greater_than_or_equal_to`, `whose_age_less_than`, `whose_age_less_than_or_equal_to`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `whose_age_between(lower, upper)`, `whose_age_in(std::span<const std::int64_t>)`, `whose_age_not_in(std::span<const std::int64_t>)`, `whose_age_is_null()`, `whose_age_is_not_null()`.
- Projection/order/group: `select_age()`, `order_by_age_asc()`, `order_by_age_desc()`, `group_by_age()`.
- Scalar aggregates: `sum_age_as(alias)`, `avg_age_as(alias)`, `min_age_as(alias)`, `max_age_as(alias)`. Use execute_for_rows for aggregate outputs.
- Model: `birthday()` / `update_birthday(const Date&)`; expression `get_birthday()`.
- Scalar predicates: `whose_birthday_is`, `whose_birthday_is_not`, `whose_birthday_greater_than`, `whose_birthday_greater_than_or_equal_to`, `whose_birthday_less_than`, `whose_birthday_less_than_or_equal_to`. Arguments: `const Date&`.
- Range/set/null predicates: `whose_birthday_between(lower, upper)`, `whose_birthday_in(std::span<const Date>)`, `whose_birthday_not_in(std::span<const Date>)`, `whose_birthday_is_null()`, `whose_birthday_is_not_null()`.
- Projection/order/group: `select_birthday()`, `order_by_birthday_asc()`, `order_by_birthday_desc()`, `group_by_birthday()`.
- Scalar aggregates: `sum_birthday_as(alias)`, `avg_birthday_as(alias)`, `min_birthday_as(alias)`, `max_birthday_as(alias)`. Use execute_for_rows for aggregate outputs.
- Model: `updated_at()` / `update_updated_at(const Timestamp&)`; expression `get_updated_at()`.
- Scalar predicates: `whose_updated_at_is`, `whose_updated_at_is_not`, `whose_updated_at_greater_than`, `whose_updated_at_greater_than_or_equal_to`, `whose_updated_at_less_than`, `whose_updated_at_less_than_or_equal_to`. Arguments: `const Timestamp&`.
- Range/set/null predicates: `whose_updated_at_between(lower, upper)`, `whose_updated_at_in(std::span<const Timestamp>)`, `whose_updated_at_not_in(std::span<const Timestamp>)`, `whose_updated_at_is_null()`, `whose_updated_at_is_not_null()`.
- Projection/order/group: `select_updated_at()`, `order_by_updated_at_asc()`, `order_by_updated_at_desc()`, `group_by_updated_at()`.
- Scalar aggregates: `sum_updated_at_as(alias)`, `avg_updated_at_as(alias)`, `min_updated_at_as(alias)`, `max_updated_at_as(alias)`. Use execute_for_rows for aggregate outputs.
- Model: `payload()` / `update_payload(const std::optional<Json>&)`; expression `get_payload()`.
- Scalar predicates: `whose_payload_is`, `whose_payload_is_not`, `whose_payload_greater_than`, `whose_payload_greater_than_or_equal_to`, `whose_payload_less_than`, `whose_payload_less_than_or_equal_to`. Arguments: `const Json&`.
- Range/set/null predicates: `whose_payload_between(lower, upper)`, `whose_payload_in(std::span<const Json>)`, `whose_payload_not_in(std::span<const Json>)`, `whose_payload_is_null()`, `whose_payload_is_not_null()`.
- Projection/order/group: `select_payload()`, `order_by_payload_asc()`, `order_by_payload_desc()`, `group_by_payload()`.
- Scalar aggregates: `sum_payload_as(alias)`, `avg_payload_as(alias)`, `min_payload_as(alias)`, `max_payload_as(alias)`. Use execute_for_rows for aggregate outputs.
- Model: `version()` / `update_version(const std::int64_t&)`; expression `get_version()`.
- Scalar predicates: `whose_version_is`, `whose_version_is_not`, `whose_version_greater_than`, `whose_version_greater_than_or_equal_to`, `whose_version_less_than`, `whose_version_less_than_or_equal_to`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `whose_version_between(lower, upper)`, `whose_version_in(std::span<const std::int64_t>)`, `whose_version_not_in(std::span<const std::int64_t>)`, `whose_version_is_null()`, `whose_version_is_not_null()`.
- Projection/order/group: `select_version()`, `order_by_version_asc()`, `order_by_version_desc()`, `group_by_version()`.
- Reverse relation: `device_list()` returns SmartList<Device>&; query `select_device_list()`, `select_device_list_with(DeviceRequest<P>)`, `with_device_list(DeviceRequest<P>)`; expression `get_device_list()`.
## Device
Entries: `Q::devices(user)` and `Q::devices_minimal(user)`. Evaluation: `E::device(entity)`.

- Model: `id()` / `update_id(const std::uint64_t&)`; expression `get_id()`.
- Scalar predicates: `with_id_is`, `with_id_is_not`, `with_id_greater_than`, `with_id_greater_than_or_equal_to`, `with_id_less_than`, `with_id_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `with_id_between(lower, upper)`, `with_id_in(std::span<const std::uint64_t>)`, `with_id_not_in(std::span<const std::uint64_t>)`, `with_id_is_null()`, `with_id_is_not_null()`.
- Projection/order/group: `select_id()`, `order_by_id_asc()`, `order_by_id_desc()`, `group_by_id()`.
- Model: `code()` / `update_code(std::string_view)`; expression `get_code()`.
- Scalar predicates: `with_code_is`, `with_code_is_not`, `with_code_greater_than`, `with_code_greater_than_or_equal_to`, `with_code_less_than`, `with_code_less_than_or_equal_to`. Arguments: `std::string_view`.
- Range/set/null predicates: `with_code_between(lower, upper)`, `with_code_in(std::span<const String>)`, `with_code_not_in(std::span<const String>)`, `with_code_is_null()`, `with_code_is_not_null()`.
- Projection/order/group: `select_code()`, `order_by_code_asc()`, `order_by_code_desc()`, `group_by_code()`.
- Scalar aggregates: `sum_code_as(alias)`, `avg_code_as(alias)`, `min_code_as(alias)`, `max_code_as(alias)`. Use execute_for_rows for aggregate outputs.
- Text predicates: `with_code_containing(text)`, `with_code_starting_with(text)`, `with_code_ending_with(text)`.
- Model: `active()` / `update_active(const bool&)`; expression `get_active()`.
- Scalar predicates: `with_active_is`, `with_active_is_not`, `with_active_greater_than`, `with_active_greater_than_or_equal_to`, `with_active_less_than`, `with_active_less_than_or_equal_to`. Arguments: `const bool&`.
- Range/set/null predicates: `with_active_between(lower, upper)`, `with_active_in(std::span<const bool>)`, `with_active_not_in(std::span<const bool>)`, `with_active_is_null()`, `with_active_is_not_null()`.
- Projection/order/group: `select_active()`, `order_by_active_asc()`, `order_by_active_desc()`, `group_by_active()`.
- Scalar aggregates: `sum_active_as(alias)`, `avg_active_as(alias)`, `min_active_as(alias)`, `max_active_as(alias)`. Use execute_for_rows for aggregate outputs.
- Boolean predicates: `which_are_active()`, `which_are_not_active()`.
- Model: `owner_id()` / `update_owner_id(const std::uint64_t&)`; expression `get_owner_id()`.
- Scalar predicates: `with_owner_is`, `with_owner_is_not`, `with_owner_greater_than`, `with_owner_greater_than_or_equal_to`, `with_owner_less_than`, `with_owner_less_than_or_equal_to`. Arguments: `const std::uint64_t&`.
- Range/set/null predicates: `with_owner_between(lower, upper)`, `with_owner_in(std::span<const std::uint64_t>)`, `with_owner_not_in(std::span<const std::uint64_t>)`, `with_owner_is_null()`, `with_owner_is_not_null()`.
- Projection/order/group: `select_owner_id()`, `order_by_owner_id_asc()`, `order_by_owner_id_desc()`, `group_by_owner_id()`.
- Scalar aggregates: `sum_owner_id_as(alias)`, `avg_owner_id_as(alias)`, `min_owner_id_as(alias)`, `max_owner_id_as(alias)`. Use execute_for_rows for aggregate outputs.
- Forward relation: `owner()` returns const Person*, `update_owner(const Person&)`; query `select_owner()`, `select_owner_with(PersonRequest<P>)`, `with_owner(PersonRequest<P>)`; expression `get_owner()`.
- Model: `version()` / `update_version(const std::int64_t&)`; expression `get_version()`.
- Scalar predicates: `with_version_is`, `with_version_is_not`, `with_version_greater_than`, `with_version_greater_than_or_equal_to`, `with_version_less_than`, `with_version_less_than_or_equal_to`. Arguments: `const std::int64_t&`.
- Range/set/null predicates: `with_version_between(lower, upper)`, `with_version_in(std::span<const std::int64_t>)`, `with_version_not_in(std::span<const std::int64_t>)`, `with_version_is_null()`, `with_version_is_not_null()`.
- Projection/order/group: `select_version()`, `order_by_version_asc()`, `order_by_version_desc()`, `group_by_version()`.
## OrderStatus
Entries: `Q::order_statuses(user)` and `Q::order_statuses_minimal(user)`. Evaluation: `E::order_status(entity)`.

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
## Category
Entries: `Q::categories(user)` and `Q::categories_minimal(user)`. Evaluation: `E::category(entity)`.

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
## Child
Entries: `Q::children(user)` and `Q::children_minimal(user)`. Evaluation: `E::child(entity)`.

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
## Inventory
Entries: `Q::inventory(user)` and `Q::inventory_minimal(user)`. Evaluation: `E::inventory(entity)`.

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

Common request methods: `comment(text)`, `purpose(text)`, `filter(expr)`, `and_filter(expr)`, `or_filter(expr)`, `select_self()`, `select_self_fields()`, `limit(n)`, `offset(n)`, `skip(n)`, `hard_limit(n)`, `search_with_text(text)`. Purpose stages an executable type; modifiers remain available.
Executable methods (exactly one UserContext argument): `execute_for_list(user)`, `execute_for_entity_list(user)`, `execute(user)`, `execute_for_entity(user)`, `execute_for_rows(user)`, `new_entity(user)`. Pollable results use `.wait()`; only new_entity returns a managed model directly.
Expression methods: `eval()`, `unwrap()`, `is_null()`, `is_not_null()`, `or_if_null(fallback)`, `or_else_if_null(callback)`, `or_default_if_null()`. List expressions add `size()` (scalar expression), `first()` and `get(index)` (entity expressions). NotLoaded is never a NULL fallback.
Save: `AuditedSaveExt<Model>::save(std::move(entity).audit_as("reason"), user).wait()`. Allocate requests/models with UserContext and call setup(user) once at bootstrap. Use explicit destination copies before closing the owning context. Unknown fields are compile errors.
