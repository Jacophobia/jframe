// bestow-contract/src/bestow.services.cppm
// Central module that re-exports all contract interfaces and provides
// the Application base class for client games.

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
export import bestow.state;
export import bestow.scene;
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

//==========================================================================
// Engine Interface
//==========================================================================

/// Engine interface for composition root and client interaction.
class IEngine {
public:
    virtual ~IEngine() = default;

    /// Register an implementation type for a contract interface.
    template<typename Contract, typename Implementation>
    void registerSystem() {
        static_assert(std::is_base_of_v<Contract, Implementation>,
            "Implementation must inherit from Contract interface");
        doRegisterSystem<Contract, Implementation>();
    }

    /// Register a factory function that creates an implementation.
    template<typename Contract>
    void registerSystem(std::function<std::unique_ptr<Contract>()> factory) {
        doRegisterFactory<Contract>(std::move(factory));
    }

    /// Register an existing instance of an implementation.
    template<typename Contract>
    void registerInstance(std::unique_ptr<Contract> instance) {
        doRegisterInstance<Contract>(std::move(instance));
    }

    /// Initialize all registered systems and start the application.
    virtual void run() = 0;

    /// Request engine shutdown.
    virtual void shutdown() = 0;

    /// Get a system instance by contract interface.
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

}  // namespace bestow
