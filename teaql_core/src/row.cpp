#include <teaql/core/row.hpp>
#include <teaql/core/debug.hpp>
#include <teaql/core/dynamic_fields_debug.hpp>
#include <teaql/core/eval.hpp>
#include <teaql/core/list.hpp>
#include <teaql/memory/arena.hpp>
#include <algorithm>
#include <charconv>
#include <limits>

namespace teaql {
namespace {
template <class Range, class Element>
void row_debug_map(DebugWriter& writer, const Range& values, Element element) {
    writer.sequence(
        values,
        [&](auto& out, const auto& pair) {
            element(out, pair.first);
            out.raw(": ");
            element(out, pair.second);
        },
        "{",
        "}");
}

template <class T>
void row_debug_optional(DebugWriter& writer, const T* value) {
    if (value) {
        writer.tuple("Some", [&](auto& out) {
            write_debug(out, *value);
        });
    } else {
        writer.raw("None");
    }
}
}

void write_debug(DebugWriter& writer, const DynamicPropertyDefinitions& value) {
    writer.structure("DynamicPropertyDefinitions", [&](auto& fields) {
        fields.field("types", [&](auto& out) {
            row_debug_map(out, value.types(), [](auto& element, const auto& item) {
                write_debug(element, item);
            });
        });
        fields.field("hash", [&](auto& out) {
            out.integer(value.schema_hash());
        });
    });
}

void write_debug(DebugWriter& writer, const FieldLayout& value) {
    // C++ owns immutable geometry here; interning hints live in Context SnapshotPool.
    writer.structure(
        "FieldLayout",
        [&](auto& fields) {
            fields.field("entity", [&](auto& out) {
                out.text(value.entity_);
            });
            fields.field("revision", [&](auto& out) {
                out.text(value.revision_);
            });
            fields.field("names", [&](auto& out) {
                out.sequence(value.names_, [](auto& element, const auto& name) {
                    element.text(name);
                });
            });
            fields.field("aliases", [&](auto& out) {
                row_debug_map(out, value.aliases_, [](auto& element, const auto& item) {
                    write_debug(element, item);
                });
            });
            fields.field("relations", [&](auto& out) {
                out.sequence(
                    value.relations_,
                    [](auto& element, const auto& name) {
                        element.text(name);
                    },
                    "{",
                    "}");
            });
        },
        true);
}

void write_debug(DebugWriter& writer, const LoadedSnapshot& value) {
    writer.structure("LoadedSnapshot", [&](auto& fields) {
        fields.field("layout", [&](auto& out) {
            write_debug(out, value.layout());
        });
        fields.field("bits", [&](auto& out) {
            out.integer(value.bits());
        });
        fields.field("overflow", [&](auto& out) {
            if (const auto* overflow = value.overflow()) {
                out.tuple("Some", [&](auto& inner) {
                    inner.sequence(
                        *overflow,
                        [](auto& element, auto index) {
                            element.integer(index);
                        },
                        "{",
                        "}");
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("selected_names", [&](auto& out) {
            if (const auto* names = value.selected_names()) {
                out.tuple("Some", [&](auto& inner) {
                    inner.sequence(
                        *names,
                        [](auto& element, const auto& name) {
                            element.text(name);
                        },
                        "{",
                        "}");
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("dynamic_properties", [&](auto& out) {
            row_debug_optional(out, value.dynamic_property_definitions());
        });
    });
}

void write_debug(DebugWriter& writer, const ColumnLayout& value) {
    writer.structure("CompactRowLayout", [&](auto& fields) {
        fields.field("names", [&](auto& out) {
            out.sequence(value.names(), [](auto& element, const auto& name) {
                element.text(name);
            });
        });
        fields.field("relation_names", [&](auto& out) {
            out.sequence(value.relation_names(), [](auto& element, const auto& name) {
                element.text(name);
            });
        });
        fields.field("dynamic_properties", [&](auto& out) {
            row_debug_optional(out, value.dynamic_property_definitions());
        });
        fields.field("primary", [&](auto& out) {
            out.tuple("OnceLock", [&](auto& inner) {
                if (value.primary_) {
                    write_debug(inner, value.primary_->get());
                } else {
                    inner.raw("<uninit>");
                }
            });
        });
        fields.field("other_types", [&](auto& out) {
            // Source diagnostic vocabulary; actual memoized Context borrows, no synthetic lock.
            out.structure(
                "Mutex",
                [&](auto& entries) {
                    entries.field("data", [&](auto& inner) {
                        inner.sequence(
                            value.other_types_,
                            [](auto& element, const auto& pair) {
                                element.integer(pair.first);
                                element.raw(": ");
                                write_debug(element, pair.second.get());
                            },
                            "{",
                            "}");
                    });
                    entries.field("poisoned", [](auto& inner) {
                        inner.boolean(false);
                    });
                },
                true);
        });
    });
}

namespace detail {
struct LoadedRelationResults {
    using allocator_type = Allocator;
    std::pmr::map<String, SmartList<CompactRow>, std::less<>> lists;
    std::optional<ArenaBorrow<DynamicFieldValues>> dynamic_fields;

    LoadedRelationResults(std::allocator_arg_t, Allocator a) : lists(a.resource()) {}

    LoadedRelationResults(std::allocator_arg_t, Allocator a, const LoadedRelationResults& other)
        : lists(other.lists, a.resource()) {
        if (other.dynamic_fields) {
            auto& arena = dynamic_cast<Arena&>(*a.resource());
            auto& copy = arena.make<DynamicFieldValues>(other.dynamic_fields->get());
            dynamic_fields.emplace(std::allocator_arg, a, copy);
        }
    }
};
}

void write_debug(DebugWriter& writer, const CompactRow& row) {
    writer.structure("CompactRow", [&](auto& fields) {
        fields.field("columns", [&](auto& out) {
            write_debug(out, *row.shared_columns());
        });
        fields.field("values", [&](auto& out) {
            out.sequence(row.values(), [](auto& inner, const auto& value) {
                write_debug(inner, value);
            });
        });
        fields.field("loaded_relations", [&](auto& out) {
            if (!row.loaded_relations_) {
                out.raw("None");
                return;
            }
            out.tuple("Some", [&](auto& inner) {
                inner.structure("LoadedRelationResults", [&](auto& metadata) {
                    metadata.field("lists", [&](auto& list_writer) {
                        list_writer.sequence(
                            row.loaded_relations_->lists,
                            [](auto& element, const auto& pair) {
                                element.text(pair.first);
                                element.raw(": ");
                                write_debug(element, pair.second, DefaultDebugElement{});
                            },
                            "{",
                            "}");
                    });
                    metadata.field("dynamic_fields", [&](auto& dynamic_writer) {
                        row_debug_optional(dynamic_writer, row.loaded_dynamic_fields());
                    });
                });
            });
        });
    });
}

namespace {
void validate_row_size(Memory owner, std::size_t columns, std::size_t values) {
    if (columns == values) {
        return;
    }
    String message("assertion `left == right` failed: row values must match the actual "
                   "projection layout\n  left: ",
                   owner);
    auto append = [&](std::size_t value) {
        char buffer[std::numeric_limits<std::size_t>::digits10 + 1];
        auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        message.append(buffer, result.ptr);
    };
    append(columns);
    message += "\n right: ";
    append(values);
    throw std::invalid_argument(std::string(message));
}

const ColumnLayout* make_layout(Memory owner,
                                Vector<String> names,
                                std::span<const String> relations = {},
                                const DynamicPropertyDefinitions* definitions = nullptr) {
    auto* arena = dynamic_cast<Arena*>(owner);
    if (!arena) {
        throw std::invalid_argument("CompactRow layouts require an explicit Context arena");
    }
    return &arena->make_payload<ColumnLayout>(std::move(names), relations, definitions);
}

const ColumnLayout* copy_layout(Memory owner, const ColumnLayout& source) {
    auto* arena = dynamic_cast<Arena*>(owner);
    if (!arena) {
        throw std::invalid_argument("CompactRow layouts require an explicit Context arena");
    }
    return &arena->make_payload<ColumnLayout>(source);
}

Vector<String> copy_names(Memory owner, std::span<const String> source) {
    Vector<String> result(owner);
    result.reserve(source.size());
    for (const auto& name : source) {
        result.emplace_back(name);
    }
    return result;
}
}

ColumnLayout::ColumnLayout(std::allocator_arg_t, Allocator a, const ColumnLayout& other)
    : ColumnLayout(std::allocator_arg,
                   a,
                   copy_names(a.resource(), other.names()),
                   other.relation_names(),
                   other.dynamic_property_definitions()) {}

ColumnLayout::ColumnLayout(std::allocator_arg_t,
                           Allocator a,
                           Vector<String> names,
                           std::span<const String> relations,
                           const DynamicPropertyDefinitions* definitions)
    : names_(std::move(names), a.resource()),
      relations_(relations.begin(), relations.end(), a.resource()), other_types_(a.resource()) {
    if (definitions) {
        auto& arena = dynamic_cast<Arena&>(*a.resource());
        auto& origin = dynamic_cast<Arena&>(*definitions->memory_resource());
        const auto& retained = origin.can_lend_to(arena)
                                   ? *definitions
                                   : arena.make<DynamicPropertyDefinitions>(*definitions);
        dynamic_properties_.emplace(std::allocator_arg, a, retained);
    }
}

const ColumnLayout& ColumnLayout::with_dynamic_property_definitions(
    const DynamicPropertyDefinitions& definitions) const {
    if (const auto* existing = dynamic_property_definitions();
        existing && *existing == definitions) {
        return *this;
    }
    return *make_layout(
        memory_resource(), copy_names(memory_resource(), names()), relation_names(), &definitions);
}

bool ColumnLayout::operator==(const ColumnLayout& other) const {
    if (!std::ranges::equal(names(), other.names()) ||
        !std::ranges::equal(relation_names(), other.relation_names())) {
        return false;
    }
    const auto* left = dynamic_property_definitions();
    const auto* right = other.dynamic_property_definitions();
    return left && right ? *left == *right : left == right;
}

eval::LoadState ColumnLayout::load_state(SnapshotPool& pool, const FieldLayout& layout) const {
    auto owner = memory_resource();
    if (pool.memory_resource() != owner) {
        throw std::invalid_argument("Row loading requires the row's explicit Context pool");
    }
    ArenaBorrow<FieldLayout> binding(std::allocator_arg, Allocator(owner), layout);
    if (primary_) {
        try {
            if (&primary_->get().layout() == &layout) {
                return eval::LoadState::indexed(owner, *primary_);
            }
        } catch (const ReferenceError& error) {
            if (error.kind() != ReferenceError::Kind::Dead) {
                throw;
            }
            primary_.reset();
        }
    }
    for (auto it = other_types_.begin(); it != other_types_.end();) {
        try {
            (void)it->second.get();
            ++it;
        } catch (const ReferenceError& error) {
            if (error.kind() != ReferenceError::Kind::Dead) {
                throw;
            }
            it = other_types_.erase(it);
        }
    }
    if (auto found = other_types_.find(binding.identity()); found != other_types_.end()) {
        return eval::LoadState::indexed(owner, found->second);
    }
    Vector<std::string_view> selected(owner);
    selected.reserve(names_.size());
    for (const auto& name : names_) {
        selected.push_back(name);
    }
    for (const auto& name : relations_) {
        selected.push_back(name);
    }
    auto state = LoadedSnapshot::projection(owner, layout, selected).into_shared(pool);
    state = LoadedSnapshot::with_property_schema(pool, state, dynamic_property_definitions());
    if (!primary_) {
        primary_ = state;
    } else {
        other_types_.emplace(binding.identity(), state);
    }
    return eval::LoadState::indexed(owner, state);
}

eval::LoadState CompactRow::indexed_load_state(SnapshotPool& pool,
                                               const FieldLayout& layout) const {
    return shared_columns()->load_state(pool, layout);
}

void CompactRow::mark_relation_loaded(std::string_view name, RelationShapeCache& cache) {
    if (cache.memory_resource() != memory_resource()) {
        throw std::invalid_argument("Relation shapes require the result Context");
    }
    if (is_loaded_relation(name)) {
        return;
    }
    if (cache.relation_ != name) {
        cache.shapes_.clear();
        cache.relation_.assign(name);
    }
    auto* previous = shared_columns();
    auto found = cache.shapes_.find(previous);
    if (found == cache.shapes_.end()) {
        Vector<String> relations(previous->relation_names().begin(),
                                 previous->relation_names().end(),
                                 memory_resource());
        relations.emplace_back(name);
        auto* next = make_layout(memory_resource(),
                                 copy_names(memory_resource(), keys()),
                                 relations,
                                 previous->dynamic_property_definitions());
        found = cache.shapes_.emplace(previous, next).first;
    }
    shared_ = found->second;
}

void CompactRow::share_layouts(std::span<CompactRow> rows) {
    if (rows.empty()) {
        return;
    }
    auto owner = rows.front().memory_resource();
    for (const auto& row : rows) {
        if (row.memory_resource() != owner) {
            throw std::invalid_argument("Row layout sharing requires one explicit Context");
        }
    }

    struct Compare {
        bool operator()(const ColumnLayout* left, const ColumnLayout* right) const {
            if (!std::ranges::equal(left->names(), right->names())) {
                return std::lexicographical_compare(left->names().begin(),
                                                    left->names().end(),
                                                    right->names().begin(),
                                                    right->names().end());
            }
            if (!std::ranges::equal(left->relation_names(), right->relation_names())) {
                return std::lexicographical_compare(left->relation_names().begin(),
                                                    left->relation_names().end(),
                                                    right->relation_names().begin(),
                                                    right->relation_names().end());
            }
            auto* a = left->dynamic_property_definitions();
            auto* b = right->dynamic_property_definitions();
            if (!a || !b) {
                return !a && b;
            }
            return a->types() < b->types();
        }
    };

    std::pmr::set<const ColumnLayout*, Compare> shapes(owner);
    for (auto& row : rows) {
        auto [position, inserted] = shapes.insert(row.shared_columns());
        row.shared_ = *position;
    }
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a) : values_(a.resource()) {
    shared_ = make_layout(a.resource(), Vector<String>(a.resource()));
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a, const Record& record)
    : values_(a.resource()) {
    Vector<String> names(a.resource());
    names.reserve(record.size());
    values_.reserve(record.size());
    for (const auto& [name, value] : record) {
        names.emplace_back(name);
        values_.emplace_back(value);
    }
    shared_ = make_layout(a.resource(), std::move(names));
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a, Record&& record) : values_(a.resource()) {
    if (record.get_allocator().resource() != a.resource()) {
        CompactRow copy(std::allocator_arg, a, std::as_const(record));
        shared_ = copy.shared_;
        values_ = std::move(copy.values_);
    } else {
        Vector<String> names(a.resource());
        names.reserve(record.size());
        values_.reserve(record.size());
        for (const auto& [name, value] : record) {
            names.emplace_back(name);
        }
        shared_ = make_layout(a.resource(), std::move(names));
        for (auto& [name, value] : record) {
            values_.emplace_back(std::move(value));
        }
    }
    record.clear();
}

CompactRow::CompactRow(std::allocator_arg_t,
                       Allocator a,
                       const ColumnLayout& columns,
                       std::span<const Value> values)
    : values_(a.resource()) {
    validate_row_size(a.resource(), columns.names().size(), values.size());
    shared_ =
        columns.memory_resource() == a.resource() ? &columns : copy_layout(a.resource(), columns);
    values_.reserve(values.size());
    for (const auto& value : values) {
        values_.emplace_back(value);
    }
}

CompactRow::CompactRow(std::allocator_arg_t,
                       Allocator a,
                       const ColumnLayout& columns,
                       Vector<Value>&& values)
    : values_(a.resource()) {
    validate_row_size(a.resource(), columns.names().size(), values.size());
    shared_ =
        columns.memory_resource() == a.resource() ? &columns : copy_layout(a.resource(), columns);
    if (values.get_allocator().resource() == a.resource()) {
        values_ = std::move(values);
    } else {
        values_.reserve(values.size());
        for (const auto& value : values) {
            values_.emplace_back(value);
        }
    }
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a, const CompactRow& other)
    : values_(a.resource()) {
    shared_ = a.resource() == other.memory_resource()
                  ? other.shared_columns()
                  : copy_layout(a.resource(), *other.shared_columns());
    values_.reserve(other.values_.size());
    for (const auto& value : other.values_) {
        values_.emplace_back(value);
    }
    if (other.loaded_relations_) {
        loaded_relations_ =
            detail::make_box<detail::LoadedRelationResults>(a.resource(), *other.loaded_relations_);
    }
}

CompactRow::CompactRow(CompactRow&& other) noexcept
    : shared_(std::exchange(other.shared_, nullptr)), values_(std::move(other.values_)),
      loaded_relations_(std::move(other.loaded_relations_)) {
    other.values_.clear();
}

CompactRow::CompactRow(std::allocator_arg_t, Allocator a, CompactRow&& other)
    : CompactRow(detail::move_to(a.resource(), std::move(other))) {}

CompactRow::~CompactRow() = default;

bool CompactRow::operator==(const CompactRow& other) const {
    if (!std::ranges::equal(keys(), other.keys()) || values_ != other.values_ ||
        !std::ranges::equal(shared_columns()->relation_names(),
                            other.shared_columns()->relation_names())) {
        return false;
    }
    const auto* a = shared_columns()->dynamic_property_definitions();
    const auto* b = other.shared_columns()->dynamic_property_definitions();
    if (a != b && (!a || !b || *a != *b)) {
        return false;
    }
    if (bool(loaded_relations_) != bool(other.loaded_relations_)) {
        return false;
    }
    if (!loaded_relations_) {
        return true;
    }
    if (loaded_relations_->lists != other.loaded_relations_->lists) {
        return false;
    }
    const auto* left = loaded_dynamic_fields();
    const auto* right = other.loaded_dynamic_fields();
    return left == right || (left && right && *left == *right);
}

detail::LoadedRelationResults& CompactRow::read_metadata() {
    if (!loaded_relations_) {
        loaded_relations_ = detail::make_box<detail::LoadedRelationResults>(memory_resource());
    }
    return *loaded_relations_;
}

const SmartList<CompactRow>* CompactRow::loaded_relation(std::string_view name) const {
    if (!loaded_relations_) {
        return nullptr;
    }
    auto found = loaded_relations_->lists.find(name);
    return found == loaded_relations_->lists.end() ? nullptr : &found->second;
}

void CompactRow::set_loaded_relation(std::string_view name, const SmartList<CompactRow>& list) {
    read_metadata().lists.insert_or_assign(String(name, memory_resource()),
                                           detail::clone_in(memory_resource(), list));
}

std::optional<SmartList<CompactRow>> CompactRow::take_loaded_relation(std::string_view name) {
    if (!loaded_relations_) {
        return {};
    }
    auto found = loaded_relations_->lists.find(name);
    if (found == loaded_relations_->lists.end()) {
        return {};
    }
    auto result = std::move(found->second);
    loaded_relations_->lists.erase(found);
    return result;
}

bool CompactRow::has_loaded_relations() const {
    return loaded_relations_ && !loaded_relations_->lists.empty();
}

void CompactRow::clear_loaded_relations() {
    loaded_relations_.reset();
}

void CompactRow::set_loaded_dynamic_fields(const DynamicFieldValues& values) {
    auto r = memory_resource();
    auto& copy = dynamic_cast<Arena&>(*r).make<DynamicFieldValues>(values);
    read_metadata().dynamic_fields.emplace(std::allocator_arg, Allocator(r), copy);
}

const DynamicFieldValues* CompactRow::loaded_dynamic_fields() const {
    return loaded_relations_ && loaded_relations_->dynamic_fields
               ? &loaded_relations_->dynamic_fields->get()
               : nullptr;
}

std::optional<DynamicFieldValues> CompactRow::take_loaded_dynamic_fields() {
    auto* values = loaded_dynamic_fields();
    if (!values) {
        return {};
    }
    auto result = detail::clone_in(memory_resource(), *values);
    loaded_relations_->dynamic_fields.reset();
    return result;
}

Result<std::monostate, DynamicFieldError>
CompactRow::merge_loaded_dynamic_fields(const DynamicFieldValues& values,
                                        DynamicFieldMergeShapes& shapes) {
    if (loaded_relations_ && loaded_relations_->dynamic_fields) {
        auto& current = const_cast<DynamicFieldValues&>(loaded_relations_->dynamic_fields->get());
        return current.merge_loaded(values, shapes);
    }
    set_loaded_dynamic_fields(values);
    return Result<std::monostate, DynamicFieldError>::ok(memory_resource(), {});
}

Result<std::monostate, String> CompactRow::validate_dynamic_properties() const {
    const auto* definitions = shared_columns()->dynamic_property_definitions();
    if (definitions) {
        for (const auto& [name, value] : iter()) {
            auto result = definitions->validate_value(name, value);
            if (!result) {
                return Result<std::monostate, String>::err(memory_resource(),
                                                           std::move(result).error());
            }
        }
    }
    return Result<std::monostate, String>::ok(memory_resource(), {});
}

void CompactRow::attach_dynamic_property_definitions(
    std::span<CompactRow> rows, const DynamicPropertyDefinitions& definitions) {
    if (rows.empty()) {
        return;
    }
    auto r = rows.front().memory_resource();
    std::pmr::map<const ColumnLayout*, const ColumnLayout*> layouts(r);
    for (auto& row : rows) {
        if (row.memory_resource() != r) {
            throw std::invalid_argument("Schema attachment requires one Context");
        }
        const auto* previous = row.shared_columns();
        auto* existing = previous->dynamic_property_definitions();
        if (existing && *existing == definitions) {
            continue;
        }
        auto found = layouts.find(previous);
        if (found == layouts.end()) {
            auto* next = make_layout(
                r, copy_names(r, previous->names()), previous->relation_names(), &definitions);
            found = layouts.emplace(previous, next).first;
        }
        row.shared_ = found->second;
    }
}

const ColumnLayout* CompactRow::shared_columns() const {
    if (!shared_) {
        shared_ = make_layout(memory_resource(), Vector<String>(memory_resource()));
    }
    return shared_;
}

Json record_to_json_value(Memory destination, const Record& record) {
    Json result(destination);
    result.data.emplace<Json::Object>(destination);
    auto& object = std::get<Json::Object>(result.data);
    for (const auto& [key, value] : record) {
        object.emplace(String(key, destination), value.to_json_value(destination));
    }
    return result;
}

Json compact_row_to_json_value(Memory destination, const CompactRow& row) {
    Json result(destination);
    result.data.emplace<Json::Object>(destination);
    auto& object = std::get<Json::Object>(result.data);
    for (std::size_t i = 0; i < row.len(); ++i) {
        object.insert_or_assign(String(row.keys()[i], destination),
                                row.values()[i].to_json_value(destination));
    }
    return result;
}

CompactRow& CompactRow::operator=(CompactRow&& other) {
    if (this == &other) {
        return *this;
    }
    if (memory_resource() == other.memory_resource()) {
        values_ = std::move(other.values_);
        loaded_relations_ = std::move(other.loaded_relations_);
        shared_ = other.shared_;
        other.shared_ = nullptr;
        other.values_.clear();
    } else {
        CompactRow copy(std::allocator_arg, Allocator(memory_resource()), other);
        values_ = std::move(copy.values_);
        loaded_relations_ = std::move(copy.loaded_relations_);
        shared_ = copy.shared_;
    }
    return *this;
}

const Value* CompactRow::get(std::string_view name) const noexcept {
    const auto names = keys();
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i] == name) {
            return &values_[i];
        }
    }
    return nullptr;
}

Value* CompactRow::get_mut(std::string_view name) noexcept {
    return const_cast<Value*>(std::as_const(*this).get(name));
}

std::optional<Value> CompactRow::insert(std::string_view name, const Value& value) {
    Value incoming(memory_resource(), value);
    if (auto* previous = get_mut(name)) {
        Value result(memory_resource(), *previous);
        *previous = std::move(incoming);
        return result;
    }
    auto names = copy_names(memory_resource(), keys());
    names.emplace_back(name);
    values_.reserve(values_.size() + 1);
    auto* layout = make_layout(memory_resource(),
                               std::move(names),
                               shared_columns()->relation_names(),
                               shared_columns()->dynamic_property_definitions());
    // Publish only after all potentially failing preparation has completed.
    values_.push_back(std::move(incoming));
    shared_ = layout;
    return {};
}

std::optional<Value> CompactRow::remove(std::string_view name) {
    (void)take_loaded_relation(name);
    const auto names = keys();
    const auto it = std::find(names.begin(), names.end(), name);
    if (it == names.end()) {
        return {};
    }
    const auto index = std::size_t(it - names.begin());
    Value previous(memory_resource(), values_[index]);
    auto replacement = copy_names(memory_resource(), names);
    replacement.erase(replacement.begin() + index);
    auto* layout = make_layout(memory_resource(),
                               std::move(replacement),
                               shared_columns()->relation_names(),
                               shared_columns()->dynamic_property_definitions());
    values_.erase(values_.begin() + index);
    shared_ = layout;
    return previous;
}

void CompactRow::extend(const CompactRow& other) {
    if (this == &other) {
        CompactRow snapshot(std::allocator_arg, Allocator(memory_resource()), other);
        extend(snapshot);
        return;
    }
    DynamicFieldMergeShapes shapes(std::allocator_arg, Allocator(memory_resource()));
    auto result = try_extend(other, shapes);
    if (!result) {
        String message("incompatible CompactRow read metadata: ", memory_resource());
        message += debug_string(memory_resource(), result.error());
        throw std::invalid_argument(std::string(message));
    }
}

Result<std::monostate, DynamicFieldError> CompactRow::try_extend(const CompactRow& other,
                                                                 DynamicFieldMergeShapes& shapes) {
    auto r = memory_resource();
    using Output = Result<std::monostate, DynamicFieldError>;
    if (this == &other) {
        CompactRow copy(std::allocator_arg, Allocator(r), other);
        return try_extend(copy, shapes);
    }
    const auto* left = shared_columns()->dynamic_property_definitions();
    const auto* right = other.shared_columns()->dynamic_property_definitions();
    const auto* definitions = left ? left : right;
    if (left && right) {
        auto reusable = DynamicPropertyDefinitions::reusable_merge(r, *left, *right);
        if (!reusable) {
            return Output::err(
                r, DynamicFieldError(r, "DYNAMIC_PROPERTY_TYPE_MISMATCH", reusable.error()));
        }
        definitions = reusable.value();
        if (!definitions) {
            auto merged = DynamicPropertyDefinitions::merge_copy(r, *left, *right);
            if (!merged) {
                return Output::err(
                    r, DynamicFieldError(r, "DYNAMIC_PROPERTY_TYPE_MISMATCH", merged.error()));
            }
            definitions = &dynamic_cast<Arena&>(*r).make<DynamicPropertyDefinitions>(
                std::move(merged).value());
        }
    }
    if (definitions) {
        for (const auto& [name, value] : iter()) {
            if (!other.contains_key(name) && !definitions->validate_value(name, value)) {
                return Output::err(r, DynamicFieldError(r, "DYNAMIC_PROPERTY_TYPE_MISMATCH", name));
            }
        }
        for (const auto& [name, value] : other.iter()) {
            if (!definitions->validate_value(name, value)) {
                return Output::err(r, DynamicFieldError(r, "DYNAMIC_PROPERTY_TYPE_MISMATCH", name));
            }
        }
    }
    if (auto* current = loaded_dynamic_fields()) {
        if (auto* incoming = other.loaded_dynamic_fields()) {
            auto checked = current->validate_merge(*incoming);
            if (!checked) {
                return Output::err(r, std::move(checked).error());
            }
        }
    }
    const auto* layout = shared_columns();
    const bool same_schema = left == definitions || (left && definitions && *left == *definitions);
    const auto relation_names = layout->relation_names();
    const bool same_relations =
        std::ranges::all_of(other.shared_columns()->relation_names(), [&](const auto& name) {
            return std::ranges::find(relation_names, name) != relation_names.end();
        });
    if (!other.loaded_relations_ && same_schema && same_relations &&
        std::ranges::equal(keys(), other.keys())) {
        // Reuse existing geometry and value capacity after metadata validation.
        // Borrowed inputs still copy payloads into this scope; duplicate columns
        // retain the source's first-slot lookup and last-incoming-value behavior.
        for (const auto& [name, value] : other.iter()) {
            const auto position = std::ranges::find(keys(), name) - keys().begin();
            values_[std::size_t(position)] = Value(r, value);
        }
        return Output::ok(r, {});
    }
    Vector<String> names = copy_names(r, keys());
    Vector<Value> values(r);
    for (const auto& value : values_) {
        values.emplace_back(value);
    }
    for (const auto& [name, value] : other.iter()) {
        auto found = std::find(names.begin(), names.end(), name);
        if (found == names.end()) {
            names.emplace_back(name);
            values.emplace_back(value);
        } else {
            values[std::size_t(found - names.begin())] = Value(r, value);
        }
    }
    Vector<String> relations = copy_names(r, shared_columns()->relation_names());
    for (const auto& name : other.shared_columns()->relation_names()) {
        if (std::find(relations.begin(), relations.end(), name) == relations.end()) {
            relations.emplace_back(name);
        }
    }
    if (!std::ranges::equal(names, keys()) ||
        !std::ranges::equal(relations, layout->relation_names()) || !same_schema) {
        layout = make_layout(r, std::move(names), relations, definitions);
    }
    if (const auto* incoming = other.loaded_dynamic_fields()) {
        auto merged = merge_loaded_dynamic_fields(*incoming, shapes);
        if (!merged) {
            return merged;
        }
    }
    if (other.loaded_relations_) {
        (void)read_metadata();
        for (const auto& [name, list] : other.loaded_relations_->lists) {
            set_loaded_relation(name, list);
        }
    }
    shared_ = layout;
    values_ = std::move(values);
    return Output::ok(r, {});
}

Record CompactRow::copy_map(Memory target) const {
    Record result(target);
    const auto names = keys();
    // Rust BTreeMap::collect keeps the last value when projected names repeat.
    for (std::size_t i = 0; i < names.size(); ++i) {
        result.insert_or_assign(String(names[i], target), Value(target, values_[i]));
    }
    return result;
}

Record CompactRow::into_map(Memory target) && {
    if (target != memory_resource()) {
        auto result = copy_map(target);
        values_.clear();
        shared_ = nullptr;
        clear_loaded_relations();
        return result;
    }
    Record result(target);
    const auto names = keys();
    // Allocate all destination nodes/keys before consuming any source value.
    for (const auto& name : names) {
        result.try_emplace(String(name, target), Value(target));
    }
    for (std::size_t index = 0; index < names.size(); ++index) {
        result.find(names[index])->second = std::move(values_[index]);
    }
    values_.clear();
    shared_ = nullptr;
    clear_loaded_relations();
    return result;
}
}
