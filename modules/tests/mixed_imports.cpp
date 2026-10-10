#include <teaql/core.hpp>
#include <typeinfo>

import teaql_memory;
import teaql_core;

const std::type_info& mixed_import_value_type() {
    return typeid(teaql::core::Value);
}
