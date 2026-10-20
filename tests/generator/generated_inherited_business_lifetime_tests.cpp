#include "../generated_inheritance_business_ids/include/cpp_inheritance_business_ids.hpp"
#include <cstdlib>
#include <iostream>

using namespace cpp_inheritance_business_ids;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

template <class Model, class ExpressionFactory>
static void check_model(ExpressionFactory expression_factory) {
    static_assert(
        std::same_as<decltype(std::declval<const Model&>().documentNumber()), DocumentNumber>);
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& model = user_context->make<Model>();
    model.updateName("inherited owned model");
    auto& retained_number = [&]() -> DocumentNumber& {
        auto value = DocumentNumber::create(r, "DOC-20231114-84WAXC");
        check(bool(value), "create the shared identifier type");
        model.updateDocumentNumber(value.value());
        return global.make<DocumentNumber>(value.value());
    }();
    auto borrowed = expression_factory(model);
    check(borrowed.getDocumentNumber().unwrap().asStr() == retained_number.asStr(),
          "inherited E exposes the shared Business-ID type before close");
    auto& retained_model = global.make<Model>(model);
    user_context.reset();
    bool expired = false;
    try {
        (void)borrowed.getDocumentNumber().unwrap();
    } catch (const ReferenceError&) {
        expired = true;
    }
    check(expired, "inherited Business-ID E access rejects an actually deleted UserContext");
    check(retained_number.asStr() == "DOC-20231114-84WAXC" &&
              retained_number.memory_resource() == global.memory_resource(),
          "explicit typed Global copy survives User destruction");
    auto value = retained_model.documentNumber();
    check(value == retained_number && value.memory_resource() == global.memory_resource() &&
              retained_model.name().get_allocator().resource() == global.memory_resource(),
          "inherited model copy propagates Global allocation into its identifier and strings");
}

static void inherited_runtime_startup() {
    static_assert(BUSINESS_ID_KEY_HEX_ENV == "TEAQL_BUSINESS_ID_KEY_V1_HEX");
    static_assert(BUSINESS_ID_KEY_VERSION_ENV == "TEAQL_BUSINESS_ID_KEY_VERSION");
    for (unsigned entry = 0; entry != 3; ++entry) {
        for (bool supply_key : {false, true}) {
            GlobalContext global;
            auto user_context_owner = global.make_user_context();
            auto& user_context = *user_context_owner;
            {
                if (supply_key) {
                    check(
                        ::setenv(BUSINESS_ID_KEY_HEX_ENV.data(),
                                 "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
                                 1) == 0,
                        "Supply the inherited domain's source key");
                } else {
                    check(::unsetenv(BUSINESS_ID_KEY_HEX_ENV.data()) == 0,
                          "Remove the inherited domain's source key");
                }
                check(::setenv(BUSINESS_ID_KEY_VERSION_ENV.data(), "1", 1) == 0 &&
                          ::setenv(DATABASE_URL_ENV.data(), ":memory:", 1) == 0,
                      "Control only this process's inherited startup environment");
                auto result = [&]() {
                    if (entry == 0) {
                        return serviceRuntimeFromConnection(
                            user_context,
                            user_context.make_resource_ref<SqliteConnection>(":memory:"));
                    }
                    if (entry == 1) {
                        ServiceRuntimeConfig config(std::allocator_arg,
                                                    Allocator(user_context.memory_resource()),
                                                    ":memory:");
                        return serviceRuntime(user_context, config);
                    }
                    return serviceRuntimeFromEnv(user_context);
                }();
                auto executor =
                    user_context
                        .require_resource<cpp_inheritance_business_ids::DataServiceExecutor>();
                check(
                    bool(result) == supply_key && bool(executor) == supply_key,
                    "All inherited startup entries condition executor registration on a valid key");
                check(bool(user_context.business_ids()) && user_context.entity("Document"),
                      "Inherited startup retains its modeled Business-ID infrastructure");
                if (result) {
                    check(&executor.value().get() == &user_context.get(result.value()),
                          "Inherited startup returns the registered checked executor");
                } else {
                    check(result.error().kind() == ServiceRuntimeError::Kind::MissingEnv,
                          "Inherited missing keys retain the typed source failure");
                }
            }
            user_context_owner.reset();
            check(global.live_user_bytes() == 0,
                  "Successful and failed inherited startup release every User allocation");
        }
    }
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        check_model<Document>([](const auto& model) {
            return E::document(model);
        });
        check_model<IntermediateRecord>([](const auto& model) {
            return E::intermediateRecord(model);
        });
        check_model<Invoice>([](const auto& model) {
            return E::invoice(model);
        });
        check_model<Receipt>([](const auto& model) {
            return E::receipt(model);
        });
        check_model<SpecialInvoice>([](const auto& model) {
            return E::specialInvoice(model);
        });
        inherited_runtime_startup();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Five inherited Business-ID ownership and six startup contracts passed\n";
}
