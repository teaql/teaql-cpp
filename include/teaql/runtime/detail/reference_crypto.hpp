#pragma once
#include <teaql/runtime/entity_reference.hpp>
#include <teaql/runtime/resources.hpp>

namespace teaql::detail {
// Synchronous callbacks must not reclaim either their owner or output domain.
class ReferenceCallGuard final {
public:
    ReferenceCallGuard(Memory owner, Memory destination) {
        auto stop = [](void*) noexcept {
            std::terminate();
        };
        own_.attach(resource_arena(owner), nullptr, stop);
        if (destination != owner) {
            output_.attach(resource_arena(destination), nullptr, stop);
        }
    }

private:
    Arena::Quiescence own_, output_;
};

CryptoNonce reference_nonce();
String reference_base64(Memory, CryptoView);
std::optional<CryptoBytes> reference_unbase64(Memory, std::string_view);
bool reference_duration_valid(ReferenceLifetime) noexcept;
UtcDateTime reference_add(UtcDateTime, ReferenceLifetime);
bool reference_time_less_equal(UtcDateTime, UtcDateTime) noexcept;

inline CryptoView reference_bytes(std::string_view text) {
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}
}
