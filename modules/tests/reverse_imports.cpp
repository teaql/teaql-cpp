#include <cstdint>
#include <typeinfo>

import teaql_core;
import teaql_async;
import teaql_memory;

const std::type_info& reverse_import_value_type() {
    static_assert(sizeof(teaql::Ref<int>) == 8);
    return typeid(teaql::core::Value);
}
