// tests/unit/EntitySystemTests.cpp
// Entity system unit tests

#include <cstddef>
#include <memory>
#include <vector>

// Use compatibility header for MSVC C++23 module support
#include <bestow/entt_compat.hpp>
#include <gtest/gtest.h>
#include <kangaru/kangaru.hpp>

import bestow.entity;
import bestow.entity.impl;
import bestow.types;

namespace bestow::tests {

class EntitySystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        entitySystem_ = std::make_unique<EntitySystem>();
    }

    std::unique_ptr<IEntitySystem> entitySystem_;
};

TEST_F(EntitySystemTest, CreateEntity) {
    Entity entity = entitySystem_->createEntity();
    EXPECT_TRUE(entitySystem_->isValid(entity));
    EXPECT_EQ(entitySystem_->entityCount(), 1);
}

TEST_F(EntitySystemTest, DestroyEntity) {
    Entity entity = entitySystem_->createEntity();
    EXPECT_TRUE(entitySystem_->isValid(entity));

    entitySystem_->destroyEntity(entity);
    EXPECT_FALSE(entitySystem_->isValid(entity));
    EXPECT_EQ(entitySystem_->entityCount(), 0);
}

TEST_F(EntitySystemTest, CreateMultipleEntities) {
    std::vector<Entity> entities;
    for (int i = 0; i < 100; ++i) {
        entities.push_back(entitySystem_->createEntity());
    }

    EXPECT_EQ(entitySystem_->entityCount(), 100);

    for (auto entity : entities) {
        EXPECT_TRUE(entitySystem_->isValid(entity));
    }
}

struct TestComponent {
    int value;
    float data;
};

TEST_F(EntitySystemTest, AddComponent) {
    Entity entity = entitySystem_->createEntity();
    auto& comp = entitySystem_->emplace<TestComponent>(entity, 42, 3.14f);

    EXPECT_EQ(comp.value, 42);
    EXPECT_FLOAT_EQ(comp.data, 3.14f);
}

TEST_F(EntitySystemTest, GetComponent) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 100, 2.5f);

    auto& comp = entitySystem_->get<TestComponent>(entity);
    EXPECT_EQ(comp.value, 100);
}

TEST_F(EntitySystemTest, ViewIteration) {
    for (int i = 0; i < 10; ++i) {
        Entity entity = entitySystem_->createEntity();
        entitySystem_->emplace<TestComponent>(entity, i, static_cast<float>(i));
    }

    int count = 0;
    for (auto entity : entitySystem_->view<TestComponent>()) {
        (void)entity;  // Suppress unused warning
        count++;
    }

    EXPECT_EQ(count, 10);
}

//==============================================================================
// Entity Query System Tests
//==============================================================================

struct TagA {
    TagA() = default;
    bool _ = false;  // Dummy member to avoid empty struct (EnTT emplace returns void for empty types)
};
struct TagB {
    TagB() = default;
    bool _ = false;  // Dummy member to avoid empty struct (EnTT emplace returns void for empty types)
};
struct TagC {
    TagC() = default;
    bool _ = false;  // Dummy member to avoid empty struct (EnTT emplace returns void for empty types)
};

TEST_F(EntitySystemTest, GroupCountEmpty) {
    EXPECT_EQ(entitySystem_->groupCount<TestComponent>(), 0);
    EXPECT_EQ(entitySystem_->groupCount<TagA>(), 0);
}

TEST_F(EntitySystemTest, GroupCountSingle) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);

    EXPECT_EQ(entitySystem_->groupCount<TagA>(), 1);
    EXPECT_EQ(entitySystem_->groupCount<TagB>(), 0);
}

TEST_F(EntitySystemTest, GroupCountMultiple) {
    for (int i = 0; i < 5; ++i) {
        Entity e = entitySystem_->createEntity();
        entitySystem_->emplace<TagA>(e);
    }

    EXPECT_EQ(entitySystem_->groupCount<TagA>(), 5);
}

TEST_F(EntitySystemTest, GroupCountMultipleComponents) {
    Entity e1 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e1);
    entitySystem_->emplace<TagB>(e1);

    Entity e2 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e2);

    // Only e1 has both TagA and TagB
    EXPECT_EQ((entitySystem_->groupCount<TagA, TagB>()), 1);
    EXPECT_EQ(entitySystem_->groupCount<TagA>(), 2);
}

TEST_F(EntitySystemTest, HasAnyEmpty) {
    EXPECT_FALSE(entitySystem_->hasAny<TagA>());
}

TEST_F(EntitySystemTest, HasAnyWithEntities) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);

    EXPECT_TRUE(entitySystem_->hasAny<TagA>());
    EXPECT_FALSE(entitySystem_->hasAny<TagB>());
}

TEST_F(EntitySystemTest, FirstEmpty) {
    auto result = entitySystem_->first<TagA>();
    EXPECT_FALSE(result.has_value());
}

TEST_F(EntitySystemTest, FirstWithEntity) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);

    auto result = entitySystem_->first<TagA>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, entity);
}

TEST_F(EntitySystemTest, FirstWithMultipleEntities) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e1);
    entitySystem_->emplace<TagA>(e2);

    auto result = entitySystem_->first<TagA>();
    ASSERT_TRUE(result.has_value());
    // Should return one of them (either e1 or e2)
    EXPECT_TRUE(*result == e1 || *result == e2);
}

TEST_F(EntitySystemTest, SingleEmpty) {
    auto result = entitySystem_->single<TagA>();
    EXPECT_FALSE(result.has_value());
}

TEST_F(EntitySystemTest, SingleWithOneEntity) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);

    auto result = entitySystem_->single<TagA>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, entity);
}

TEST_F(EntitySystemTest, SingleWithMultipleEntities) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e1);
    entitySystem_->emplace<TagA>(e2);

    // Should return nullopt when more than one entity matches
    auto result = entitySystem_->single<TagA>();
    EXPECT_FALSE(result.has_value());
}

TEST_F(EntitySystemTest, CollectEmpty) {
    auto entities = entitySystem_->collect<TagA>();
    EXPECT_TRUE(entities.empty());
}

TEST_F(EntitySystemTest, CollectMultiple) {
    std::vector<Entity> created;
    for (int i = 0; i < 5; ++i) {
        Entity e = entitySystem_->createEntity();
        entitySystem_->emplace<TagA>(e);
        created.push_back(e);
    }

    auto collected = entitySystem_->collect<TagA>();
    EXPECT_EQ(collected.size(), 5);

    // All created entities should be in collected
    for (Entity e : created) {
        EXPECT_TRUE(std::find(collected.begin(), collected.end(), e) != collected.end());
    }
}

TEST_F(EntitySystemTest, CollectExcludingBasic) {
    Entity alive = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(alive);

    Entity dead = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(dead);
    entitySystem_->emplace<TagB>(dead);  // TagB marks as "dead"

    // Collect TagA excluding TagB
    auto livingEntities = entitySystem_->collectExcluding<TagA, TagB>();
    EXPECT_EQ(livingEntities.size(), 1);
    EXPECT_EQ(livingEntities[0], alive);
}

TEST_F(EntitySystemTest, CollectExcludingMultiple) {
    // Create 3 entities with TagA
    Entity e1 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e1);

    Entity e2 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e2);
    entitySystem_->emplace<TagB>(e2);  // Excluded

    Entity e3 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e3);
    entitySystem_->emplace<TagC>(e3);  // Not excluded

    // Only e2 should be excluded (has TagB)
    auto result = entitySystem_->collectExcluding<TagA, TagB>();
    EXPECT_EQ(result.size(), 2);
}

TEST_F(EntitySystemTest, CollectSafeForDeletion) {
    for (int i = 0; i < 5; ++i) {
        Entity e = entitySystem_->createEntity();
        entitySystem_->emplace<TagA>(e);
    }

    // Collect first, then delete - should be safe
    auto entities = entitySystem_->collect<TagA>();
    for (Entity e : entities) {
        entitySystem_->destroyEntity(e);
    }

    EXPECT_EQ(entitySystem_->groupCount<TagA>(), 0);
    EXPECT_EQ(entitySystem_->entityCount(), 0);
}

//==============================================================================
// Component Management Tests
//==============================================================================

TEST_F(EntitySystemTest, RemoveComponent) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 42, 3.14f);

    EXPECT_TRUE(entitySystem_->allOf<TestComponent>(entity));

    entitySystem_->remove<TestComponent>(entity);

    EXPECT_FALSE(entitySystem_->allOf<TestComponent>(entity));
}

TEST_F(EntitySystemTest, TryGetExisting) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 100, 2.5f);

    TestComponent* comp = entitySystem_->tryGet<TestComponent>(entity);
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(comp->value, 100);
}

TEST_F(EntitySystemTest, TryGetNonExisting) {
    Entity entity = entitySystem_->createEntity();

    TestComponent* comp = entitySystem_->tryGet<TestComponent>(entity);
    EXPECT_EQ(comp, nullptr);
}

TEST_F(EntitySystemTest, TryGetConstExisting) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 42, 1.5f);

    const auto& constSystem = *entitySystem_;
    const TestComponent* comp = constSystem.tryGet<TestComponent>(entity);
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(comp->value, 42);
}

TEST_F(EntitySystemTest, TryGetConstNonExisting) {
    Entity entity = entitySystem_->createEntity();

    const auto& constSystem = *entitySystem_;
    const TestComponent* comp = constSystem.tryGet<TestComponent>(entity);
    EXPECT_EQ(comp, nullptr);
}

TEST_F(EntitySystemTest, AllOfSingleComponent) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);

    EXPECT_TRUE(entitySystem_->allOf<TagA>(entity));
    EXPECT_FALSE(entitySystem_->allOf<TagB>(entity));
}

TEST_F(EntitySystemTest, AllOfMultipleComponents) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);
    entitySystem_->emplace<TagB>(entity);

    EXPECT_TRUE((entitySystem_->allOf<TagA, TagB>(entity)));
    EXPECT_FALSE((entitySystem_->allOf<TagA, TagB, TagC>(entity)));
}

TEST_F(EntitySystemTest, AnyOfSingleComponent) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);

    EXPECT_TRUE(entitySystem_->anyOf<TagA>(entity));
    EXPECT_FALSE(entitySystem_->anyOf<TagB>(entity));
}

TEST_F(EntitySystemTest, AnyOfMultipleComponents) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(entity);

    EXPECT_TRUE((entitySystem_->anyOf<TagA, TagB>(entity)));
    EXPECT_TRUE((entitySystem_->anyOf<TagB, TagA>(entity)));
    EXPECT_FALSE((entitySystem_->anyOf<TagB, TagC>(entity)));
}

//==============================================================================
// Type-Erased Component Tests
//==============================================================================

TEST_F(EntitySystemTest, AddComponentTypeErased) {
    Entity entity = entitySystem_->createEntity();
    TestComponent data{42, 3.14f};

    void* result = entitySystem_->addComponent(
        entity,
        entt::type_hash<TestComponent>::value(),
        &data,
        sizeof(TestComponent)
    );

    // Current implementation returns nullptr - this is a stub
    // When properly implemented, should return non-null
    EXPECT_EQ(result, nullptr);
}

TEST_F(EntitySystemTest, RemoveComponentTypeErased) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 100, 2.5f);

    // This is a stub method - doesn't actually remove
    entitySystem_->removeComponent(entity, entt::type_hash<TestComponent>::value());

    // Since it's stubbed, component should still exist
    EXPECT_TRUE(entitySystem_->allOf<TestComponent>(entity));
}

TEST_F(EntitySystemTest, GetComponentTypeErased) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 42, 1.5f);

    void* comp = entitySystem_->getComponent(entity, entt::type_hash<TestComponent>::value());

    // Current implementation returns nullptr - this is a stub
    EXPECT_EQ(comp, nullptr);
}

TEST_F(EntitySystemTest, GetComponentTypeErasedConst) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 42, 1.5f);

    const auto& constSystem = *entitySystem_;
    const void* comp = constSystem.getComponent(entity, entt::type_hash<TestComponent>::value());

    // Current implementation returns nullptr - this is a stub
    EXPECT_EQ(comp, nullptr);
}

TEST_F(EntitySystemTest, HasComponentTypeErased) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 42, 1.5f);

    bool has = entitySystem_->hasComponent(entity, entt::type_hash<TestComponent>::value());

    // Current implementation returns false - this is a stub
    EXPECT_FALSE(has);
}

//==============================================================================
// Query System Tests
//==============================================================================

TEST_F(EntitySystemTest, QueryEmpty) {
    EntitySelector selector{};
    auto result = entitySystem_->query(selector);

    EXPECT_TRUE(result.empty());
}

TEST_F(EntitySystemTest, QueryWithEntities) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();

    EntitySelector selector{};
    auto result = entitySystem_->query(selector);

    // Query should return all entities when no filters specified
    EXPECT_EQ(result.size(), 2);
}

TEST_F(EntitySystemTest, QueryWithPredicate) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();
    Entity e3 = entitySystem_->createEntity();

    entitySystem_->emplace<TestComponent>(e1, 10, 1.0f);
    entitySystem_->emplace<TestComponent>(e2, 20, 2.0f);
    entitySystem_->emplace<TestComponent>(e3, 30, 3.0f);

    EntitySelector selector{};
    selector.predicate = [this](Entity e) {
        auto* comp = entitySystem_->tryGet<TestComponent>(e);
        return comp && comp->value >= 20;
    };

    auto result = entitySystem_->query(selector);

    // Should only return e2 and e3 (value >= 20)
    EXPECT_EQ(result.size(), 2);
}

//==============================================================================
// Iteration Tests
//==============================================================================

TEST_F(EntitySystemTest, EachCallback) {
    std::vector<Entity> created;
    for (int i = 0; i < 5; ++i) {
        created.push_back(entitySystem_->createEntity());
    }

    std::vector<Entity> iterated;
    entitySystem_->each([&iterated](Entity e) {
        iterated.push_back(e);
    });

    EXPECT_EQ(iterated.size(), 5);

    // All created entities should be iterated
    for (Entity e : created) {
        EXPECT_TRUE(std::find(iterated.begin(), iterated.end(), e) != iterated.end());
    }
}

TEST_F(EntitySystemTest, EachCallbackWithInvalidEntities) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();
    entitySystem_->destroyEntity(e2);
    Entity e3 = entitySystem_->createEntity();

    std::vector<Entity> iterated;
    entitySystem_->each([&iterated](Entity e) {
        iterated.push_back(e);
    });

    // Should only iterate valid entities (e1 and e3)
    EXPECT_EQ(iterated.size(), 2);
    EXPECT_TRUE(std::find(iterated.begin(), iterated.end(), e1) != iterated.end());
    EXPECT_TRUE(std::find(iterated.begin(), iterated.end(), e3) != iterated.end());
}

TEST_F(EntitySystemTest, UpdateDoesNotCrash) {
    // Entity system update is a no-op but should not crash
    entitySystem_->update(DeltaTime{16.67f});
    entitySystem_->update(DeltaTime{0.0f});
    entitySystem_->update(DeltaTime{1000.0f});
}

//==============================================================================
// Registry Access Tests
//==============================================================================

TEST_F(EntitySystemTest, GetRegistryMutable) {
    entt::registry& reg = entitySystem_->getRegistry();
    Entity e = reg.create();

    EXPECT_TRUE(entitySystem_->isValid(e));
}

TEST_F(EntitySystemTest, GetRegistryConst) {
    Entity e = entitySystem_->createEntity();

    const auto& constSystem = *entitySystem_;
    const entt::registry& reg = constSystem.getRegistry();

    EXPECT_TRUE(reg.valid(e));
}

//==============================================================================
// Edge Cases and Boundary Tests
//==============================================================================

TEST_F(EntitySystemTest, DestroyAlreadyDestroyedEntity) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->destroyEntity(entity);

    // Destroying again should not crash
    entitySystem_->destroyEntity(entity);

    EXPECT_FALSE(entitySystem_->isValid(entity));
}

TEST_F(EntitySystemTest, DestroyInvalidEntity) {
    // Create a fake invalid entity
    Entity invalid = Entity{999999};

    // Should not crash
    entitySystem_->destroyEntity(invalid);

    EXPECT_FALSE(entitySystem_->isValid(invalid));
}

TEST_F(EntitySystemTest, IsValidDefaultConstructedEntity) {
    Entity entity{};
    EXPECT_FALSE(entitySystem_->isValid(entity));
}

TEST_F(EntitySystemTest, EmptySystemEntityCount) {
    EXPECT_EQ(entitySystem_->entityCount(), 0);
}

TEST_F(EntitySystemTest, EntityCountAfterCreateAndDestroy) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();
    Entity e3 = entitySystem_->createEntity();

    EXPECT_EQ(entitySystem_->entityCount(), 3);

    entitySystem_->destroyEntity(e2);

    EXPECT_EQ(entitySystem_->entityCount(), 2);

    entitySystem_->destroyEntity(e1);
    entitySystem_->destroyEntity(e3);

    EXPECT_EQ(entitySystem_->entityCount(), 0);
}

TEST_F(EntitySystemTest, ViewConstVersion) {
    for (int i = 0; i < 3; ++i) {
        Entity entity = entitySystem_->createEntity();
        entitySystem_->emplace<TestComponent>(entity, i * 10, static_cast<float>(i));
    }

    const auto& constSystem = *entitySystem_;
    int count = 0;
    for (auto entity : constSystem.view<TestComponent>()) {
        (void)entity;
        count++;
    }

    EXPECT_EQ(count, 3);
}

TEST_F(EntitySystemTest, MultipleComponentTypes) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 42, 3.14f);
    entitySystem_->emplace<TagA>(entity);
    entitySystem_->emplace<TagB>(entity);

    EXPECT_TRUE(entitySystem_->allOf<TestComponent>(entity));
    EXPECT_TRUE(entitySystem_->allOf<TagA>(entity));
    EXPECT_TRUE(entitySystem_->allOf<TagB>(entity));
    EXPECT_TRUE((entitySystem_->allOf<TestComponent, TagA, TagB>(entity)));
}

TEST_F(EntitySystemTest, GetComponentModification) {
    Entity entity = entitySystem_->createEntity();
    entitySystem_->emplace<TestComponent>(entity, 42, 3.14f);

    auto& comp = entitySystem_->get<TestComponent>(entity);
    comp.value = 100;
    comp.data = 9.99f;

    auto& verifyComp = entitySystem_->get<TestComponent>(entity);
    EXPECT_EQ(verifyComp.value, 100);
    EXPECT_FLOAT_EQ(verifyComp.data, 9.99f);
}

TEST_F(EntitySystemTest, CollectMultipleComponentTypes) {
    // Create entities with different component combinations
    Entity e1 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e1);
    entitySystem_->emplace<TagB>(e1);

    Entity e2 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e2);
    entitySystem_->emplace<TagB>(e2);

    Entity e3 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e3);

    auto result = entitySystem_->collect<TagA, TagB>();
    EXPECT_EQ(result.size(), 2);
}

TEST_F(EntitySystemTest, GroupCountAfterRemoval) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e1);
    entitySystem_->emplace<TagA>(e2);

    EXPECT_EQ(entitySystem_->groupCount<TagA>(), 2);

    entitySystem_->remove<TagA>(e1);

    EXPECT_EQ(entitySystem_->groupCount<TagA>(), 1);
}

TEST_F(EntitySystemTest, HasAnyAfterRemoval) {
    Entity e = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e);

    EXPECT_TRUE(entitySystem_->hasAny<TagA>());

    entitySystem_->remove<TagA>(e);

    EXPECT_FALSE(entitySystem_->hasAny<TagA>());
}

TEST_F(EntitySystemTest, FirstAfterEntityDestruction) {
    Entity e1 = entitySystem_->createEntity();
    Entity e2 = entitySystem_->createEntity();
    entitySystem_->emplace<TagA>(e1);
    entitySystem_->emplace<TagA>(e2);

    entitySystem_->destroyEntity(e1);

    auto result = entitySystem_->first<TagA>();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, e2);
}

//==============================================================================
// Stress and Performance Tests
//==============================================================================

TEST_F(EntitySystemTest, CreateManyEntities) {
    const int COUNT = 10000;
    std::vector<Entity> entities;
    entities.reserve(COUNT);

    for (int i = 0; i < COUNT; ++i) {
        entities.push_back(entitySystem_->createEntity());
    }

    EXPECT_EQ(entitySystem_->entityCount(), COUNT);

    for (auto e : entities) {
        EXPECT_TRUE(entitySystem_->isValid(e));
    }
}

TEST_F(EntitySystemTest, CreateAndDestroyMany) {
    const int COUNT = 1000;

    for (int cycle = 0; cycle < 5; ++cycle) {
        std::vector<Entity> entities;
        for (int i = 0; i < COUNT; ++i) {
            entities.push_back(entitySystem_->createEntity());
        }

        EXPECT_EQ(entitySystem_->entityCount(), COUNT);

        for (auto e : entities) {
            entitySystem_->destroyEntity(e);
        }

        EXPECT_EQ(entitySystem_->entityCount(), 0);
    }
}

TEST_F(EntitySystemTest, ManyComponentsPerEntity) {
    Entity entity = entitySystem_->createEntity();

    entitySystem_->emplace<TestComponent>(entity, 1, 1.0f);
    entitySystem_->emplace<TagA>(entity);
    entitySystem_->emplace<TagB>(entity);
    entitySystem_->emplace<TagC>(entity);

    EXPECT_TRUE((entitySystem_->allOf<TestComponent, TagA, TagB, TagC>(entity)));
    EXPECT_EQ(entitySystem_->get<TestComponent>(entity).value, 1);
}

//==============================================================================
// Kangaru DI Integration Tests
//==============================================================================

TEST(EntitySystemKangaruTests, ServiceInstantiation) {
    kgr::container container;
    auto& system = container.service<EntitySystemService>();

    // Verify the service is functional
    Entity entity = system.createEntity();
    EXPECT_TRUE(system.isValid(entity));
}

TEST(EntitySystemKangaruTests, ServiceIsSingleton) {
    kgr::container container;
    auto& system1 = container.service<EntitySystemService>();
    auto& system2 = container.service<EntitySystemService>();

    // Same instance should be returned (singleton)
    EXPECT_EQ(&system1, &system2);
}

TEST(EntitySystemKangaruTests, ServiceRetainsSameRegistry) {
    kgr::container container;
    auto& system = container.service<EntitySystemService>();

    Entity e1 = system.createEntity();

    // Access service again - should have same registry with same entity
    auto& systemAgain = container.service<EntitySystemService>();
    EXPECT_TRUE(systemAgain.isValid(e1));
    EXPECT_EQ(systemAgain.entityCount(), 1);
}

TEST(EntitySystemKangaruTests, ServiceSupportsComponents) {
    kgr::container container;
    auto& system = container.service<EntitySystemService>();

    Entity entity = system.createEntity();
    system.emplace<TestComponent>(entity, 42, 3.14f);

    auto& comp = system.get<TestComponent>(entity);
    EXPECT_EQ(comp.value, 42);
    EXPECT_FLOAT_EQ(comp.data, 3.14f);
}

TEST(EntitySystemKangaruTests, MultipleContainersHaveSeparateServices) {
    kgr::container container1;
    kgr::container container2;

    auto& system1 = container1.service<EntitySystemService>();
    auto& system2 = container2.service<EntitySystemService>();

    // Different containers should have different service instances
    EXPECT_NE(&system1, &system2);

    // Entities created in one should not exist in the other
    Entity e1 = system1.createEntity();
    EXPECT_FALSE(system2.isValid(e1));
}

TEST(EntitySystemKangaruTests, ServiceSupportsInterfacePolymorphism) {
    kgr::container container;
    auto& system = container.service<EntitySystemService>();

    // Can be used through IEntitySystem interface
    IEntitySystem& interface = system;
    Entity entity = interface.createEntity();
    EXPECT_TRUE(interface.isValid(entity));
}

}  // namespace bestow::tests
