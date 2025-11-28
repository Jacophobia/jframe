// tests/unit/BlueprintFactoryTests.cpp
// Blueprint Factory unit tests

#include <gtest/gtest.h>

import jframe.blueprints;
import jframe.blueprints.impl;
import jframe.entity;
import jframe.entity.impl;
import jframe.physics;
import jframe.physics.impl;
import jframe.types;
import std;

namespace jframe::tests {

class BlueprintFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        entitySystem_ = createEntitySystem();
        physicsSystem_ = createBox2DPhysicsSystem();
        physicsSystem_->initialize();
        blueprintFactory_ = createBlueprintFactory(*entitySystem_, physicsSystem_.get());
    }

    void TearDown() override {
        blueprintFactory_.reset();
        physicsSystem_.reset();
        entitySystem_.reset();
    }

    std::unique_ptr<IEntitySystem> entitySystem_;
    std::unique_ptr<IPhysicsSystem> physicsSystem_;
    std::unique_ptr<IBlueprintFactory> blueprintFactory_;
};

TEST_F(BlueprintFactoryTest, LoadSimpleBlueprint) {
    const char* lua = R"(
        Blueprints = {
            TestEntity = {
                components = {}
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("TestEntity"));
}

TEST_F(BlueprintFactoryTest, LoadMultipleBlueprints) {
    const char* lua = R"(
        Blueprints = {
            EntityA = { components = {} },
            EntityB = { components = {} },
            EntityC = { components = {} }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    auto names = blueprintFactory_->getBlueprintNames();
    EXPECT_EQ(names.size(), 3);
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("EntityA"));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("EntityB"));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("EntityC"));
}

TEST_F(BlueprintFactoryTest, CreateEntityFromBlueprint) {
    const char* lua = R"(
        Blueprints = {
            TestEntity = {
                components = {}
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("TestEntity", 100.0f, 200.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));

    // Should have Transform2D
    EXPECT_TRUE(entitySystem_->allOf<Transform2D>(entity));

    auto& transform = entitySystem_->get<Transform2D>(entity);
    EXPECT_FLOAT_EQ(transform.x, 100.0f);
    EXPECT_FLOAT_EQ(transform.y, 200.0f);
}

TEST_F(BlueprintFactoryTest, CreateEntityWithSize) {
    const char* lua = R"(
        Blueprints = {
            Platform = {
                components = {},
                physics = {
                    type = "static"
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Platform", 50.0f, 100.0f, 200.0f, 20.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));

    // Should have physics body
    EXPECT_TRUE(physicsSystem_->hasBody(entity));
}

TEST_F(BlueprintFactoryTest, CreateEntityWithDebugRect) {
    const char* lua = R"(
        Blueprints = {
            Box = {
                components = {
                    DebugRect = {
                        size = {64, 64},
                        fillColor = {255, 0, 0, 255},
                        layer = 10
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Box", 0.0f, 0.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));
    EXPECT_TRUE(entitySystem_->allOf<DebugRect>(entity));

    auto& rect = entitySystem_->get<DebugRect>(entity);
    EXPECT_FLOAT_EQ(rect.size.x, 64.0f);
    EXPECT_FLOAT_EQ(rect.size.y, 64.0f);
    EXPECT_EQ(rect.fillColor.r, 255);
    EXPECT_EQ(rect.fillColor.g, 0);
    EXPECT_EQ(rect.fillColor.b, 0);
    EXPECT_EQ(rect.layer, 10);
}

TEST_F(BlueprintFactoryTest, CreateEntityWithDebugCircle) {
    const char* lua = R"(
        Blueprints = {
            Ball = {
                components = {
                    DebugCircle = {
                        radius = 32,
                        fillColor = {0, 255, 0, 255},
                        segments = 24
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Ball", 0.0f, 0.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));
    EXPECT_TRUE(entitySystem_->allOf<DebugCircle>(entity));

    auto& circle = entitySystem_->get<DebugCircle>(entity);
    EXPECT_FLOAT_EQ(circle.radius, 32.0f);
    EXPECT_EQ(circle.fillColor.g, 255);
    EXPECT_EQ(circle.segments, 24);
}

TEST_F(BlueprintFactoryTest, InheritanceWorks) {
    const char* lua = R"(
        Blueprints = {
            BaseEntity = {
                components = {
                    DebugRect = {
                        size = {32, 32},
                        fillColor = {128, 128, 128, 255}
                    }
                }
            },
            DerivedEntity = {
                inherits = "BaseEntity",
                components = {
                    DebugRect = {
                        fillColor = {255, 0, 0, 255}
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity derived = blueprintFactory_->create("DerivedEntity", 0.0f, 0.0f);
    EXPECT_TRUE(entitySystem_->allOf<DebugRect>(derived));

    auto& rect = entitySystem_->get<DebugRect>(derived);
    // Should have parent's size
    EXPECT_FLOAT_EQ(rect.size.x, 32.0f);
    EXPECT_FLOAT_EQ(rect.size.y, 32.0f);
    // Should have child's color override
    EXPECT_EQ(rect.fillColor.r, 255);
    EXPECT_EQ(rect.fillColor.g, 0);
}

TEST_F(BlueprintFactoryTest, PhysicsBodyCreation) {
    const char* lua = R"(
        Blueprints = {
            DynamicBox = {
                components = {},
                physics = {
                    type = "dynamic",
                    size = {32, 32},
                    density = 1.0,
                    friction = 0.5,
                    restitution = 0.2,
                    fixedRotation = true
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("DynamicBox", 100.0f, 200.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));
    EXPECT_TRUE(physicsSystem_->hasBody(entity));
}

TEST_F(BlueprintFactoryTest, SensorBodyCreation) {
    const char* lua = R"(
        Blueprints = {
            Trigger = {
                components = {},
                physics = {
                    type = "static",
                    size = {64, 64},
                    sensor = true
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Trigger", 0.0f, 0.0f);
    EXPECT_TRUE(physicsSystem_->hasBody(entity));
}

TEST_F(BlueprintFactoryTest, GetBlueprintDefinition) {
    const char* lua = R"(
        Blueprints = {
            TestEntity = {
                components = {
                    DebugRect = { size = {100, 50} }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    auto def = blueprintFactory_->getBlueprint("TestEntity");
    ASSERT_TRUE(def.has_value());
    EXPECT_EQ(def->name, "TestEntity");
}

TEST_F(BlueprintFactoryTest, NonExistentBlueprintReturnsNullopt) {
    auto def = blueprintFactory_->getBlueprint("DoesNotExist");
    EXPECT_FALSE(def.has_value());
}

TEST_F(BlueprintFactoryTest, HasBlueprintReturnsFalseForUnknown) {
    EXPECT_FALSE(blueprintFactory_->hasBlueprint("Unknown"));
}

TEST_F(BlueprintFactoryTest, ClearBlueprints) {
    const char* lua = R"(
        Blueprints = {
            TestEntity = { components = {} }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("TestEntity"));

    blueprintFactory_->clearBlueprints();
    EXPECT_FALSE(blueprintFactory_->hasBlueprint("TestEntity"));
    EXPECT_TRUE(blueprintFactory_->getBlueprintNames().empty());
}

TEST_F(BlueprintFactoryTest, ReloadBlueprints) {
    const char* lua = R"(
        Blueprints = {
            TestEntity = { components = {} }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    // Reload should work without error
    blueprintFactory_->reloadBlueprints();
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("TestEntity"));
}

TEST_F(BlueprintFactoryTest, ComponentRegistration) {
    // Custom component for testing
    struct CustomComponent {
        int value = 0;
    };

    blueprintFactory_->registerComponent("CustomComponent",
        [](Entity e, IEntitySystem& sys, const PropertyMap& props) {
            int val = 42;
            if (auto it = props.find("value"); it != props.end()) {
                val = std::any_cast<int>(it->second);
            }
            sys.emplace<CustomComponent>(e, val);
        });

    EXPECT_TRUE(blueprintFactory_->isComponentRegistered("CustomComponent"));
    EXPECT_FALSE(blueprintFactory_->isComponentRegistered("UnknownComponent"));
}

TEST_F(BlueprintFactoryTest, PropertyOverrides) {
    const char* lua = R"(
        Blueprints = {
            Box = {
                components = {
                    DebugRect = {
                        size = {32, 32},
                        fillColor = {128, 128, 128, 255}
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    // Create with color override
    PropertyMap overrides;
    overrides["DebugRect.fillColor.r"] = 255;
    overrides["DebugRect.fillColor.g"] = 0;
    overrides["DebugRect.fillColor.b"] = 0;

    Entity entity = blueprintFactory_->create("Box", 0.0f, 0.0f, overrides);
    EXPECT_TRUE(entitySystem_->isValid(entity));
}

TEST_F(BlueprintFactoryTest, InvalidLuaReturnsFalse) {
    const char* invalidLua = "this is not valid lua {{{";
    EXPECT_FALSE(blueprintFactory_->loadBlueprints(invalidLua));
}

TEST_F(BlueprintFactoryTest, EmptyBlueprintsTableIsValid) {
    const char* lua = "Blueprints = {}";
    EXPECT_TRUE(blueprintFactory_->loadBlueprints(lua));
    EXPECT_TRUE(blueprintFactory_->getBlueprintNames().empty());
}

}  // namespace jframe::tests
