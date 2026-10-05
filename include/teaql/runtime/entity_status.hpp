#pragma once
#include <stdexcept>
#include <string_view>

namespace teaql {
enum class EntityAction { Update, Delete, Persist, Recover };
enum class EntityStatus {
    New,
    Persisted,
    PersistedDeleted,
    Updated,
    UpdatedDeleted,
    UpdatedRecover,
    Refer
};

// Exact transition table from Rust v5.0.5 runtime/entity_status.rs.
constexpr EntityStatus next_status(EntityStatus status, EntityAction action) {
    switch (status) {
    case EntityStatus::New:
        if (action == EntityAction::Update) {
            return EntityStatus::New;
        }
        if (action == EntityAction::Persist) {
            return EntityStatus::Persisted;
        }
        break;
    case EntityStatus::Persisted:
        if (action == EntityAction::Update) {
            return EntityStatus::Updated;
        }
        if (action == EntityAction::Delete) {
            return EntityStatus::UpdatedDeleted;
        }
        break;
    case EntityStatus::PersistedDeleted:
        if (action == EntityAction::Recover) {
            return EntityStatus::UpdatedRecover;
        }
        break;
    case EntityStatus::Updated:
        if (action == EntityAction::Update) {
            return EntityStatus::Updated;
        }
        if (action == EntityAction::Persist) {
            return EntityStatus::Persisted;
        }
        break;
    case EntityStatus::UpdatedDeleted:
        if (action == EntityAction::Delete) {
            return EntityStatus::UpdatedDeleted;
        }
        if (action == EntityAction::Persist) {
            return EntityStatus::PersistedDeleted;
        }
        break;
    case EntityStatus::UpdatedRecover:
        if (action == EntityAction::Recover) {
            return EntityStatus::UpdatedRecover;
        }
        if (action == EntityAction::Persist) {
            return EntityStatus::Persisted;
        }
        break;
    case EntityStatus::Refer:
        break;
    }
    throw std::invalid_argument("Invalid entity status transition");
}

constexpr bool need_persist(EntityStatus status) noexcept {
    return status == EntityStatus::New || status == EntityStatus::Updated ||
           status == EntityStatus::UpdatedDeleted || status == EntityStatus::UpdatedRecover;
}

constexpr bool is_new(EntityStatus status) noexcept {
    return status == EntityStatus::New;
}

constexpr bool is_updated(EntityStatus status) noexcept {
    return status == EntityStatus::Updated;
}

constexpr bool is_deleted(EntityStatus status) noexcept {
    return status == EntityStatus::UpdatedDeleted || status == EntityStatus::PersistedDeleted;
}

constexpr bool is_recover(EntityStatus status) noexcept {
    return status == EntityStatus::UpdatedRecover;
}

constexpr std::string_view to_string(EntityStatus status) noexcept {
    switch (status) {
    case EntityStatus::New:
        return "New";
    case EntityStatus::Persisted:
        return "Persisted";
    case EntityStatus::PersistedDeleted:
        return "PersistedDeleted";
    case EntityStatus::Updated:
        return "Updated";
    case EntityStatus::UpdatedDeleted:
        return "UpdatedDeleted";
    case EntityStatus::UpdatedRecover:
        return "UpdatedRecover";
    case EntityStatus::Refer:
        return "Refer";
    }
    return {};
}
}
