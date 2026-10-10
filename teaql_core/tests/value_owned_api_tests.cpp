#include <teaql/core/value.hpp>
#include <teaql/runtime/context.hpp>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class FailingResource final : public std::pmr::memory_resource {
public:
    bool fail = false;
    std::size_t live_bytes = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (fail) {
            throw std::bad_alloc{};
        }
        auto* result = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        live_bytes += bytes;
        return result;
    }

    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        live_bytes -= bytes;
        std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

static void local_transfer() {
    GlobalContext global;
    auto user = global.make_user_context();
    auto owner = user->memory_resource();
    auto& text = user->make<String>(65536, 'x');
    auto* characters = text.data();
    auto& text_value = user->make<Value>(std::move(text));
    check(text.empty() && text_value.try_text()->data() == characters &&
              text_value.try_text()->size() == 65536,
          "same-owner String transfers its exact payload without another character allocation");
    auto& list = user->make<Value::List>();
    list.reserve(128);
    list.emplace_back("list payload long enough to allocate");
    list.emplace_back(UINT64_MAX);
    auto* elements = list.data();
    auto capacity = list.capacity();
    auto* nested_text = list.front().try_text()->data();
    auto& list_value = user->make<Value>(std::move(list));
    check(list.empty() && list_value.as_list()->data() == elements &&
              list_value.as_list()->capacity() == capacity &&
              list_value.as_list()->front().try_text()->data() == nested_text,
          "same-owner List transfers capacity, elements and nested strings");
    auto& object = user->make<Value::Object>();
    object.emplace(String("long key requiring character storage", owner),
                   Value(owner, "object payload long enough to allocate"));
    auto* node = &*object.begin();
    auto* key = object.begin()->first.data();
    auto& object_value = user->make<Value>(std::move(object));
    check(object.empty() && &*object_value.as_object()->begin() == node &&
              object_value.as_object()->begin()->first.data() == key,
          "same-owner Object transfers nodes and key storage");
    user->close();
    check(global.live_user_bytes() == 0, "close returns all transferred payload blocks");
}

static void foreign_retention_and_failure() {
    FailingResource upstream;
    {
        GlobalContext global(&upstream);
        (void)global.make<Value>(0); // Precharge the object page before payload failure injection.
        Value* retained_text = nullptr;
        Value* retained_list = nullptr;
        Value* retained_object = nullptr;
        {
            auto user = global.make_user_context();
            auto owner = user->memory_resource();
            auto& text = user->make<String>(100000, 't');
            auto& list = user->make<Value::List>();
            list.emplace_back(text);
            auto& object = user->make<Value::Object>();
            object.emplace(String("large", owner), Value(owner, text));
            auto* text_data = text.data();
            auto* list_data = list.data();
            auto* object_node = &*object.begin();
            upstream.fail = true;
            for (unsigned kind = 0; kind < 3; ++kind) {
                bool failed = false;
                try {
                    if (kind == 0) {
                        (void)global.make<Value>(std::move(text));
                    } else if (kind == 1) {
                        (void)global.make<Value>(std::move(list));
                    } else {
                        (void)global.make<Value>(std::move(object));
                    }
                } catch (const std::bad_alloc&) {
                    failed = true;
                }
                check(failed && text.size() == 100000 && text.data() == text_data &&
                          list.size() == 1 && list.data() == list_data && object.size() == 1 &&
                          &*object.begin() == object_node,
                      "failed foreign preparation preserves every source");
            }
            upstream.fail = false;
            retained_text = &global.make<Value>(std::move(text));
            retained_list = &global.make<Value>(std::move(list));
            retained_object = &global.make<Value>(std::move(object));
            check(retained_text->try_text()->data() != text_data &&
                      retained_list->as_list()->data() != list_data &&
                      &*retained_object->as_object()->begin() != object_node,
                  "foreign retention owns fresh destination storage");
            check(text.size() == 100000 && list.size() == 1 && object.size() == 1,
                  "foreign relocation retains the existing Value copy contract");
        }
        check(global.live_user_bytes() == 0 && retained_text->try_text()->size() == 100000 &&
                  retained_list->as_list()->front().try_text()->size() == 100000 &&
                  retained_object->as_object()->begin()->second.try_text()->size() == 100000,
              "retained payloads survive actual User destruction");
        auto destination = global.memory_resource();
        check(std::get<String>(retained_text->data).get_allocator().resource() == destination &&
                  retained_list->as_list()->get_allocator().resource() == destination &&
                  retained_list->as_list()->front().memory_resource() == destination &&
                  std::get<String>(retained_list->as_list()->front().data)
                          .get_allocator()
                          .resource() == destination &&
                  retained_object->as_object()->get_allocator().resource() == destination &&
                  retained_object->as_object()->begin()->first.get_allocator().resource() ==
                      destination &&
                  retained_object->as_object()->begin()->second.memory_resource() == destination &&
                  std::get<String>(retained_object->as_object()->begin()->second.data)
                          .get_allocator()
                          .resource() == destination,
              "nested containers, keys and values propagate the destination");
    }
    check(upstream.live_bytes == 0,
          "Global destruction returns successful and failed-copy storage");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        local_transfer();
        foreign_retention_and_failure();
        std::pmr::set_default_resource(previous);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
