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

}  // namespace jframe::tests
