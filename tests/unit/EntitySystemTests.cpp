// tests/unit/EntitySystemTests.cpp
// Entity system unit tests

#include <cstddef>
#include <memory>
#include <vector>

#include <entt/entt.hpp>
#include <gtest/gtest.h>

import jframe.entity;
import jframe.entity.impl;
import jframe.types;

namespace jframe::tests {

class EntitySystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        entitySystem_ = createEntitySystem();
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

struct TagA {};
struct TagB {};
struct TagC {};

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

}  // namespace jframe::tests
