#pragma once
#include <teaql/memory/types.hpp>
#include <map>

namespace teaql {
// Source BTreeMap<String, BTreeMap<String, String>> metadata. Standard PMR
// uses-allocator construction propagates the explicit scope into nested maps.
using GeneratedWireFieldMap = std::pmr::map<String, String, std::less<>>;
using GeneratedWireFieldMappings = std::pmr::map<String, GeneratedWireFieldMap, std::less<>>;
}
