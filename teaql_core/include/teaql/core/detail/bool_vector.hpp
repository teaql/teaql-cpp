#pragma once
#include <teaql/memory/types.hpp>
#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace teaql::detail {
// Rust Vec<bool> exposes real Boolean references. std::vector<bool> cannot
// supply the pointer/slice contracts of SmartList, so use scoped typed storage.
class BoolVector {
public:
    using allocator_type = Allocator;
    using value_type = bool;
    using iterator = bool*;
    using const_iterator = const bool*;

    explicit BoolVector(Memory owner) : owner_(owner) {}

    BoolVector(std::allocator_arg_t, Allocator owner) : BoolVector(owner.resource()) {}

    BoolVector(std::allocator_arg_t, Allocator owner, const BoolVector& other)
        : BoolVector(owner.resource()) {
        reserve(other.size());
        for (bool value : other) {
            emplace_back(value);
        }
    }

    BoolVector(const BoolVector&) = delete;

    BoolVector(BoolVector&& other) noexcept
        : owner_(other.owner_), values_(std::exchange(other.values_, nullptr)),
          size_(std::exchange(other.size_, 0)), capacity_(std::exchange(other.capacity_, 0)) {}

    BoolVector(std::allocator_arg_t, Allocator owner, BoolVector&& other)
        : BoolVector(owner.resource()) {
        if (owner_ == other.owner_) {
            values_ = std::exchange(other.values_, nullptr);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
        } else {
            reserve(other.size());
            for (bool value : other) {
                emplace_back(value);
            }
        }
    }

    BoolVector& operator=(BoolVector&& other) {
        if (this != &other) {
            if (owner_ == other.owner_) {
                release();
                values_ = std::exchange(other.values_, nullptr);
                size_ = std::exchange(other.size_, 0);
                capacity_ = std::exchange(other.capacity_, 0);
            } else {
                BoolVector copy(std::allocator_arg, Allocator(owner_), other);
                swap(copy);
            }
        }
        return *this;
    }

    ~BoolVector() {
        release();
    }

    Allocator get_allocator() const noexcept {
        return Allocator(owner_);
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t capacity() const noexcept {
        return capacity_;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    bool* data() noexcept {
        return values_ ? values_ : &empty_value_;
    }

    const bool* data() const noexcept {
        return values_ ? values_ : &empty_value_;
    }

    iterator begin() noexcept {
        return data();
    }

    iterator end() noexcept {
        return data() + size_;
    }

    const_iterator begin() const noexcept {
        return data();
    }

    const_iterator end() const noexcept {
        return data() + size_;
    }

    bool& operator[](std::size_t index) noexcept {
        return values_[index];
    }

    const bool& operator[](std::size_t index) const noexcept {
        return values_[index];
    }

    bool& at(std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Boolean vector index");
        }
        return values_[index];
    }

    const bool& at(std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Boolean vector index");
        }
        return values_[index];
    }

    bool& back() noexcept {
        return values_[size_ - 1];
    }

    const bool& back() const noexcept {
        return values_[size_ - 1];
    }

    void reserve(std::size_t capacity) {
        if (capacity <= capacity_) {
            return;
        }
        if (capacity > max_size()) {
            throw std::length_error("Boolean vector capacity");
        }
        std::pmr::polymorphic_allocator<bool> allocator(owner_);
        auto replacement = allocator.allocate(capacity);
        std::uninitialized_copy(begin(), end(), replacement);
        release();
        values_ = replacement;
        capacity_ = capacity;
    }

    bool& emplace_back(bool value) {
        if (size_ == capacity_) {
            const auto maximum = max_size();
            if (size_ == maximum) {
                throw std::length_error("Boolean vector capacity");
            }
            reserve(capacity_ > maximum / 2 ? maximum : std::max<std::size_t>(1, capacity_ * 2));
        }
        std::construct_at(values_ + size_, value);
        return values_[size_++];
    }

    void push_back(bool value) {
        emplace_back(value);
    }

    void pop_back() noexcept {
        std::destroy_at(values_ + --size_);
    }

    void clear() noexcept {
        if (size_) {
            std::destroy_n(values_, size_);
        }
        size_ = 0;
    }

    void swap(BoolVector& other) {
        if (owner_ != other.owner_) {
            throw std::invalid_argument("Boolean vector swap requires one allocation domain");
        }
        std::swap(values_, other.values_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    bool operator==(const BoolVector& other) const noexcept {
        return size_ == other.size_ && std::equal(begin(), end(), other.begin());
    }

private:
    static constexpr std::size_t max_size() noexcept {
        return std::size_t(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(bool);
    }

    Memory owner_;
    bool* values_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
    bool empty_value_ = false;

    void release() noexcept {
        if (values_) {
            std::destroy_n(values_, size_);
            std::pmr::polymorphic_allocator<bool>(owner_).deallocate(values_, capacity_);
        }
    }
};
}
