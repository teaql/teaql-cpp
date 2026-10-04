#pragma once
#include <memory_resource>
#include <string>
#include <vector>

namespace teaql {
using Memory = std::pmr::memory_resource*;
using String = std::pmr::string;
template<class T> using Vector = std::pmr::vector<T>;
using Allocator = std::pmr::polymorphic_allocator<std::byte>;
}
