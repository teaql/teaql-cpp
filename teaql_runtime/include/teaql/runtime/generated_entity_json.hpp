#pragma once
#include <teaql/runtime/entity_runtime.hpp>
#include <functional>

namespace teaql {
// Shared presentation traversal; generated code supplies field/relation mappings only.
class GeneratedEntityJson {
public:
    GeneratedEntityJson(Memory r,
                        EntityJsonTraversal& traversal,
                        std::string_view entity,
                        const void* address,
                        const EntityRuntimeState& root,
                        std::uint64_t id)
        : resource_(r), traversal_(traversal), entity_(entity), root_(root), id_(id),
          expand_(traversal.enter(entity, reinterpret_cast<std::uintptr_t>(address))), values_(r) {}

    GeneratedEntityJson(const GeneratedEntityJson&) = delete;

    ~GeneratedEntityJson() {
        traversal_.leave();
    }

    template <class Encode>
    void field(std::string_view name, Encode&& encode, bool loaded, bool identity) {
        if (loaded && (expand_ || identity)) {
            values_.insert_or_assign(String(name, resource_),
                                     std::invoke(encode).to_json_value(resource_));
        }
    }

    void dynamic(const Record& values) {
        if (expand_) {
            for (const auto& [name, value] : values) {
                values_.insert_or_assign(String(name, resource_), value.to_json_value(resource_));
            }
        }
    }

    template <Entity T>
    void forward(std::string_view name, const T* embedded, bool loaded, const Value& local) {
        if (!expand_ || !valid_) {
            return;
        }
        if (embedded) {
            child(name, *embedded);
            return;
        }
        if (!loaded) {
            return;
        }
        const T* target = nullptr;
        if (const auto* edge = root_.resolve_relation_option<T>(entity_, id_, name)) {
            target = *edge ? &**edge : nullptr;
        } else if (auto id = local.try_u64()) {
            target = root_.resolve_entity<T>(*id);
        }
        if (target) {
            child(name, *target);
        } else if (std::holds_alternative<std::monostate>(local.data)) {
            values_.insert_or_assign(String(name, resource_), Json(resource_));
        }
    }

    template <Entity T>
    void reverse_one(std::string_view name, bool loaded) {
        if (expand_ && valid_ && loaded) {
            if (const auto* edge = root_.resolve_relation_option<T>(entity_, id_, name)) {
                if (*edge) {
                    child(name, **edge);
                } else {
                    values_.insert_or_assign(String(name, resource_), Json(resource_));
                }
            }
        }
    }

    template <Entity T>
    void reverse_many(std::string_view name, bool loaded) {
        if (expand_ && valid_ && loaded) {
            if (const auto* list = root_.resolve_relation_list<T>(entity_, id_, name)) {
                Json::Array values(resource_);
                values.reserve(list->data.size());
                for (const auto& item : list->data) {
                    auto json = item.borrowed_json(resource_, traversal_);
                    if (!json) {
                        valid_ = false;
                        return;
                    }
                    values.emplace_back(std::move(*json));
                }
                values_.insert_or_assign(String(name, resource_), Json(resource_, values));
            }
        }
    }

    std::optional<Json> finish(const DynamicFieldValues* fields) {
        if (!valid_) {
            return std::nullopt;
        }
        for (std::string_view name :
             {"_comment", "_dirty_fields", "_original_values", "_is_new", "_is_deleted"}) {
            auto found = values_.find(name);
            if (found != values_.end()) {
                values_.erase(found);
            }
        }
        if (expand_ && fields) {
            for (const auto& [code, value] : fields->values()) {
                String name("#", resource_);
                name += code;
                values_.insert_or_assign(std::move(name), value.to_json_value(resource_));
            }
        }
        return Json(resource_, values_);
    }

private:
    template <Entity T>
    void child(std::string_view name, const T& value) {
        auto json = value.borrowed_json(resource_, traversal_);
        if (!json) {
            valid_ = false;
            return;
        }
        values_.insert_or_assign(String(name, resource_), std::move(*json));
    }

    Memory resource_;
    EntityJsonTraversal& traversal_;
    std::string_view entity_;
    const EntityRuntimeState& root_;
    std::uint64_t id_;
    bool expand_;
    bool valid_ = true;
    Json::Object values_;
};
}
