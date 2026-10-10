#include <teaql/core/time.hpp>
#include <teaql/core/value.hpp>
#include <teaql/runtime/detail/data_service/data_service.hpp>
#include <typeinfo>

teaql::Timestamp header_timestamp(std::int64_t milliseconds) {
    return teaql::Timestamp::from_u64(static_cast<std::uint64_t>(milliseconds));
}

const std::type_info& header_value_type() {
    return typeid(teaql::Value);
}

bool header_value_matches(const teaql::Value& value, std::string_view expected) {
    return std::get<teaql::String>(value.data) == expected;
}

const std::type_info& header_metadata_type() {
    return typeid(teaql::detail::UserContextMetadata);
}

std::size_t header_metadata_size() {
    return sizeof(teaql::detail::UserContextMetadata);
}

teaql::Memory header_metadata_memory(const teaql::MetadataStore& metadata) {
    return metadata.memory_resource();
}
