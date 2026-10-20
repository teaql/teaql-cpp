#include <teaql/provider_sqlite/runtime_config.hpp>
#include <filesystem>
#include <charconv>

namespace teaql {
namespace {
void create_generated_parent_directories(Memory resource,
                                         const std::filesystem::path& path,
                                         std::error_code& error) {
    Vector<std::filesystem::path> uncreated(resource);
    auto current = path;
    // Rust attempts mkdir on each ancestor before inspecting an existing path.
    // create_directories has a different error for an existing ordinary file.
    while (!current.empty() && current.parent_path() != current) {
        std::filesystem::create_directory(current, error);
        if (!error) {
            break;
        }
        if (error != std::errc::no_such_file_or_directory) {
            return;
        }
        uncreated.emplace_back(current);
        current = current.parent_path();
    }
    error.clear();
    for (auto directory = uncreated.rbegin(); directory != uncreated.rend(); ++directory) {
        std::filesystem::create_directory(*directory, error);
        if (error) {
            return;
        }
    }
}
}

Result<String, GeneratedRuntimeConfigurationError>
prepare_generated_sqlite_runtime_path(Memory resource, std::string_view database_url) {
    using Output = Result<String, GeneratedRuntimeConfigurationError>;
    auto path = database_url;
    if (path.starts_with("sqlite:")) {
        path.remove_prefix(7);
        // Rust trims repetitions of the whole "//" pattern, not single slashes.
        while (path.starts_with("//")) {
            path.remove_prefix(2);
        }
    }
    path = path.substr(0, path.find('?'));
    String filename(path, resource);
    // std::filesystem is an immediate OS adapter. Its temporary path objects
    // leave scope here; only the explicitly allocated filename is retained.
    auto parent = std::filesystem::path(filename).parent_path();
    if (!parent.empty()) {
        if (parent.native().find('\0') != parent.native().npos) {
            return Output::err(resource,
                               GeneratedRuntimeConfigurationError::connection_error(
                                   resource, "file name contained an unexpected NUL byte"));
        }
        std::error_code error;
        create_generated_parent_directories(resource, parent, error);
        if (error) {
            String message(error.message(), resource);
            message += " (os error ";
            char digits[32];
            auto number = std::to_chars(std::begin(digits), std::end(digits), error.value());
            message.append(digits, number.ptr);
            message += ')';
            return Output::err(
                resource, GeneratedRuntimeConfigurationError::connection_error(resource, message));
        }
    }
    if (auto nul = filename.find('\0'); nul != filename.npos) {
        String message("nul byte found in provided data at position: ", resource);
        char digits[32];
        auto number = std::to_chars(std::begin(digits), std::end(digits), nul);
        message.append(digits, number.ptr);
        return Output::err(resource,
                           GeneratedRuntimeConfigurationError::connection_error(resource, message));
    }
    return Output::ok(resource, std::move(filename));
}
}
