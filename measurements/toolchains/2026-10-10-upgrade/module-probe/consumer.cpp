#include <cstdint>
import teaql.toolchain_probe;

int main() {
    teaql::Ref<int> reference(42);
    teaql::MemoryLimits limits;
    teaql::Timestamp timestamp(42);
    static_assert(sizeof(teaql::Ref<int>) == 8);
    return reference.id() == 42 && limits.max_objects == 1000000 && timestamp.as_millis() == 42 ? 0
                                                                                                : 1;
}
