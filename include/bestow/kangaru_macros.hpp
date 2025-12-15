// include/bestow/kangaru_macros.hpp
// Kangaru DI helper macros for system implementation
//
// These macros must be #included in the global module fragment of implementation
// modules because macros are NOT exported through C++20 modules.
//
// Usage in implementation modules:
//   module;
//   #include <kangaru/kangaru.hpp>
//   #include <bestow/kangaru_macros.hpp>
//   ...
//   export module bestow.yoursystem.impl;
//   import bestow.services;
//
//   export namespace bestow {
//   BESTOW_SYSTEM(YourSystem, IYourSystem, IDep1, IDep2)
//   public:
//       void doWork() { pIDep1_->method(); }
//   };
//   }
//
// Note: This header requires <kangaru/kangaru.hpp> to be included first.

#ifndef BESTOW_KANGARU_MACROS_HPP
#define BESTOW_KANGARU_MACROS_HPP

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
//   - Private member variables (pSystemName_)
//
// Usage:
//   BESTOW_SYSTEM(ConfigSystem, IConfigSystem, IAssetSystem, IEventSystem)
//   public:
//       void loadConfig() {
//           pIAssetSystem_->load(...);
//           pIEventSystem_->emit(...);
//       }
//   };
//   // Register: container.service<ConfigSystem::Service>()
//
// Pass FULL type names for dependencies (IAssetSystem, not AssetSystem).
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
//   - Private member variables (pDepName_)
//
// Usage: BESTOW_SYSTEM(ImplType, InterfaceType, Dep1, Dep2, ...) body };
//
// Pass FULL type names - no automatic I prefix is added anywhere.
// This allows injecting both interfaces and non-interface types.
//
// Example:
//   BESTOW_SYSTEM(ConfigSystem, IConfigSystem, IAssetSystem, IEventSystem)
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
// Usage: BESTOW_SYSTEM(ImplType, InterfaceType [, Dep1, Dep2, ...]) body };
#define BESTOW_SYSTEM(ImplType, InterfaceType, ...) \
    BESTOW_CONCAT(BESTOW_SYS_, BESTOW_DEP_COUNT(__VA_ARGS__ __VA_OPT__(,) _))(ImplType, InterfaceType __VA_OPT__(,) __VA_ARGS__)

#endif // BESTOW_KANGARU_MACROS_HPP
