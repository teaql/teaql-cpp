#include "../support/sample_progress_fixture.hpp"

class Collector final : public SampleDataLogSink {
public:
    using allocator_type = Allocator;

    Collector(std::allocator_arg_t, Allocator a, bool enabled)
        : messages(a.resource()), active(enabled) {}

    Memory memory_resource() const noexcept override {
        return messages.get_allocator().resource();
    }

    bool enabled(const UserContext&) const override {
        return active;
    }

    void info(const UserContext&, std::string_view message) const override {
        messages.emplace_back(message);
    }

    mutable Vector<String> messages;
    bool active;
};

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            ContextLimits limits;
            limits.user.max_bytes = 1024ull * 1024 * 1024;
            limits.max_live_user_bytes = limits.user.max_bytes;
            GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
            auto user_context = global.make_user_context();
            {
                auto r = user_context->memory_resource();
                auto input = Json::parse(r, line);
                auto& sink =
                    user_context->make<Collector>(std::get<bool>(field(*input, "enabled").data));
                user_context->insert_resource(*user_context, static_cast<SampleDataLogSink&>(sink));
                auto outcome = observe(*user_context, *input);
                auto output = detail::model_json(r);
                detail::model_put(output, "outcome", outcome);
                Json::Array messages(r);
                for (const auto& message : sink.messages) {
                    messages.emplace_back(message);
                }
                detail::model_put(output, "messages", Json(r, messages));
                std::cout << output.dump(r) << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Sample progress retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
