// tests/unit/BlueprintFactoryTests.cpp
// Blueprint Factory unit tests

#include <gtest/gtest.h>

import bestow.blueprints;
import bestow.blueprints.impl;
import bestow.entity;
import bestow.entity.impl;
import bestow.physics;
import bestow.physics.impl;
import bestow.types;
import std;

namespace bestow::tests {

class BlueprintFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        entitySystem_ = std::make_unique<EntitySystem>();

        auto physicsImpl = std::make_unique<Box2DPhysicsSystem>();
        physicsImpl->initialize();
        physicsSystem_ = std::move(physicsImpl);

        blueprintFactory_ = std::make_unique<BlueprintFactory>(*entitySystem_, physicsSystem_.get());
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

// ============================================================================
// Additional Coverage Tests
// ============================================================================

// Test create() with size and overrides (4th overload)
TEST_F(BlueprintFactoryTest, CreateWithSizeAndOverrides) {
    const char* lua = R"(
        Blueprints = {
            ConfigurableBox = {
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

    PropertyMap overrides;
    overrides["DebugRect.fillColor.r"] = 255;
    overrides["DebugRect.fillColor.g"] = 255;
    overrides["DebugRect.fillColor.b"] = 0;

    Entity entity = blueprintFactory_->create("ConfigurableBox", 10.0f, 20.0f, 64.0f, 64.0f, overrides);
    EXPECT_TRUE(entitySystem_->isValid(entity));

    auto& rect = entitySystem_->get<DebugRect>(entity);
    // Color should be overridden to yellow
    EXPECT_EQ(rect.fillColor.r, 255);
    EXPECT_EQ(rect.fillColor.g, 255);
    EXPECT_EQ(rect.fillColor.b, 0);
}

// Test creating entity from non-existent blueprint
TEST_F(BlueprintFactoryTest, CreateFromNonExistentBlueprintReturnsNullEntity) {
    Entity entity = blueprintFactory_->create("DoesNotExist", 0.0f, 0.0f);
    EXPECT_FALSE(entitySystem_->isValid(entity));
}

// Test metadata parsing and storage
TEST_F(BlueprintFactoryTest, MetadataParsingWorks) {
    const char* lua = R"(
        Blueprints = {
            Collectible = {
                components = {},
                metadata = {
                    score = 100,
                    sound = "coin.wav",
                    respawns = true
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    auto def = blueprintFactory_->getBlueprint("Collectible");
    ASSERT_TRUE(def.has_value());
    EXPECT_FALSE(def->metadata.empty());
    EXPECT_EQ(def->metadata.size(), 3);
}

// Test multi-level inheritance chain
TEST_F(BlueprintFactoryTest, MultiLevelInheritance) {
    const char* lua = R"(
        Blueprints = {
            Base = {
                components = {
                    DebugRect = {
                        size = {16, 16},
                        fillColor = {255, 255, 255, 255}
                    }
                }
            },
            Middle = {
                inherits = "Base",
                components = {
                    DebugRect = {
                        size = {32, 32}
                    }
                }
            },
            Final = {
                inherits = "Middle",
                components = {
                    DebugRect = {
                        fillColor = {0, 255, 0, 255}
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Final", 0.0f, 0.0f);
    auto& rect = entitySystem_->get<DebugRect>(entity);

    // Should have Middle's size (32, 32)
    EXPECT_FLOAT_EQ(rect.size.x, 32.0f);
    EXPECT_FLOAT_EQ(rect.size.y, 32.0f);

    // Should have Final's color (green)
    EXPECT_EQ(rect.fillColor.r, 0);
    EXPECT_EQ(rect.fillColor.g, 255);
    EXPECT_EQ(rect.fillColor.b, 0);
}

// Test inheritance with non-existent parent
TEST_F(BlueprintFactoryTest, InheritanceFromMissingParentDoesNotCrash) {
    const char* lua = R"(
        Blueprints = {
            Orphan = {
                inherits = "NonExistentParent",
                components = {
                    DebugRect = { size = {50, 50} }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    // Should still create entity without parent's properties
    Entity entity = blueprintFactory_->create("Orphan", 0.0f, 0.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));
}

// Test DebugLine component
TEST_F(BlueprintFactoryTest, CreateEntityWithDebugLine) {
    const char* lua = R"(
        Blueprints = {
            Arrow = {
                components = {
                    DebugLine = {
                        endOffset = {100, 50},
                        color = {255, 0, 0, 255},
                        thickness = 3.0,
                        layer = 5
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Arrow", 0.0f, 0.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));
    EXPECT_TRUE(entitySystem_->allOf<DebugLine>(entity));

    auto& line = entitySystem_->get<DebugLine>(entity);
    EXPECT_FLOAT_EQ(line.endOffset.x, 100.0f);
    EXPECT_FLOAT_EQ(line.endOffset.y, 50.0f);
    EXPECT_EQ(line.color.r, 255);
    EXPECT_FLOAT_EQ(line.thickness, 3.0f);
    EXPECT_EQ(line.layer, 5);
}

// Test physics with kinematic body type
TEST_F(BlueprintFactoryTest, KinematicBodyCreation) {
    const char* lua = R"(
        Blueprints = {
            MovingPlatform = {
                components = {},
                physics = {
                    type = "kinematic",
                    size = {128, 16},
                    friction = 0.8
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("MovingPlatform", 50.0f, 100.0f);
    EXPECT_TRUE(physicsSystem_->hasBody(entity));
}

// Test physics with all properties
TEST_F(BlueprintFactoryTest, PhysicsWithAllProperties) {
    const char* lua = R"(
        Blueprints = {
            ComplexBody = {
                components = {},
                physics = {
                    type = "dynamic",
                    size = {40, 40},
                    density = 2.0,
                    friction = 0.7,
                    restitution = 0.5,
                    linearDamping = 0.1,
                    fixedRotation = false,
                    sensor = false,
                    collisionLayer = "enemies"
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("ComplexBody", 0.0f, 0.0f);
    EXPECT_TRUE(physicsSystem_->hasBody(entity));
}

// Test physics without physics system (null physics)
TEST_F(BlueprintFactoryTest, PhysicsDefinitionWithoutPhysicsSystemDoesNotCrash) {
    // Create factory without physics system
    auto entitySystemNoPhysics = std::make_unique<EntitySystem>();
    auto factoryNoPhysics = std::make_unique<BlueprintFactory>(*entitySystemNoPhysics, nullptr);

    const char* lua = R"(
        Blueprints = {
            PhysicsEntity = {
                components = {},
                physics = {
                    type = "dynamic",
                    size = {32, 32}
                }
            }
        }
    )";

    ASSERT_TRUE(factoryNoPhysics->loadBlueprints(lua));

    // Should create entity but without physics body
    Entity entity = factoryNoPhysics->create("PhysicsEntity", 0.0f, 0.0f);
    EXPECT_TRUE(entitySystemNoPhysics->isValid(entity));
}

// Test clearing blueprints clears last source
TEST_F(BlueprintFactoryTest, ClearBlueprintsRemovesLastSource) {
    const char* lua = R"(
        Blueprints = {
            TestEntity = { components = {} }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));
    blueprintFactory_->clearBlueprints();

    // After clear, reload should do nothing (no source stored)
    blueprintFactory_->reloadBlueprints();
    EXPECT_FALSE(blueprintFactory_->hasBlueprint("TestEntity"));
}

// Test reload after empty clear
TEST_F(BlueprintFactoryTest, ReloadWithoutPriorLoadDoesNotCrash) {
    blueprintFactory_->reloadBlueprints();
    EXPECT_TRUE(blueprintFactory_->getBlueprintNames().empty());
}

// Test getBlueprintNames returns all names
TEST_F(BlueprintFactoryTest, GetBlueprintNamesReturnsAllNames) {
    const char* lua = R"(
        Blueprints = {
            Alpha = { components = {} },
            Beta = { components = {} },
            Gamma = { components = {} }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    auto names = blueprintFactory_->getBlueprintNames();
    EXPECT_EQ(names.size(), 3);

    // Should contain all three names
    bool hasAlpha = std::find(names.begin(), names.end(), "Alpha") != names.end();
    bool hasBeta = std::find(names.begin(), names.end(), "Beta") != names.end();
    bool hasGamma = std::find(names.begin(), names.end(), "Gamma") != names.end();

    EXPECT_TRUE(hasAlpha);
    EXPECT_TRUE(hasBeta);
    EXPECT_TRUE(hasGamma);
}

// Test loading blueprints replaces old ones
TEST_F(BlueprintFactoryTest, LoadingNewBlueprintsReplacesOld) {
    const char* lua1 = R"(
        Blueprints = {
            OldEntity = { components = {} }
        }
    )";

    const char* lua2 = R"(
        Blueprints = {
            NewEntity = { components = {} }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua1));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("OldEntity"));

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua2));
    EXPECT_FALSE(blueprintFactory_->hasBlueprint("OldEntity"));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("NewEntity"));
}

// Test Lua syntax error handling
TEST_F(BlueprintFactoryTest, MalformedLuaSyntaxReturnsFalse) {
    const char* badLua = R"(
        Blueprints = {
            Broken = { components = {} }  -- Missing closing brace
    )";

    EXPECT_FALSE(blueprintFactory_->loadBlueprints(badLua));
}

// Test missing Blueprints table
TEST_F(BlueprintFactoryTest, MissingBlueprintsTableReturnsFalse) {
    const char* lua = R"(
        SomeOtherTable = {
            Data = "value"
        }
    )";

    EXPECT_FALSE(blueprintFactory_->loadBlueprints(lua));
}

// Test Lua with return value instead of global table
TEST_F(BlueprintFactoryTest, BlueprintsReturnedFromScriptWorks) {
    const char* lua = R"(
        return {
            ReturnedEntity = {
                components = {
                    DebugRect = { size = {64, 64} }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("ReturnedEntity"));
}

// Test component with unknown type is silently ignored
TEST_F(BlueprintFactoryTest, UnknownComponentTypeIsIgnored) {
    const char* lua = R"(
        Blueprints = {
            Entity = {
                components = {
                    UnknownComponent = {
                        someProperty = 123
                    },
                    DebugRect = {
                        size = {32, 32}
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Entity", 0.0f, 0.0f);
    EXPECT_TRUE(entitySystem_->isValid(entity));

    // DebugRect should be created
    EXPECT_TRUE(entitySystem_->allOf<DebugRect>(entity));
}

// Test physics size override with explicit values
TEST_F(BlueprintFactoryTest, PhysicsSizeOverrideWithExplicitSize) {
    const char* lua = R"(
        Blueprints = {
            Box = {
                components = {
                    DebugRect = { size = {100, 100} }
                },
                physics = {
                    type = "static",
                    size = {50, 50}
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Box", 0.0f, 0.0f);
    EXPECT_TRUE(physicsSystem_->hasBody(entity));

    // Physics body should use physics.size, not component size
    Vec2 bodySize = physicsSystem_->getBodySize(entity);
    EXPECT_FLOAT_EQ(bodySize.x, 50.0f);
    EXPECT_FLOAT_EQ(bodySize.y, 50.0f);
}

// Test physics inherits size from DebugRect when not specified
TEST_F(BlueprintFactoryTest, PhysicsInheritsSizeFromDebugRect) {
    const char* lua = R"(
        Blueprints = {
            Box = {
                components = {
                    DebugRect = { size = {80, 60} }
                },
                physics = {
                    type = "static"
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    // Create without explicit size
    Entity entity = blueprintFactory_->create("Box", 0.0f, 0.0f);
    EXPECT_TRUE(physicsSystem_->hasBody(entity));

    Vec2 bodySize = physicsSystem_->getBodySize(entity);
    // Should inherit from DebugRect
    EXPECT_FLOAT_EQ(bodySize.x, 80.0f);
    EXPECT_FLOAT_EQ(bodySize.y, 60.0f);
}

// Test DebugRect syncs to physics body size after creation
TEST_F(BlueprintFactoryTest, DebugRectSyncsToPhysicsBodySize) {
    const char* lua = R"(
        Blueprints = {
            Box = {
                components = {
                    DebugRect = { size = {10, 10} }  -- Small initial size
                },
                physics = {
                    type = "static",
                    size = {100, 200}  -- Larger physics size
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Box", 0.0f, 0.0f);

    // DebugRect should be synced to physics body size
    auto& rect = entitySystem_->get<DebugRect>(entity);
    EXPECT_FLOAT_EQ(rect.size.x, 100.0f);
    EXPECT_FLOAT_EQ(rect.size.y, 200.0f);
}

// Test DebugCircle syncs to physics body size
TEST_F(BlueprintFactoryTest, DebugCircleSyncsToPhysicsBodySize) {
    const char* lua = R"(
        Blueprints = {
            Ball = {
                components = {
                    DebugCircle = { radius = 5 }
                },
                physics = {
                    type = "dynamic",
                    size = {40, 40}
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Ball", 0.0f, 0.0f);

    // DebugCircle radius should be synced (min of width/height / 2)
    auto& circle = entitySystem_->get<DebugCircle>(entity);
    EXPECT_FLOAT_EQ(circle.radius, 20.0f);  // 40 / 2
}

// Test component registration overwrite
TEST_F(BlueprintFactoryTest, ComponentRegistrationCanBeOverwritten) {
    struct TestComponent {
        int value = 0;
    };

    // Register once
    blueprintFactory_->registerComponent("TestComp",
        [](Entity e, IEntitySystem& sys, const PropertyMap& props) {
            sys.emplace<TestComponent>(e, TestComponent{1});
        });

    EXPECT_TRUE(blueprintFactory_->isComponentRegistered("TestComp"));

    // Register again with different behavior (should overwrite)
    blueprintFactory_->registerComponent("TestComp",
        [](Entity e, IEntitySystem& sys, const PropertyMap& props) {
            sys.emplace<TestComponent>(e, TestComponent{2});
        });

    EXPECT_TRUE(blueprintFactory_->isComponentRegistered("TestComp"));
}

// Test nested property tables in metadata
TEST_F(BlueprintFactoryTest, NestedPropertyTablesInMetadata) {
    const char* lua = R"(
        Blueprints = {
            Entity = {
                components = {},
                metadata = {
                    config = {
                        speed = 100,
                        jumpHeight = 200
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    auto def = blueprintFactory_->getBlueprint("Entity");
    ASSERT_TRUE(def.has_value());
    EXPECT_FALSE(def->metadata.empty());
}

// Test all DebugRect properties
TEST_F(BlueprintFactoryTest, DebugRectAllProperties) {
    const char* lua = R"(
        Blueprints = {
            StyledBox = {
                components = {
                    DebugRect = {
                        width = 100,
                        height = 50,
                        fillColor = {255, 128, 0, 200},
                        outlineColor = {0, 0, 0, 255},
                        outlineWidth = 2.5,
                        layer = 15,
                        filled = false
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("StyledBox", 0.0f, 0.0f);
    auto& rect = entitySystem_->get<DebugRect>(entity);

    EXPECT_FLOAT_EQ(rect.size.x, 100.0f);
    EXPECT_FLOAT_EQ(rect.size.y, 50.0f);
    EXPECT_EQ(rect.fillColor.r, 255);
    EXPECT_EQ(rect.fillColor.g, 128);
    EXPECT_EQ(rect.fillColor.b, 0);
    EXPECT_EQ(rect.fillColor.a, 200);
    EXPECT_EQ(rect.outlineColor.r, 0);
    EXPECT_FLOAT_EQ(rect.outlineWidth, 2.5f);
    EXPECT_EQ(rect.layer, 15);
    EXPECT_FALSE(rect.filled);
}

// Test all DebugCircle properties
TEST_F(BlueprintFactoryTest, DebugCircleAllProperties) {
    const char* lua = R"(
        Blueprints = {
            StyledCircle = {
                components = {
                    DebugCircle = {
                        radius = 50,
                        fillColor = {0, 255, 255, 128},
                        outlineColor = {255, 0, 255, 255},
                        outlineWidth = 3.0,
                        layer = 20,
                        filled = false,
                        segments = 16
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("StyledCircle", 0.0f, 0.0f);
    auto& circle = entitySystem_->get<DebugCircle>(entity);

    EXPECT_FLOAT_EQ(circle.radius, 50.0f);
    EXPECT_EQ(circle.fillColor.g, 255);
    EXPECT_EQ(circle.fillColor.b, 255);
    EXPECT_EQ(circle.fillColor.a, 128);
    EXPECT_EQ(circle.outlineColor.r, 255);
    EXPECT_FLOAT_EQ(circle.outlineWidth, 3.0f);
    EXPECT_EQ(circle.layer, 20);
    EXPECT_FALSE(circle.filled);
    EXPECT_EQ(circle.segments, 16);
}

// Test color with missing alpha defaults to 255
TEST_F(BlueprintFactoryTest, ColorWithoutAlphaDefaultsTo255) {
    const char* lua = R"(
        Blueprints = {
            Box = {
                components = {
                    DebugRect = {
                        size = {32, 32},
                        fillColor = {255, 0, 0}  -- RGB only, no alpha
                    }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Box", 0.0f, 0.0f);
    auto& rect = entitySystem_->get<DebugRect>(entity);

    EXPECT_EQ(rect.fillColor.a, 255);
}

// Test sandbox prevents dangerous functions
TEST_F(BlueprintFactoryTest, SandboxBlocksDangerousFunctions) {
    const char* lua = R"(
        -- Try to use os functions (should be blocked)
        if os then
            error("os should be blocked")
        end

        Blueprints = {
            Safe = { components = {} }
        }
    )";

    EXPECT_TRUE(blueprintFactory_->loadBlueprints(lua));
    EXPECT_TRUE(blueprintFactory_->hasBlueprint("Safe"));
}

// Test inheritance preserves physics when child doesn't override
TEST_F(BlueprintFactoryTest, InheritancePreservesPhysics) {
    const char* lua = R"(
        Blueprints = {
            Parent = {
                components = {},
                physics = {
                    type = "static",
                    size = {64, 64}
                }
            },
            Child = {
                inherits = "Parent",
                components = {
                    DebugRect = { size = {32, 32} }
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Child", 0.0f, 0.0f);
    EXPECT_TRUE(physicsSystem_->hasBody(entity));
}

// Test child physics overrides parent physics
TEST_F(BlueprintFactoryTest, ChildPhysicsOverridesParent) {
    const char* lua = R"(
        Blueprints = {
            Parent = {
                components = {},
                physics = {
                    type = "static",
                    size = {64, 64}
                }
            },
            Child = {
                inherits = "Parent",
                components = {},
                physics = {
                    type = "dynamic",
                    size = {32, 32}
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("Child", 0.0f, 0.0f);
    Vec2 bodySize = physicsSystem_->getBodySize(entity);

    // Should use child's physics size, not parent's
    EXPECT_FLOAT_EQ(bodySize.x, 32.0f);
    EXPECT_FLOAT_EQ(bodySize.y, 32.0f);
}

// Test Transform2D is always added
TEST_F(BlueprintFactoryTest, Transform2DAlwaysAdded) {
    const char* lua = R"(
        Blueprints = {
            MinimalEntity = {
                components = {}
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    Entity entity = blueprintFactory_->create("MinimalEntity", 123.0f, 456.0f);
    EXPECT_TRUE(entitySystem_->allOf<Transform2D>(entity));

    auto& transform = entitySystem_->get<Transform2D>(entity);
    EXPECT_FLOAT_EQ(transform.x, 123.0f);
    EXPECT_FLOAT_EQ(transform.y, 456.0f);
    EXPECT_FLOAT_EQ(transform.rotation, 0.0f);
    EXPECT_FLOAT_EQ(transform.scaleX, 1.0f);
    EXPECT_FLOAT_EQ(transform.scaleY, 1.0f);
}

// Test default physics size when no component or size specified
TEST_F(BlueprintFactoryTest, DefaultPhysicsSizeUsedWhenNoSizeAvailable) {
    const char* lua = R"(
        Blueprints = {
            NoSizeEntity = {
                components = {},
                physics = {
                    type = "dynamic"
                }
            }
        }
    )";

    ASSERT_TRUE(blueprintFactory_->loadBlueprints(lua));

    // Create without size, and blueprint has no DebugRect
    Entity entity = blueprintFactory_->create("NoSizeEntity", 0.0f, 0.0f);
    EXPECT_TRUE(physicsSystem_->hasBody(entity));

    Vec2 bodySize = physicsSystem_->getBodySize(entity);
    // Should use default size of 32x32
    EXPECT_FLOAT_EQ(bodySize.x, 32.0f);
    EXPECT_FLOAT_EQ(bodySize.y, 32.0f);
}

}  // namespace bestow::tests
