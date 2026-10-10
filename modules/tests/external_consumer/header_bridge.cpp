#include <teaql/core/value.hpp>
#include <typeinfo>

const std::type_info& installed_header_value_type() {
    return typeid(teaql::Value);
}

bool installed_header_value_matches(const teaql::Value& value, std::string_view expected) {
    return value.try_text() == expected;
}
