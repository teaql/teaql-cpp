#include <teaql/runtime/detail/reference_crypto.hpp>
#include <cerrno>
#include <limits>
#include <system_error>
#if defined(__linux__)
#include <fcntl.h>
#include <sys/random.h>
#include <unistd.h>
#else
#include <random>
#endif

namespace teaql::detail {
CryptoNonce reference_nonce() {
    CryptoNonce result{};
#if defined(__linux__)
    std::size_t offset = 0;
    while (offset < result.size()) {
        auto count = ::getrandom(result.data() + offset, result.size() - offset, 0);
        if (count > 0) {
            offset += static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else if (count < 0 && errno == ENOSYS) {
            break;
        } else {
            throw std::system_error(
                count == 0 ? EIO : errno, std::generic_category(), "Reference entropy acquisition");
        }
    }
    if (offset == result.size()) {
        return result;
    }

    struct Descriptor {
        int fd;

        ~Descriptor() {
            if (fd >= 0) {
                ::close(fd);
            }
        }
    } descriptor{::open("/dev/urandom", O_RDONLY | O_CLOEXEC)};

    if (descriptor.fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Reference entropy source");
    }
    while (offset < result.size()) {
        auto count = ::read(descriptor.fd, result.data() + offset, result.size() - offset);
        if (count > 0) {
            offset += static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            throw std::system_error(
                count == 0 ? EIO : errno, std::generic_category(), "Reference entropy read");
        }
    }
#else
    std::random_device source;
    if (source.entropy() == 0) {
        throw std::runtime_error("Reference entropy source is unavailable");
    }
    for (auto& byte : result) {
        byte = static_cast<std::uint8_t>(source());
    }
#endif
    return result;
}

String reference_base64(Memory r, CryptoView bytes) {
    constexpr std::string_view alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    String out(r);
    std::uint32_t bits = 0;
    unsigned count = 0;
    for (auto byte : bytes) {
        bits = (bits << 8) | byte;
        count += 8;
        while (count >= 6) {
            count -= 6;
            out.push_back(alphabet[(bits >> count) & 63]);
        }
    }
    if (count) {
        out.push_back(alphabet[(bits << (6 - count)) & 63]);
    }
    return out;
}

std::optional<CryptoBytes> reference_unbase64(Memory r, std::string_view text) {
    if (text.size() % 4 == 1) {
        return {};
    }
    CryptoBytes out(r);
    std::uint32_t bits = 0;
    unsigned count = 0;
    for (auto c : text) {
        unsigned value;
        if (c >= 'A' && c <= 'Z') {
            value = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            value = c - 'a' + 26;
        } else if (c >= '0' && c <= '9') {
            value = c - '0' + 52;
        } else if (c == '-') {
            value = 62;
        } else if (c == '_') {
            value = 63;
        } else {
            return {};
        }
        bits = (bits << 6) | value;
        count += 6;
        if (count >= 8) {
            count -= 8;
            out.push_back(static_cast<std::uint8_t>(bits >> count));
        }
    }
    if (count && (bits & ((1u << count) - 1))) {
        return {};
    }
    return out;
}

bool reference_duration_valid(ReferenceLifetime duration) noexcept {
    constexpr auto maximum = std::uint64_t(std::numeric_limits<std::int64_t>::max() / 1000);
    return duration.nanoseconds < 1'000'000'000 &&
           (duration.seconds < maximum ||
            (duration.seconds == maximum && duration.nanoseconds <= 807'000'000));
}

UtcDateTime reference_add(UtcDateTime time, ReferenceLifetime duration) {
    if (!reference_duration_valid(duration)) {
        throw std::out_of_range("Reference lifetime exceeds chrono TimeDelta");
    }
    auto seconds = time.timestamp();
    auto nanos = time.timestamp_subsec_nanos();
    if (nanos >= 1'000'000'000) {
        if (duration.seconds || duration.nanoseconds >= 2'000'000'000 - nanos) {
            nanos -= 1'000'000'000;
        } else {
            return *UtcDateTime::from_timestamp(seconds, nanos + duration.nanoseconds);
        }
    }
    auto delta = duration.seconds + (nanos + duration.nanoseconds >= 1'000'000'000);
    if (delta > std::uint64_t(std::numeric_limits<std::int64_t>::max()) ||
        seconds > std::numeric_limits<std::int64_t>::max() - static_cast<std::int64_t>(delta)) {
        throw std::out_of_range("Reference expiry exceeds chrono DateTime");
    }
    auto result = UtcDateTime::from_timestamp(seconds + static_cast<std::int64_t>(delta),
                                              (nanos + duration.nanoseconds) % 1'000'000'000);
    if (!result) {
        throw std::out_of_range("Reference expiry exceeds chrono DateTime");
    }
    return *result;
}

bool reference_time_less_equal(UtcDateTime a, UtcDateTime b) noexcept {
    return a.timestamp() < b.timestamp() ||
           (a.timestamp() == b.timestamp() &&
            a.timestamp_subsec_nanos() <= b.timestamp_subsec_nanos());
}
}
