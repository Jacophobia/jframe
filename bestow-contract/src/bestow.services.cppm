// bestow-contract/src/bestow.services.cppm
// Central module for all abstract Kangaru service definitions
// Clients import this module to get abstract services for dependency injection.
// Implementation modules import this and provide concrete overrides.

module;

#include <kangaru/kangaru.hpp>

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
