#include <teaql/runtime/sample_data_logging.hpp>
#include <iostream>

using namespace teaql;

class DisabledSink final : public SampleDataLogSink {
public:
    using allocator_type = Allocator;

    DisabledSink(std::allocator_arg_t, Allocator allocator) : resource_(allocator.resource()) {}

    Memory memory_resource() const noexcept override {
        return resource_;
    }

    bool enabled(const UserContext&) const override {
        return false;
    }

    void info(const UserContext&, std::string_view) const override {
        throw std::logic_error("Disabled sink received a message");
    }

private:
    Memory resource_;
};

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto unreachable = [](Memory) -> String {
            throw std::logic_error("Disabled formatter executed");
        };
        detail::sample_info(*user_context, unreachable);
        auto& sink = user_context->make<DisabledSink>();
        user_context->insert_resource(*user_context, static_cast<SampleDataLogSink&>(sink));
        detail::sample_info(*user_context, unreachable);
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
