// bestow-contract/src/bestow.services.cppm
// Central module for all abstract Kangaru service definitions
// Clients import this module to get abstract services for dependency injection.
// Implementation modules import this and provide concrete overrides.

module;

#include <kangaru/kangaru.hpp>

//==========================================================================
// Service Definition Helper Macros
//
// Two patterns are available:
//
// PATTERN 1: BESTOW_SYSTEM - Nested Service class (PREFERRED)
// ============================================================
// Place inside your class to generate a nested Service type, constructor,
// and dependency member variables. Access service via MyClass::Service.
//
//   class AssetSystem : public IAssetSystem {
//       BESTOW_SYSTEM(AssetSystem, AssetSystem, EventSystem)
//   public:
//       void doWork() { dep1_->emit(...); }  // dep1_ is IEventSystem*
//   };
//   // Register: container.service<AssetSystem::Service>()
//
// PATTERN 2: BESTOW_SERVICE - Standalone service struct
// ======================================================
// Place after your class definition for a standalone service struct.
//
//   class AssetSystem : public IAssetSystem { ... };
//   BESTOW_SERVICE(AssetSystem, AssetSystem, EventSystem);
//
// Both patterns support 0-4 dependencies and auto-select based on count.
//
// Naming conventions:
//   - Interface: I##InterfaceType##Service (e.g., IAssetSystemService)
//   - Dependencies: dep1_, dep2_, dep3_, dep4_ (injected as I##DepType*)
//==========================================================================

// Helper macros for argument counting (supports 0-4 dependencies)
#define BESTOW_ARG_N(_0, _1, _2, _3, _4, N, ...) N
#define BESTOW_NARGS(...) BESTOW_ARG_N(__VA_ARGS__ __VA_OPT__(,) 4, 3, 2, 1, 0)
#define BESTOW_CONCAT_IMPL(a, b) a##b
#define BESTOW_CONCAT(a, b) BESTOW_CONCAT_IMPL(a, b)

// Internal service macros (numbered versions)
#define BESTOW_SERVICE_0(ImplType, InterfaceType) \
    struct ImplType##Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> {}

#define BESTOW_SERVICE_1(ImplType, InterfaceType, Dep1Type) \
    struct ImplType##Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct(kgr::inject_t<bestow::I##Dep1Type##Service> d1) \
            -> kgr::inject_result<bestow::I##Dep1Type*> { \
            return kgr::inject(&d1.service()); \
        } \
    }

#define BESTOW_SERVICE_2(ImplType, InterfaceType, Dep1Type, Dep2Type) \
    struct ImplType##Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::I##Dep1Type##Service> d1, \
            kgr::inject_t<bestow::I##Dep2Type##Service> d2) \
            -> kgr::inject_result<bestow::I##Dep1Type*, bestow::I##Dep2Type*> { \
            return kgr::inject(&d1.service(), &d2.service()); \
        } \
    }

#define BESTOW_SERVICE_3(ImplType, InterfaceType, Dep1Type, Dep2Type, Dep3Type) \
    struct ImplType##Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::I##Dep1Type##Service> d1, \
            kgr::inject_t<bestow::I##Dep2Type##Service> d2, \
            kgr::inject_t<bestow::I##Dep3Type##Service> d3) \
            -> kgr::inject_result<bestow::I##Dep1Type*, bestow::I##Dep2Type*, bestow::I##Dep3Type*> { \
            return kgr::inject(&d1.service(), &d2.service(), &d3.service()); \
        } \
    }

#define BESTOW_SERVICE_4(ImplType, InterfaceType, Dep1Type, Dep2Type, Dep3Type, Dep4Type) \
    struct ImplType##Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::I##Dep1Type##Service> d1, \
            kgr::inject_t<bestow::I##Dep2Type##Service> d2, \
            kgr::inject_t<bestow::I##Dep3Type##Service> d3, \
            kgr::inject_t<bestow::I##Dep4Type##Service> d4) \
            -> kgr::inject_result<bestow::I##Dep1Type*, bestow::I##Dep2Type*, bestow::I##Dep3Type*, bestow::I##Dep4Type*> { \
            return kgr::inject(&d1.service(), &d2.service(), &d3.service(), &d4.service()); \
        } \
    }

// Dispatch helpers - extract just the dependency arguments (skip first 2)
#define BESTOW_DEPS_0(ImplType, InterfaceType) BESTOW_SERVICE_0(ImplType, InterfaceType)
#define BESTOW_DEPS_1(ImplType, InterfaceType, D1) BESTOW_SERVICE_1(ImplType, InterfaceType, D1)
#define BESTOW_DEPS_2(ImplType, InterfaceType, D1, D2) BESTOW_SERVICE_2(ImplType, InterfaceType, D1, D2)
#define BESTOW_DEPS_3(ImplType, InterfaceType, D1, D2, D3) BESTOW_SERVICE_3(ImplType, InterfaceType, D1, D2, D3)
#define BESTOW_DEPS_4(ImplType, InterfaceType, D1, D2, D3, D4) BESTOW_SERVICE_4(ImplType, InterfaceType, D1, D2, D3, D4)

// Count only the dependency arguments (total args minus 2 for ImplType and InterfaceType)
#define BESTOW_DEP_COUNT_IMPL(_1, _2, _3, _4, _5, _6, N, ...) N
#define BESTOW_DEP_COUNT(...) BESTOW_DEP_COUNT_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0, 0)

// Primary variadic macro - automatically dispatches based on dependency count
// Usage: BESTOW_SERVICE(ImplType, InterfaceType [, Dep1, Dep2, ...])
#define BESTOW_SERVICE(ImplType, InterfaceType, ...) \
    BESTOW_CONCAT(BESTOW_DEPS_, BESTOW_DEP_COUNT(__VA_ARGS__ __VA_OPT__(,) _))(ImplType, InterfaceType __VA_OPT__(,) __VA_ARGS__)

//==========================================================================
// BESTOW_SYSTEM - Nested Service class pattern
//
// Place inside your class definition to generate:
//   - A nested 'Service' struct for Kangaru registration
//   - Constructor with dependency injection
//   - Private member variables (dep1_, dep2_, etc.)
//
// The class type is available within the nested Service because C++ allows
// the enclosing class name to be used in nested class definitions.
//==========================================================================

// 0 dependencies - default constructible
#define BESTOW_SYSTEM_0(ImplType, InterfaceType) \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> {}; \
    ImplType() = default; \
public:

// 1 dependency
#define BESTOW_SYSTEM_1(ImplType, InterfaceType, Dep1Type) \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct(kgr::inject_t<bestow::I##Dep1Type##Service> d1) \
            -> kgr::inject_result<bestow::I##Dep1Type*> { \
            return kgr::inject(&d1.service()); \
        } \
    }; \
    explicit ImplType(bestow::I##Dep1Type* dep1 = nullptr) : dep1_(dep1) {} \
protected: \
    bestow::I##Dep1Type* dep1_ = nullptr; \
public:

// 2 dependencies
#define BESTOW_SYSTEM_2(ImplType, InterfaceType, Dep1Type, Dep2Type) \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::I##Dep1Type##Service> d1, \
            kgr::inject_t<bestow::I##Dep2Type##Service> d2) \
            -> kgr::inject_result<bestow::I##Dep1Type*, bestow::I##Dep2Type*> { \
            return kgr::inject(&d1.service(), &d2.service()); \
        } \
    }; \
    explicit ImplType(bestow::I##Dep1Type* dep1 = nullptr, bestow::I##Dep2Type* dep2 = nullptr) \
        : dep1_(dep1), dep2_(dep2) {} \
protected: \
    bestow::I##Dep1Type* dep1_ = nullptr; \
    bestow::I##Dep2Type* dep2_ = nullptr; \
public:

// 3 dependencies
#define BESTOW_SYSTEM_3(ImplType, InterfaceType, Dep1Type, Dep2Type, Dep3Type) \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::I##Dep1Type##Service> d1, \
            kgr::inject_t<bestow::I##Dep2Type##Service> d2, \
            kgr::inject_t<bestow::I##Dep3Type##Service> d3) \
            -> kgr::inject_result<bestow::I##Dep1Type*, bestow::I##Dep2Type*, bestow::I##Dep3Type*> { \
            return kgr::inject(&d1.service(), &d2.service(), &d3.service()); \
        } \
    }; \
    explicit ImplType( \
        bestow::I##Dep1Type* dep1 = nullptr, \
        bestow::I##Dep2Type* dep2 = nullptr, \
        bestow::I##Dep3Type* dep3 = nullptr) \
        : dep1_(dep1), dep2_(dep2), dep3_(dep3) {} \
protected: \
    bestow::I##Dep1Type* dep1_ = nullptr; \
    bestow::I##Dep2Type* dep2_ = nullptr; \
    bestow::I##Dep3Type* dep3_ = nullptr; \
public:

// 4 dependencies
#define BESTOW_SYSTEM_4(ImplType, InterfaceType, Dep1Type, Dep2Type, Dep3Type, Dep4Type) \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::I##InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::I##Dep1Type##Service> d1, \
            kgr::inject_t<bestow::I##Dep2Type##Service> d2, \
            kgr::inject_t<bestow::I##Dep3Type##Service> d3, \
            kgr::inject_t<bestow::I##Dep4Type##Service> d4) \
            -> kgr::inject_result<bestow::I##Dep1Type*, bestow::I##Dep2Type*, bestow::I##Dep3Type*, bestow::I##Dep4Type*> { \
            return kgr::inject(&d1.service(), &d2.service(), &d3.service(), &d4.service()); \
        } \
    }; \
    explicit ImplType( \
        bestow::I##Dep1Type* dep1 = nullptr, \
        bestow::I##Dep2Type* dep2 = nullptr, \
        bestow::I##Dep3Type* dep3 = nullptr, \
        bestow::I##Dep4Type* dep4 = nullptr) \
        : dep1_(dep1), dep2_(dep2), dep3_(dep3), dep4_(dep4) {} \
protected: \
    bestow::I##Dep1Type* dep1_ = nullptr; \
    bestow::I##Dep2Type* dep2_ = nullptr; \
    bestow::I##Dep3Type* dep3_ = nullptr; \
    bestow::I##Dep4Type* dep4_ = nullptr; \
public:

// Dispatch helpers for BESTOW_SYSTEM
#define BESTOW_SYS_0(ImplType, InterfaceType) BESTOW_SYSTEM_0(ImplType, InterfaceType)
#define BESTOW_SYS_1(ImplType, InterfaceType, D1) BESTOW_SYSTEM_1(ImplType, InterfaceType, D1)
#define BESTOW_SYS_2(ImplType, InterfaceType, D1, D2) BESTOW_SYSTEM_2(ImplType, InterfaceType, D1, D2)
#define BESTOW_SYS_3(ImplType, InterfaceType, D1, D2, D3) BESTOW_SYSTEM_3(ImplType, InterfaceType, D1, D2, D3)
#define BESTOW_SYS_4(ImplType, InterfaceType, D1, D2, D3, D4) BESTOW_SYSTEM_4(ImplType, InterfaceType, D1, D2, D3, D4)

// Primary variadic macro for nested Service pattern
// Usage: BESTOW_SYSTEM(ImplType, InterfaceType [, Dep1, Dep2, ...])
#define BESTOW_SYSTEM(ImplType, InterfaceType, ...) \
    BESTOW_CONCAT(BESTOW_SYS_, BESTOW_DEP_COUNT(__VA_ARGS__ __VA_OPT__(,) _))(ImplType, InterfaceType __VA_OPT__(,) __VA_ARGS__)

export module bestow.services;

import std;

// Re-export all contract modules so implementation modules can access interfaces
// through a single import of bestow.services
export import bestow.types;
export import bestow.entity;
export import bestow.graphics;
export import bestow.graphics3d;
export import bestow.audio;
export import bestow.input;
export import bestow.assets;
export import bestow.save;
export import bestow.level;
export import bestow.events;
export import bestow.physics;
export import bestow.physics3d;
export import bestow.ai;
export import bestow.camera;
export import bestow.config;
export import bestow.gas;
export import bestow.blueprints;
export import bestow.ui;
export import bestow.gamestate;
export import bestow.shader;

export namespace bestow {

//==========================================================================
// Abstract Service Definitions
//
// These are the abstract service types that backends override.
// Use kgr::service<IXxxSystemService>(container) to get the interface.
//==========================================================================

// Core Entity System
struct IEntitySystemService : kgr::abstract_service<IEntitySystem> {};

// Event System
struct IEventSystemService : kgr::abstract_service<IEventSystem> {};

// Configuration System
struct IConfigSystemService : kgr::abstract_service<IConfigSystem> {};

// Asset System
struct IAssetSystemService : kgr::abstract_service<IAssetSystem> {};

// Input System
struct IInputSystemService : kgr::abstract_service<IInputSystem> {};

// Audio System
struct IAudioSystemService : kgr::abstract_service<IAudioSystem> {};

// Save System
struct ISaveSystemService : kgr::abstract_service<ISaveSystem> {};

// Level System
struct ILevelSystemService : kgr::abstract_service<ILevelSystem> {};

// Camera System
struct ICameraSystemService : kgr::abstract_service<ICameraSystem> {};

// 2D Graphics System
struct IGraphicsSystemService : kgr::abstract_service<IGraphicsSystem> {};

// 3D Graphics System
struct IGraphics3DSystemService : kgr::abstract_service<IGraphics3DSystem> {};

// 2D Physics System
struct IPhysicsSystemService : kgr::abstract_service<IPhysicsSystem> {};

// 3D Physics System
struct IPhysics3DSystemService : kgr::abstract_service<IPhysics3DSystem> {};

// AI System
struct IAISystemService : kgr::abstract_service<IAISystem> {};

// UI System
struct IUISystemService : kgr::abstract_service<IUISystem> {};

// Game State System
struct IGameStateSystemService : kgr::abstract_service<IGameStateSystem> {};

// Gameplay Ability System (GAS)
struct IGASSystemService : kgr::abstract_service<IGASSystem> {};

// Shader System
struct IShaderSystemService : kgr::abstract_service<IShaderSystem> {};

// Blueprint Factory
struct IBlueprintFactoryService : kgr::abstract_service<IBlueprintFactory> {};

//==========================================================================
// Application Interface
//==========================================================================

/// Application interface that clients implement.
/// The Engine will instantiate the client's Application via DI and call run().
class IApplication {
public:
    virtual ~IApplication() = default;
    
    /// Called by the Engine to start the application.
    /// All systems have been initialized and are available via DI.
    virtual void run() = 0;
    
    /// Called by the Engine when shutdown is requested.
    /// Application should clean up and exit gracefully.
    virtual void shutdown() {}
};

// Application Service for DI
struct IApplicationService : kgr::abstract_service<IApplication> {};

//==========================================================================
// Engine Interface
//==========================================================================

/// Engine interface for composition root and client interaction.
class IEngine {
public:
    virtual ~IEngine() = default;
    
    /// Register an implementation type for a contract interface.
    /// The implementation must be constructible and satisfy the contract.
    template<typename Contract, typename Implementation>
    void registerSystem() {
        static_assert(std::is_base_of_v<Contract, Implementation>, 
            "Implementation must inherit from Contract interface");
        doRegisterSystem<Contract, Implementation>();
    }
    
    /// Register a factory function that creates an implementation.
    /// Useful for complex initialization or when implementation is not default constructible.
    template<typename Contract>
    void registerSystem(std::function<std::unique_ptr<Contract>()> factory) {
        doRegisterFactory<Contract>(std::move(factory));
    }
    
    /// Register an existing instance of an implementation.
    /// Engine takes ownership of the instance.
    template<typename Contract>
    void registerInstance(std::unique_ptr<Contract> instance) {
        doRegisterInstance<Contract>(std::move(instance));
    }
    
    /// Initialize all registered systems and start the application.
    /// Will call the registered Application's run() method.
    virtual void run() = 0;
    
    /// Request engine shutdown. Will call Application's shutdown() method.
    virtual void shutdown() = 0;
    
    /// Get a system instance by contract interface.
    /// Useful for late binding or optional system access.
    template<typename Contract>
    Contract* getSystem() {
        return doGetSystem<Contract>();
    }

protected:
    virtual void doRegisterSystem_impl(const std::type_info& contract, const std::type_info& impl) = 0;
    virtual void doRegisterFactory_impl(const std::type_info& contract, std::function<void*()> factory) = 0;
    virtual void doRegisterInstance_impl(const std::type_info& contract, void* instance) = 0;
    virtual void* doGetSystem_impl(const std::type_info& contract) = 0;
    
private:
    template<typename Contract, typename Implementation>
    void doRegisterSystem() {
        doRegisterSystem_impl(typeid(Contract), typeid(Implementation));
    }
    
    template<typename Contract>
    void doRegisterFactory(std::function<std::unique_ptr<Contract>()> factory) {
        auto voidFactory = [f = std::move(factory)]() -> void* {
            return f().release();
        };
        doRegisterFactory_impl(typeid(Contract), std::move(voidFactory));
    }
    
    template<typename Contract>
    void doRegisterInstance(std::unique_ptr<Contract> instance) {
        doRegisterInstance_impl(typeid(Contract), instance.release());
    }
    
    template<typename Contract>
    Contract* doGetSystem() {
        return static_cast<Contract*>(doGetSystem_impl(typeid(Contract)));
    }
};

// Engine Service for DI
struct IEngineService : kgr::abstract_service<IEngine> {};

}  // namespace bestow
