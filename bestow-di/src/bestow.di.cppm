// bestow-di/src/bestow.di.cppm
// Lightweight .NET-style Dependency Injection framework for Bestow
//
// Usage:
//   ServiceCollection services;
//   services.addSingleton<IEventSystem, EventSystem>();  // 0-arg constructor
//   services.addSingleton<IAssetSystem, AssetSystem>(
//       [](ServiceProvider& sp) { return new AssetSystem(sp.get<IEventSystem>()); });
//   auto provider = services.build();
//   auto& events = provider.get<IEventSystem>();

export module bestow.di;

import std;

export namespace bestow::di {

//==========================================================================
// Forward declarations
//==========================================================================

class ServiceProvider;

//==========================================================================
// HasInitialize concept — auto-call initialize() during build()
//==========================================================================

template<typename T>
concept HasInitialize = requires(T& t) {
    { t.initialize() } -> std::convertible_to<bool>;
};

//==========================================================================
// ServiceEntry — type-erased service registration
//==========================================================================

struct ServiceEntry {
    // Type-erased factory: creates the instance, stores in entry
    std::function<void*(ServiceProvider&)> factory;

    // The created instance (nullptr until resolved)
    void* instance = nullptr;

    // Destructor for cleanup
    std::function<void(void*)> destructor;

    // Optional initialize callback
    std::function<bool(void*)> initializer;

    // Type info for diagnostics
    std::type_index implType;
    std::type_index contractType;

    ServiceEntry(std::type_index impl, std::type_index contract)
        : implType(impl), contractType(contract) {}
};

//==========================================================================
// ServiceProvider — frozen container, resolves services
//==========================================================================

class ServiceProvider {
public:
    ServiceProvider() = default;
    ~ServiceProvider() {
        // Destroy in reverse creation order
        for (auto it = creationOrder_.rbegin(); it != creationOrder_.rend(); ++it) {
            auto entryIt = entries_.find(*it);
            if (entryIt != entries_.end() && entryIt->second.instance && entryIt->second.destructor) {
                entryIt->second.destructor(entryIt->second.instance);
            }
        }
    }

    ServiceProvider(const ServiceProvider&) = delete;
    ServiceProvider& operator=(const ServiceProvider&) = delete;
    ServiceProvider(ServiceProvider&&) noexcept = default;
    ServiceProvider& operator=(ServiceProvider&&) noexcept = default;

    /// Get a service by contract interface. Throws if not registered.
    template<typename Contract>
    Contract& get() {
        auto it = entries_.find(std::type_index(typeid(Contract)));
        if (it == entries_.end()) {
            throw std::runtime_error(
                std::string("Service not registered: ") + typeid(Contract).name());
        }
        if (!it->second.instance) {
            resolve(it->second);
        }
        return *static_cast<Contract*>(it->second.instance);
    }

    /// Get a service by contract interface, or nullptr if not registered.
    template<typename Contract>
    Contract* tryGet() {
        auto it = entries_.find(std::type_index(typeid(Contract)));
        if (it == entries_.end()) {
            return nullptr;
        }
        if (!it->second.instance) {
            resolve(it->second);
        }
        return static_cast<Contract*>(it->second.instance);
    }

    /// Check if a service is registered for a contract.
    template<typename Contract>
    bool has() const {
        return entries_.contains(std::type_index(typeid(Contract)));
    }

    /// Non-template resolve for inject_arg
    void* resolveByType(std::type_index type) {
        auto it = entries_.find(type);
        if (it == entries_.end()) {
            throw std::runtime_error(
                std::string("Dependency not registered: ") + type.name());
        }
        if (!it->second.instance) {
            resolve(it->second);
        }
        return it->second.instance;
    }

private:
    friend class ServiceCollection;

    void resolve(ServiceEntry& entry) {
        if (entry.instance) return;  // Already resolved (handles circular deps)

        entry.instance = entry.factory(*this);
        creationOrder_.push_back(entry.contractType);
    }

    std::unordered_map<std::type_index, ServiceEntry> entries_;
    std::vector<std::type_index> creationOrder_;  // For reverse-order destruction
};

//==========================================================================
// ServiceCollection — mutable registration phase
//==========================================================================

class ServiceCollection {
public:
    /// Register a singleton with default constructor: addSingleton<IFoo, FooImpl>()
    /// For types with dependencies, use the factory overload instead.
    template<typename Contract, typename Impl>
        requires std::is_default_constructible_v<Impl>
    ServiceCollection& addSingleton() {
        static_assert(std::is_base_of_v<Contract, Impl>,
            "Implementation must inherit from Contract");

        ServiceEntry entry(std::type_index(typeid(Impl)), std::type_index(typeid(Contract)));
        entry.factory = [](ServiceProvider&) -> void* {
            return new Impl();
        };
        entry.destructor = [](void* ptr) { delete static_cast<Impl*>(ptr); };

        if constexpr (HasInitialize<Impl>) {
            entry.initializer = [](void* ptr) -> bool {
                return static_cast<Impl*>(ptr)->initialize();
            };
        }

        entries_.emplace(std::type_index(typeid(Contract)), std::move(entry));
        registrationOrder_.push_back(std::type_index(typeid(Contract)));
        return *this;
    }

    /// Register a singleton with a factory function that resolves dependencies.
    /// Example:
    ///   services.addSingleton<IAssetSystem, AssetSystem>(
    ///       [](ServiceProvider& sp) { return new AssetSystem(sp.get<IEventSystem>()); });
    template<typename Contract, typename Impl>
    ServiceCollection& addSingleton(std::function<Impl*(ServiceProvider&)> factory) {
        static_assert(std::is_base_of_v<Contract, Impl>,
            "Implementation must inherit from Contract");

        ServiceEntry entry(std::type_index(typeid(Impl)), std::type_index(typeid(Contract)));
        entry.factory = [f = std::move(factory)](ServiceProvider& sp) -> void* {
            return static_cast<Contract*>(f(sp));
        };
        entry.destructor = [](void* ptr) { delete static_cast<Impl*>(ptr); };

        if constexpr (HasInitialize<Impl>) {
            entry.initializer = [](void* ptr) -> bool {
                return static_cast<Impl*>(ptr)->initialize();
            };
        }

        entries_.emplace(std::type_index(typeid(Contract)), std::move(entry));
        registrationOrder_.push_back(std::type_index(typeid(Contract)));
        return *this;
    }

    /// Register a pre-existing instance (takes ownership).
    template<typename Contract>
    ServiceCollection& addInstance(Contract* instance) {
        ServiceEntry entry(std::type_index(typeid(Contract)), std::type_index(typeid(Contract)));
        entry.factory = [instance](ServiceProvider&) -> void* {
            return instance;
        };
        // Don't set destructor — caller owns the instance or it's stack-allocated
        entries_.emplace(std::type_index(typeid(Contract)), std::move(entry));
        registrationOrder_.push_back(std::type_index(typeid(Contract)));
        return *this;
    }

    /// Register a pre-existing instance with destructor (ServiceProvider takes ownership).
    template<typename Contract, typename Impl>
    ServiceCollection& addInstance(Impl* instance) {
        static_assert(std::is_base_of_v<Contract, Impl>,
            "Implementation must inherit from Contract");

        ServiceEntry entry(std::type_index(typeid(Impl)), std::type_index(typeid(Contract)));
        entry.factory = [instance](ServiceProvider&) -> void* {
            return static_cast<Contract*>(instance);
        };
        entry.destructor = [](void* ptr) { delete static_cast<Impl*>(ptr); };

        if constexpr (HasInitialize<Impl>) {
            entry.initializer = [](void* ptr) -> bool {
                return static_cast<Impl*>(ptr)->initialize();
            };
        }

        entries_.emplace(std::type_index(typeid(Contract)), std::move(entry));
        registrationOrder_.push_back(std::type_index(typeid(Contract)));
        return *this;
    }

    /// Build the ServiceProvider: creates all singletons and calls initialize().
    ServiceProvider build() {
        ServiceProvider provider;
        provider.entries_ = std::move(entries_);

        // Resolve all services (creates instances in dependency order)
        for (auto& typeIdx : registrationOrder_) {
            auto it = provider.entries_.find(typeIdx);
            if (it != provider.entries_.end() && !it->second.instance) {
                provider.resolve(it->second);
            }
        }

        // Call initialize() on all services that have it, in registration order
        for (auto& typeIdx : registrationOrder_) {
            auto it = provider.entries_.find(typeIdx);
            if (it != provider.entries_.end() && it->second.initializer) {
                bool ok = it->second.initializer(it->second.instance);
                if (!ok) {
                    throw std::runtime_error(
                        std::string("Service initialization failed: ") +
                        it->second.implType.name());
                }
            }
        }

        return provider;
    }

private:
    std::unordered_map<std::type_index, ServiceEntry> entries_;
    std::vector<std::type_index> registrationOrder_;
};

}  // namespace bestow::di
