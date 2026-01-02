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
// PATTERN 1: BESTOW_SYSTEM - Complete class with nested Service (PREFERRED)
// =========================================================================
// Generates the entire class declaration including:
//   - Class opening line with inheritance
//   - Nested Service struct for Kangaru registration
//   - Constructor with dependency injection
//   - Protected member variables (pSystemName_)
//
// Usage:
//   BESTOW_SYSTEM(ConfigSystem, IConfigSystem, AssetSystem, EventSystem) {
//   public:
//       void loadConfig() {
//           pAssetSystem_->load(...);   // IAssetSystem*
//           pEventSystem_->emit(...);   // IEventSystem*
//       }
//   };
//   // Register: container.service<ConfigSystem::Service>()
//
// Note: Pass base names (AssetSystem, not IAssetSystem) for dependencies.
// The macro adds the I prefix for types and service lookup.
// Interface parameter (2nd arg) should include I if it's an interface.
//
// PATTERN 2: BESTOW_SERVICE - Standalone service struct
// ======================================================
// Place after your class definition for a standalone service struct.
//
//   class AssetSystem : public IAssetSystem { ... };
//   BESTOW_SERVICE(AssetSystem, AssetSystem, EventSystem);
//
// Both patterns support 0-4 dependencies and auto-select based on count.
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

// Count only the dependency arguments (accounts for trailing sentinel '_')
// The sentinel '_' is always added via __VA_OPT__, so we need 5 placeholders before N:
// - 0 deps: (_)           -> N at position 6 -> 0
// - 1 dep:  (D1, _)       -> N at position 6 -> 1
// - 2 deps: (D1, D2, _)   -> N at position 6 -> 2
// - etc.
#define BESTOW_DEP_COUNT_IMPL(_1, _2, _3, _4, _5, N, ...) N
#define BESTOW_DEP_COUNT(...) BESTOW_DEP_COUNT_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)

// Primary variadic macro - automatically dispatches based on dependency count
// Usage: BESTOW_SERVICE(ImplType, InterfaceType [, Dep1, Dep2, ...])
#define BESTOW_SERVICE(ImplType, InterfaceType, ...) \
    BESTOW_CONCAT(BESTOW_DEPS_, BESTOW_DEP_COUNT(__VA_ARGS__ __VA_OPT__(,) _))(ImplType, InterfaceType __VA_OPT__(,) __VA_ARGS__)

//==========================================================================
// BESTOW_SYSTEM - Complete class definition with nested Service
//
// Generates:
//   - Class declaration line: class ImplType : public InterfaceType {
//   - Nested Service struct for Kangaru registration
//   - Constructor with dependency injection
//   - Protected member variables (pDepName_)
//
// Usage: BESTOW_SYSTEM(ImplType, InterfaceType, Dep1, Dep2, ...) { body };
//
// Pass FULL type names - no automatic I prefix is added anywhere.
// This allows injecting both interfaces and non-interface types.
//
// Example:
//   BESTOW_SYSTEM(ConfigSystem, IConfigSystem, IAssetSystem, IEventSystem) {
//   public:
//       void work() { pIAssetSystem_->load(); pIEventSystem_->emit(); }
//   };
//
// For cleaner variable names, you can define type aliases:
//   using AssetSystem = IAssetSystem;  // Then pass AssetSystem -> pAssetSystem_
//==========================================================================

// 0 dependencies - default constructible
#define BESTOW_SYSTEM_0(ImplType, InterfaceType) \
class ImplType : public InterfaceType { \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::InterfaceType##Service> {}; \
    ImplType() = default;

// 1 dependency
#define BESTOW_SYSTEM_1(ImplType, InterfaceType, Dep1) \
class ImplType : public InterfaceType { \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::InterfaceType##Service> { \
        static auto construct(kgr::inject_t<bestow::Dep1##Service> d1) \
            -> kgr::inject_result<bestow::Dep1*> { \
            return kgr::inject(&d1.service()); \
        } \
    }; \
    explicit ImplType(bestow::Dep1* p##Dep1 = nullptr) : p##Dep1##_(p##Dep1) {} \
private: \
    bestow::Dep1* p##Dep1##_ = nullptr;

// 2 dependencies
#define BESTOW_SYSTEM_2(ImplType, InterfaceType, Dep1, Dep2) \
class ImplType : public InterfaceType { \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::Dep1##Service> d1, \
            kgr::inject_t<bestow::Dep2##Service> d2) \
            -> kgr::inject_result<bestow::Dep1*, bestow::Dep2*> { \
            return kgr::inject(&d1.service(), &d2.service()); \
        } \
    }; \
    explicit ImplType(bestow::Dep1* p##Dep1 = nullptr, bestow::Dep2* p##Dep2 = nullptr) \
        : p##Dep1##_(p##Dep1), p##Dep2##_(p##Dep2) {} \
private: \
    bestow::Dep1* p##Dep1##_ = nullptr; \
    bestow::Dep2* p##Dep2##_ = nullptr;

// 3 dependencies
#define BESTOW_SYSTEM_3(ImplType, InterfaceType, Dep1, Dep2, Dep3) \
class ImplType : public InterfaceType { \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::Dep1##Service> d1, \
            kgr::inject_t<bestow::Dep2##Service> d2, \
            kgr::inject_t<bestow::Dep3##Service> d3) \
            -> kgr::inject_result<bestow::Dep1*, bestow::Dep2*, bestow::Dep3*> { \
            return kgr::inject(&d1.service(), &d2.service(), &d3.service()); \
        } \
    }; \
    explicit ImplType( \
        bestow::Dep1* p##Dep1 = nullptr, \
        bestow::Dep2* p##Dep2 = nullptr, \
        bestow::Dep3* p##Dep3 = nullptr) \
        : p##Dep1##_(p##Dep1), p##Dep2##_(p##Dep2), p##Dep3##_(p##Dep3) {} \
private: \
    bestow::Dep1* p##Dep1##_ = nullptr; \
    bestow::Dep2* p##Dep2##_ = nullptr; \
    bestow::Dep3* p##Dep3##_ = nullptr;

// 4 dependencies
#define BESTOW_SYSTEM_4(ImplType, InterfaceType, Dep1, Dep2, Dep3, Dep4) \
class ImplType : public InterfaceType { \
public: \
    struct Service : kgr::single_service<ImplType>, kgr::overrides<bestow::InterfaceType##Service> { \
        static auto construct( \
            kgr::inject_t<bestow::Dep1##Service> d1, \
            kgr::inject_t<bestow::Dep2##Service> d2, \
            kgr::inject_t<bestow::Dep3##Service> d3, \
            kgr::inject_t<bestow::Dep4##Service> d4) \
            -> kgr::inject_result<bestow::Dep1*, bestow::Dep2*, bestow::Dep3*, bestow::Dep4*> { \
            return kgr::inject(&d1.service(), &d2.service(), &d3.service(), &d4.service()); \
        } \
    }; \
    explicit ImplType( \
        bestow::Dep1* p##Dep1 = nullptr, \
        bestow::Dep2* p##Dep2 = nullptr, \
        bestow::Dep3* p##Dep3 = nullptr, \
        bestow::Dep4* p##Dep4 = nullptr) \
        : p##Dep1##_(p##Dep1), p##Dep2##_(p##Dep2), p##Dep3##_(p##Dep3), p##Dep4##_(p##Dep4) {} \
private: \
    bestow::Dep1* p##Dep1##_ = nullptr; \
    bestow::Dep2* p##Dep2##_ = nullptr; \
    bestow::Dep3* p##Dep3##_ = nullptr; \
    bestow::Dep4* p##Dep4##_ = nullptr;

// Dispatch helpers for BESTOW_SYSTEM
#define BESTOW_SYS_0(ImplType, InterfaceType) BESTOW_SYSTEM_0(ImplType, InterfaceType)
#define BESTOW_SYS_1(ImplType, InterfaceType, D1) BESTOW_SYSTEM_1(ImplType, InterfaceType, D1)
#define BESTOW_SYS_2(ImplType, InterfaceType, D1, D2) BESTOW_SYSTEM_2(ImplType, InterfaceType, D1, D2)
#define BESTOW_SYS_3(ImplType, InterfaceType, D1, D2, D3) BESTOW_SYSTEM_3(ImplType, InterfaceType, D1, D2, D3)
#define BESTOW_SYS_4(ImplType, InterfaceType, D1, D2, D3, D4) BESTOW_SYSTEM_4(ImplType, InterfaceType, D1, D2, D3, D4)

// Primary variadic macro for complete class definition
// Usage: BESTOW_SYSTEM(ImplType, InterfaceType [, Dep1, Dep2, ...]) { body };
#define BESTOW_SYSTEM(ImplType, InterfaceType, ...) \
    BESTOW_CONCAT(BESTOW_SYS_, BESTOW_DEP_COUNT(__VA_ARGS__ __VA_OPT__(,) _))(ImplType, InterfaceType __VA_OPT__(,) __VA_ARGS__)

export module bestow.services;

import std;

// Re-export all contract modules so implementation modules can access interfaces
// through a single import of bestow.services
export import bestow.types;
export import bestow.entity;
export import bestow.graphics.context;  // Base graphics interface
export import bestow.uirender;          // UI render backend interface
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
export import bestow.animation;

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

// Graphics Context (base interface for 2D and 3D graphics)
struct IGraphicsContextService : kgr::abstract_service<IGraphicsContext> {};

// UI Render Backend
struct IUIRenderBackendService : kgr::abstract_service<IUIRenderBackend> {};

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

// Blueprint Factory
struct IBlueprintFactoryService : kgr::abstract_service<IBlueprintFactory> {};

// Animation System
struct IAnimationSystemService : kgr::abstract_service<IAnimationSystem> {};

//==========================================================================
// Contract -> Service Type Mapping
//
// This trait maps contract interfaces to their abstract service types.
// Used by Engine::get<Contract>() to resolve services.
//==========================================================================

template<typename Contract> struct ServiceFor;
template<> struct ServiceFor<IEntitySystem> { using type = IEntitySystemService; };
template<> struct ServiceFor<IEventSystem> { using type = IEventSystemService; };
template<> struct ServiceFor<IConfigSystem> { using type = IConfigSystemService; };
template<> struct ServiceFor<IAssetSystem> { using type = IAssetSystemService; };
template<> struct ServiceFor<IInputSystem> { using type = IInputSystemService; };
template<> struct ServiceFor<IAudioSystem> { using type = IAudioSystemService; };
template<> struct ServiceFor<ISaveSystem> { using type = ISaveSystemService; };
template<> struct ServiceFor<ILevelSystem> { using type = ILevelSystemService; };
template<> struct ServiceFor<ICameraSystem> { using type = ICameraSystemService; };
template<> struct ServiceFor<IGraphicsContext> { using type = IGraphicsContextService; };
template<> struct ServiceFor<IUIRenderBackend> { using type = IUIRenderBackendService; };
template<> struct ServiceFor<IGraphicsSystem> { using type = IGraphicsSystemService; };
template<> struct ServiceFor<IGraphics3DSystem> { using type = IGraphics3DSystemService; };
template<> struct ServiceFor<IPhysicsSystem> { using type = IPhysicsSystemService; };
template<> struct ServiceFor<IPhysics3DSystem> { using type = IPhysics3DSystemService; };
template<> struct ServiceFor<IAISystem> { using type = IAISystemService; };
template<> struct ServiceFor<IUISystem> { using type = IUISystemService; };
template<> struct ServiceFor<IGameStateSystem> { using type = IGameStateSystemService; };
template<> struct ServiceFor<IGASSystem> { using type = IGASSystemService; };
template<> struct ServiceFor<IBlueprintFactory> { using type = IBlueprintFactoryService; };
template<> struct ServiceFor<IAnimationSystem> { using type = IAnimationSystemService; };

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

//==========================================================================
// Application Base Class (CRTP)
//
// Use this base class to enable automatic dependency injection with
// engine.run<MyGame>() - no need to list dependencies in the run call.
//
// Usage:
//   class MyGame : public Application<MyGame, IGraphics3DSystem, IInputSystem> {
//   public:
//       MyGame(IGraphics3DSystem& g, IInputSystem& i) : graphics_(&g), input_(&i) {}
//       void run() override { /* game loop */ }
//   };
//
//   engine.run<MyGame>();  // Dependencies auto-detected from base class
//==========================================================================

template<typename Derived, typename... Deps>
class Application : public IApplication {
public:
    /// Type alias for Engine to detect dependencies
    using Dependencies = std::tuple<Deps...>;
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
