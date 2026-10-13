#include <teaql/core/load_state.hpp>
#include <stdexcept>

namespace teaql {
Result<FieldLayout, String>
FieldLayout::from_generated(Memory owner,
                            std::string_view entity,
                            std::string_view revision,
                            std::span<const Index> indexes,
                            std::span<const Mapping> mappings,
                            std::span<const std::string_view> relations,
                            std::span<const std::string_view> expected_members) {
    using Output = Result<FieldLayout, String>;
    try {
        return Output::ok(
            owner,
            FieldLayout(owner, entity, revision, indexes, mappings, relations, expected_members));
    } catch (const std::invalid_argument& error) {
        return Output::err(owner, String(error.what(), owner));
    }
}

FieldLayout::FieldLayout(Memory owner,
                         std::string_view entity,
                         std::string_view revision,
                         std::span<const Index> indexes,
                         std::span<const Mapping> mappings,
                         std::span<const std::string_view> relations,
                         std::span<const std::string_view> expected_members)
    : entity_(entity, owner), revision_(revision, owner), names_(owner), aliases_(owner),
      relations_(owner) {
    if (entity.empty() || revision.empty()) {
        throw std::invalid_argument("missing entity or field-layout revision");
    }
    names_.resize(indexes.size());
    for (const auto& [name, position] : indexes) {
        if (name.empty() || name.front() == '#' || name.front() == '_' ||
            position >= names_.size()) {
            String message("invalid fixed field/index: ", owner);
            message += name;
            message += '=';
            message += std::to_string(position);
            throw std::invalid_argument(std::string(message));
        }
        if (!names_[position].empty() || !aliases_.emplace(String(name, owner), position).second) {
            String message("duplicate fixed field/index: ", owner);
            message += name;
            message += '=';
            message += std::to_string(position);
            throw std::invalid_argument(std::string(message));
        }
        names_[position] = name;
    }
    if (!aliases_.contains("id") || !aliases_.contains("version")) {
        throw std::invalid_argument("generated layout must include id and version");
    }
    std::pmr::set<String, std::less<>> mapped(owner);
    for (const auto& [canonical, member, column] : mappings) {
        auto found = aliases_.find(canonical);
        if (found == aliases_.end()) {
            throw std::invalid_argument(std::string("mapping has no fixed field: ") +
                                        std::string(canonical));
        }
        auto position = found->second;
        if (names_[position] != canonical) {
            throw std::invalid_argument(
                std::string("mapping names an alias, not a canonical field: ") +
                std::string(canonical));
        }
        if (!mapped.emplace(canonical).second) {
            throw std::invalid_argument(std::string("duplicate field mapping: ") +
                                        std::string(canonical));
        }
        for (auto alias : {member, column}) {
            if (alias.empty()) {
                throw std::invalid_argument(std::string("blank alias for ") +
                                            std::string(canonical));
            }
            auto prior = aliases_.find(alias);
            if (prior != aliases_.end() && prior->second != position) {
                throw std::invalid_argument(std::string("ambiguous fixed-field alias: ") +
                                            std::string(alias));
            }
            aliases_.insert_or_assign(String(alias, owner), position);
        }
    }
    if (mapped.size() != indexes.size()) {
        throw std::invalid_argument("incomplete fixed-field mappings");
    }
    std::pmr::set<std::size_t> expected(owner);
    for (auto member : expected_members) {
        auto found = aliases_.find(member);
        if (found == aliases_.end()) {
            throw std::invalid_argument("generated layout disagrees with typed entity fields");
        }
        expected.insert(found->second);
    }
    if (expected.size() != indexes.size() || expected_members.size() != indexes.size()) {
        throw std::invalid_argument("generated layout disagrees with typed entity fields");
    }
    for (auto relation : relations) {
        relations_.emplace(relation);
    }
}

FieldLayout::FieldLayout(std::allocator_arg_t, Allocator a, const FieldLayout& other)
    : entity_(other.entity_, a.resource()), revision_(other.revision_, a.resource()),
      names_(other.names_, a.resource()), aliases_(other.aliases_, a.resource()),
      relations_(other.relations_, a.resource()) {}

std::optional<std::size_t> FieldLayout::index(std::string_view name) const {
    auto found = aliases_.find(name);
    return found == aliases_.end() ? std::nullopt : std::optional(found->second);
}

bool FieldLayout::is_relation(std::string_view name) const {
    return relations_.contains(name);
}
}

namespace teaql {
namespace {
Arena& snapshot_arena(Memory resource) {
    auto* arena = dynamic_cast<Arena*>(resource);
    if (!arena) {
        throw std::invalid_argument("Loaded snapshots require a Context arena");
    }
    return *arena;
}

template <class T>
ArenaBorrow<T> copied_metadata(Memory destination, const ArenaBorrow<T>& source) {
    const auto& value = source.get();
    auto& target = snapshot_arena(destination);
    auto& origin = snapshot_arena(value.memory_resource());
    if (origin.can_lend_to(target)) {
        return ArenaBorrow<T>(std::allocator_arg, Allocator(destination), value);
    }
    auto& copy = target.make<T>(value);
    return ArenaBorrow<T>(std::allocator_arg, Allocator(destination), copy);
}
}

LoadedSnapshot::LoadedSnapshot(Memory r, const FieldLayout& layout)
    : owner_(r), layout_(std::allocator_arg, Allocator(r), layout) {}

LoadedSnapshot::LoadedSnapshot(std::allocator_arg_t, Allocator a, const LoadedSnapshot& source)
    : owner_(a.resource()), layout_(copied_metadata(owner_, source.layout_)), bits_(source.bits_) {
    if (source.overflow_) {
        overflow_.emplace(*source.overflow_, owner_);
    }
    if (source.selected_names_) {
        selected_names_.emplace(*source.selected_names_, owner_);
    }
    if (source.shared_overflow_) {
        shared_overflow_ = copied_metadata(owner_, *source.shared_overflow_);
    }
    if (source.shared_names_) {
        shared_names_ = copied_metadata(owner_, *source.shared_names_);
    }
    if (source.dynamic_properties_) {
        dynamic_properties_ = copied_metadata(owner_, *source.dynamic_properties_);
    }
}

LoadedSnapshot LoadedSnapshot::fully_loaded(Memory r, const FieldLayout& layout) {
    LoadedSnapshot result(r, layout);
    auto count = layout.field_count();
    result.bits_ = count >= 64 ? UINT64_MAX : (std::uint64_t{1} << count) - 1;
    if (count > 64) {
        result.overflow_.emplace(r);
        for (std::size_t index = 64; index < count; ++index) {
            result.overflow_->insert(index);
        }
    }
    return result;
}

LoadedSnapshot LoadedSnapshot::projection(Memory r,
                                          const FieldLayout& layout,
                                          std::span<const std::string_view> fields) {
    LoadedSnapshot result(r, layout);
    for (auto field : fields) {
        result.mark(field, true);
    }
    return result;
}

const LoadedSnapshot::Overflow* LoadedSnapshot::overflow() const {
    return shared_overflow_ ? &shared_overflow_->get().fields : overflow_ ? &*overflow_ : nullptr;
}

const LoadedSnapshot::SelectedNames* LoadedSnapshot::selected_names() const {
    return shared_names_     ? &shared_names_->get().fields
           : selected_names_ ? &*selected_names_
                             : nullptr;
}

bool LoadedSnapshot::is_loaded(std::string_view field) const {
    const auto& type = layout_.get();
    if (type.is_relation(field) || field.starts_with('#')) {
        auto* names = selected_names();
        return names && names->contains(field);
    }
    auto index = type.index(field);
    if (!index) {
        return false;
    }
    return *index < 64 ? (bits_ & (std::uint64_t{1} << *index)) != 0
                       : overflow() && overflow()->contains(*index);
}

void LoadedSnapshot::mark(std::string_view field, bool loaded) {
    const auto& type = layout_.get();
    if (type.is_relation(field) || field.starts_with('#')) {
        if (is_loaded(field) == loaded) {
            return;
        }
        if (shared_names_) {
            selected_names_.emplace(shared_names_->get().fields, owner_);
            shared_names_.reset();
        }
        if (!selected_names_) {
            selected_names_.emplace(owner_);
        }
        if (loaded) {
            selected_names_->emplace(field);
        } else {
            auto item = selected_names_->find(field);
            if (item != selected_names_->end()) {
                selected_names_->erase(item);
            }
        }
        if (selected_names_->empty()) {
            selected_names_.reset();
        }
    } else if (auto index = type.index(field)) {
        if (*index < 64) {
            if (loaded) {
                bits_ |= std::uint64_t{1} << *index;
            } else {
                bits_ &= ~(std::uint64_t{1} << *index);
            }
        } else {
            if (is_loaded(field) == loaded) {
                return;
            }
            if (shared_overflow_) {
                overflow_.emplace(shared_overflow_->get().fields, owner_);
                shared_overflow_.reset();
            }
            if (!overflow_) {
                overflow_.emplace(owner_);
            }
            if (loaded) {
                overflow_->insert(*index);
            } else {
                overflow_->erase(*index);
            }
            if (overflow_->empty()) {
                overflow_.reset();
            }
        }
    }
}

bool LoadedSnapshot::operator==(const LoadedSnapshot& other) const {
    auto* definitions = dynamic_property_definitions();
    auto* compared = other.dynamic_property_definitions();
    auto* overflow_left = overflow();
    auto* overflow_right = other.overflow();
    auto* names_left = selected_names();
    auto* names_right = other.selected_names();
    return layout_ == other.layout_ && bits_ == other.bits_ &&
           (overflow_left == overflow_right ||
            (overflow_left && overflow_right && *overflow_left == *overflow_right)) &&
           (names_left == names_right ||
            (names_left && names_right && *names_left == *names_right)) &&
           (definitions == compared || (definitions && compared && *definitions == *compared));
}

std::size_t LoadedSnapshot::fingerprint() const {
    std::size_t hash = std::hash<std::uint64_t>{}(bits_);
    std::size_t overflow_hash = 0, names_hash = 0;
    if (auto* fields = overflow()) {
        for (auto index : *fields) {
            overflow_hash += std::hash<std::size_t>{}(index);
        }
    }
    if (auto* fields = selected_names()) {
        for (const auto& name : *fields) {
            names_hash += std::hash<std::string_view>{}(name);
        }
    }
    hash ^= overflow_hash + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    hash ^= names_hash + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    if (auto* definitions = dynamic_property_definitions()) {
        hash ^= definitions->fingerprint() + 0x9e3779b9u + (hash << 6) + (hash >> 2);
    }
    return hash;
}

LoadedSnapshot::Borrow LoadedSnapshot::into_shared(SnapshotPool& pool) && {
    return pool.publish(std::move(*this));
}

Result<LoadedSnapshot::Borrow, String> LoadedSnapshot::with_loaded(SnapshotPool& pool,
                                                                   const Borrow& source,
                                                                   std::string_view field,
                                                                   bool loaded) {
    auto r = pool.memory_resource();
    using Output = Result<Borrow, String>;
    if (source.memory_resource() != r) {
        throw std::invalid_argument(
            "Loaded-state composition requires an explicit destination copy");
    }
    const auto& state = source.get();
    if (!state.layout().index(field) && !state.layout().is_relation(field) &&
        !field.starts_with('#')) {
        String error("unknown loadable field: ", r);
        error += state.layout().entity();
        error += '.';
        error += field;
        return Output::err(r, std::move(error));
    }
    if (state.is_loaded(field) == loaded) {
        return Output::ok(r, source);
    }
    auto next = detail::clone_in(r, state);
    next.mark(field, loaded);
    return Output::ok(r, pool.publish(std::move(next)));
}

LoadedSnapshot::Borrow LoadedSnapshot::with_dynamic_property_definitions(
    SnapshotPool& pool, const Borrow& source, const DynamicPropertyDefinitions& definitions) {
    return with_property_schema(pool, source, &definitions);
}

LoadedSnapshot::Borrow LoadedSnapshot::with_property_schema(
    SnapshotPool& pool, const Borrow& source, const DynamicPropertyDefinitions* definitions) {
    auto r = pool.memory_resource();
    if (source.memory_resource() != r) {
        throw std::invalid_argument(
            "Loaded-state composition requires an explicit destination copy");
    }
    auto* previous = source.get().dynamic_property_definitions();
    if (previous == definitions || (previous && definitions && *previous == *definitions)) {
        return source;
    }
    auto next = detail::clone_in(r, source.get());
    next.dynamic_properties_.reset();
    if (definitions) {
        ArenaBorrow<DynamicPropertyDefinitions> binding(
            std::allocator_arg, Allocator(r), *definitions);
        next.dynamic_properties_ = copied_metadata(r, binding);
    }
    return pool.publish(std::move(next));
}

Result<LoadedSnapshot::Borrow, String> LoadedSnapshot::with_dynamic_fields(
    SnapshotPool& pool, const Borrow& source, const DynamicFieldValues& values) {
    auto r = pool.memory_resource();
    using Output = Result<Borrow, String>;
    if (source.memory_resource() != r) {
        throw std::invalid_argument(
            "Loaded-state composition requires an explicit destination copy");
    }
    const auto& state = source.get();
    if (values.definitions().owner_type() != state.layout().entity()) {
        return Output::err(r, String("dynamic field owner disagrees with fixed entity layout", r));
    }
    const auto& selected = values.selected_codes();
    const auto* existing = state.selected_names();
    std::size_t count = 0;
    bool same = true;
    if (existing) {
        for (const auto& name : *existing) {
            if (name.starts_with('#')) {
                ++count;
                same = same && selected.contains(std::string_view(name).substr(1));
            }
        }
    }
    if (same && count == selected.size()) {
        return Output::ok(r, source);
    }
    SelectedNames names(r);
    if (existing) {
        for (const auto& name : *existing) {
            if (!name.starts_with('#')) {
                names.emplace(name);
            }
        }
    }
    for (const auto& code : selected) {
        String name("#", r);
        name += code;
        names.emplace(std::move(name));
    }
    auto next = detail::clone_in(r, state);
    next.shared_names_.reset();
    next.selected_names_.reset();
    if (!names.empty()) {
        next.selected_names_.emplace(std::move(names));
    }
    return Output::ok(r, pool.publish(std::move(next)));
}

SnapshotPool::SnapshotPool(std::allocator_arg_t, Allocator a)
    : arena_(&snapshot_arena(a.resource())), hints_(a.resource()) {}

LoadedSnapshot::Borrow SnapshotPool::publish(LoadedSnapshot state) {
    auto r = memory_resource();
    if (state.memory_resource() != r) {
        throw std::invalid_argument(
            "Loaded-state publication requires an explicit destination copy");
    }
    auto layout_key = state.layout_.identity();
    auto key = state.fingerprint();
    auto layout = hints_.find(layout_key);
    if (layout != hints_.end()) {
        auto bucket = layout->second.find(key);
        if (bucket != layout->second.end()) {
            for (const auto& candidate : bucket->second) {
                try {
                    if (candidate.get() == state) {
                        return candidate;
                    }
                } catch (const ReferenceError& error) {
                    if (error.kind() != ReferenceError::Kind::Dead) {
                        throw;
                    }
                }
            }
        }
    }
    std::size_t retained = 0;
    if (layout != hints_.end()) {
        for (auto bucket = layout->second.begin(); bucket != layout->second.end();) {
            auto& entries = bucket->second;
            std::erase_if(entries, [](const auto& entry) {
                try {
                    (void)entry.get();
                    return false;
                } catch (const ReferenceError& error) {
                    if (error.kind() != ReferenceError::Kind::Dead) {
                        throw;
                    }
                    return true;
                }
            });
            retained += entries.size();
            if (entries.empty()) {
                bucket = layout->second.erase(bucket);
            } else {
                ++bucket;
            }
        }
    }
    // Freeze only new auxiliary payloads. Same-scope snapshot clones borrow these
    // immutable nodes and detach just the component that is actually modified.
    if (state.overflow_) {
        auto& node = arena_->make<LoadedSnapshot::OverflowNode>(std::move(*state.overflow_));
        state.shared_overflow_.emplace(std::allocator_arg, Allocator(r), node);
        state.overflow_.reset();
    }
    if (state.selected_names_) {
        auto& node = arena_->make<LoadedSnapshot::NamesNode>(std::move(*state.selected_names_));
        state.shared_names_.emplace(std::allocator_arg, Allocator(r), node);
        state.selected_names_.reset();
    }
    auto& stored = arena_->make<LoadedSnapshot>(std::move(state));
    LoadedSnapshot::Borrow result(std::allocator_arg, Allocator(r), stored);
    if (retained < max_shared_state_hints) {
        hints_[layout_key][key].push_back(result);
    }
    return result;
}

LoadedSnapshot::Borrow SnapshotPool::copy(const LoadedSnapshot& source) {
    return publish(detail::clone_in(memory_resource(), source));
}

std::size_t SnapshotPool::hint_count(const FieldLayout& layout) const {
    ArenaBorrow<FieldLayout> binding(std::allocator_arg, Allocator(memory_resource()), layout);
    auto found = hints_.find(binding.identity());
    std::size_t count = 0;
    if (found != hints_.end()) {
        for (const auto& [hash, entries] : found->second) {
            count += entries.size();
        }
    }
    return count;
}
}
