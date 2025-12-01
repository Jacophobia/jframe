"""
Comprehensive Tests for All System Demos

Tests that verify all system demos run correctly and produce expected output.
This file provides a quick way to verify all demos are functional.
"""

import pytest


# List of all demo executables
ALL_DEMOS = [
    "entity-demo",
    "events-demo",
    "config-demo",
    "assets-demo",
    "input-demo",
    "save-demo",
    "graphics-demo",
    "audio-demo",
    "physics-demo",
    "level-demo",
    "ai-demo",
    "camera-demo",
    "gas-demo",
    "blueprints-demo",
]

# Non-interactive demos (can run to completion without user input)
NON_INTERACTIVE_DEMOS = [
    "entity-demo",
    "events-demo",
    "config-demo",
    "assets-demo",
    "save-demo",
    "audio-demo",
    "physics-demo",
    "level-demo",
    "ai-demo",
    "camera-demo",
    "gas-demo",
    "blueprints-demo",
]

# Interactive demos (require display/input)
INTERACTIVE_DEMOS = [
    "input-demo",
    "graphics-demo",
]


class TestAllDemosExist:
    """Verify all demo executables exist."""

    @pytest.mark.parametrize("demo_name", ALL_DEMOS)
    def test_demo_exists(self, demo_runner, demo_name):
        """Verify each demo executable exists."""
        assert demo_runner.demo_exists(demo_name), f"{demo_name} executable not found"


class TestNonInteractiveDemos:
    """Run all non-interactive demos and verify they complete successfully."""

    @pytest.mark.parametrize("demo_name", NON_INTERACTIVE_DEMOS)
    def test_demo_runs_successfully(self, demo_runner, demo_name):
        """Verify each non-interactive demo runs and exits cleanly."""
        result = demo_runner.run_demo(demo_name, timeout=60)
        assert result.returncode == 0, \
            f"{demo_name} failed with return code {result.returncode}\n" \
            f"stdout: {result.stdout[:500]}\n" \
            f"stderr: {result.stderr[:500]}"

    @pytest.mark.parametrize("demo_name", NON_INTERACTIVE_DEMOS)
    def test_demo_no_crashes(self, demo_runner, demo_name):
        """Verify demos don't crash with segfaults or exceptions."""
        result = demo_runner.run_demo(demo_name, timeout=60)
        combined_output = result.stdout + result.stderr

        crash_indicators = [
            "segmentation fault",
            "segfault",
            "sigsegv",
            "sigabrt",
            "abort",
            "core dumped",
            "terminate called",
            "uncaught exception",
        ]

        for indicator in crash_indicators:
            assert indicator not in combined_output.lower(), \
                f"{demo_name} appears to have crashed: found '{indicator}'"


class TestConfigDemo:
    """Specific tests for Config System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("config-demo")

    def test_loads_lua_config(self, demo_runner):
        """Verify Lua config loading is demonstrated."""
        result = demo_runner.run_demo("config-demo", timeout=30)
        output = result.stdout.lower()

        assert "lua" in output or "config" in output, \
            "Lua config loading not demonstrated"

    def test_demonstrates_value_types(self, demo_runner):
        """Verify different value types are demonstrated."""
        result = demo_runner.run_demo("config-demo", timeout=30)
        output = result.stdout.lower()

        type_indicators = ["float", "int", "bool", "string", "array"]
        found = sum(1 for t in type_indicators if t in output)
        assert found >= 2, "Not enough value types demonstrated"


class TestAssetsDemo:
    """Specific tests for Assets System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("assets-demo")

    def test_demonstrates_asset_types(self, demo_runner):
        """Verify different asset types are demonstrated."""
        result = demo_runner.run_demo("assets-demo", timeout=30)
        output = result.stdout.lower()

        asset_types = ["texture", "sound", "font", "level"]
        found = sum(1 for t in asset_types if t in output)
        assert found >= 2, "Not enough asset types demonstrated"

    def test_demonstrates_loading(self, demo_runner):
        """Verify asset loading is demonstrated."""
        result = demo_runner.run_demo("assets-demo", timeout=30)
        output = result.stdout.lower()

        assert "load" in output, "Asset loading not demonstrated"


class TestSaveDemo:
    """Specific tests for Save System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("save-demo")

    def test_demonstrates_save_load(self, demo_runner):
        """Verify save/load operations are demonstrated."""
        result = demo_runner.run_demo("save-demo", timeout=30)
        output = result.stdout.lower()

        assert "save" in output and "load" in output, \
            "Save/load operations not demonstrated"

    def test_demonstrates_serialization(self, demo_runner):
        """Verify serialization is demonstrated."""
        result = demo_runner.run_demo("save-demo", timeout=30)
        output = result.stdout.lower()

        assert "serialize" in output or "archive" in output or "slot" in output, \
            "Serialization not demonstrated"


class TestAudioDemo:
    """Specific tests for Audio System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("audio-demo")

    def test_demonstrates_channels(self, demo_runner):
        """Verify channel-based audio is demonstrated."""
        result = demo_runner.run_demo("audio-demo", timeout=30)
        output = result.stdout.lower()

        assert "channel" in output, "Channel audio not demonstrated"

    def test_demonstrates_3d_audio(self, demo_runner):
        """Verify 3D positional audio is demonstrated."""
        result = demo_runner.run_demo("audio-demo", timeout=30)
        output = result.stdout.lower()

        assert "positional" in output or "3d" in output or "listener" in output, \
            "3D audio not demonstrated"


class TestPhysicsDemo:
    """Specific tests for Physics System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("physics-demo")

    def test_demonstrates_bodies(self, demo_runner):
        """Verify physics body creation is demonstrated."""
        result = demo_runner.run_demo("physics-demo", timeout=30)
        output = result.stdout.lower()

        assert "body" in output or "physics" in output, \
            "Physics bodies not demonstrated"

    def test_demonstrates_collision(self, demo_runner):
        """Verify collision detection is demonstrated."""
        result = demo_runner.run_demo("physics-demo", timeout=30)
        output = result.stdout.lower()

        assert "collision" in output or "raycast" in output, \
            "Collision detection not demonstrated"


class TestLevelDemo:
    """Specific tests for Level System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("level-demo")

    def test_demonstrates_level_loading(self, demo_runner):
        """Verify level loading is demonstrated."""
        result = demo_runner.run_demo("level-demo", timeout=30)
        output = result.stdout.lower()

        assert "level" in output and "load" in output, \
            "Level loading not demonstrated"

    def test_demonstrates_spawn_points(self, demo_runner):
        """Verify spawn points are demonstrated."""
        result = demo_runner.run_demo("level-demo", timeout=30)
        output = result.stdout.lower()

        assert "spawn" in output or "entity" in output, \
            "Spawn points not demonstrated"


class TestAIDemo:
    """Specific tests for AI System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("ai-demo")

    def test_demonstrates_behavior_trees(self, demo_runner):
        """Verify behavior trees are demonstrated."""
        result = demo_runner.run_demo("ai-demo", timeout=30)
        output = result.stdout.lower()

        assert "behavior" in output or "blackboard" in output, \
            "Behavior trees not demonstrated"

    def test_demonstrates_navigation(self, demo_runner):
        """Verify navigation/pathfinding is demonstrated."""
        result = demo_runner.run_demo("ai-demo", timeout=30)
        output = result.stdout.lower()

        assert "path" in output or "nav" in output or "patrol" in output, \
            "Navigation not demonstrated"


class TestCameraDemo:
    """Specific tests for Camera System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("camera-demo")

    def test_demonstrates_following(self, demo_runner):
        """Verify camera following is demonstrated."""
        result = demo_runner.run_demo("camera-demo", timeout=30)
        output = result.stdout.lower()

        assert "target" in output or "follow" in output, \
            "Camera following not demonstrated"

    def test_demonstrates_effects(self, demo_runner):
        """Verify camera effects are demonstrated."""
        result = demo_runner.run_demo("camera-demo", timeout=30)
        output = result.stdout.lower()

        assert "shake" in output or "zoom" in output, \
            "Camera effects not demonstrated"


class TestGASDemo:
    """Specific tests for GAS (Gameplay Ability System) demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("gas-demo")

    def test_demonstrates_abilities(self, demo_runner):
        """Verify abilities are demonstrated."""
        result = demo_runner.run_demo("gas-demo", timeout=30)
        output = result.stdout.lower()

        assert "ability" in output, "Abilities not demonstrated"

    def test_demonstrates_effects(self, demo_runner):
        """Verify gameplay effects are demonstrated."""
        result = demo_runner.run_demo("gas-demo", timeout=30)
        output = result.stdout.lower()

        assert "effect" in output, "Effects not demonstrated"

    def test_demonstrates_attributes(self, demo_runner):
        """Verify attributes are demonstrated."""
        result = demo_runner.run_demo("gas-demo", timeout=30)
        output = result.stdout.lower()

        assert "attribute" in output or "health" in output or "mana" in output, \
            "Attributes not demonstrated"


class TestBlueprintsDemo:
    """Specific tests for Blueprints System demo."""

    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("blueprints-demo")

    def test_demonstrates_blueprint_loading(self, demo_runner):
        """Verify blueprint loading is demonstrated."""
        result = demo_runner.run_demo("blueprints-demo", timeout=30)
        output = result.stdout.lower()

        assert "blueprint" in output and "load" in output, \
            "Blueprint loading not demonstrated"

    def test_demonstrates_entity_creation(self, demo_runner):
        """Verify entity creation from blueprints is demonstrated."""
        result = demo_runner.run_demo("blueprints-demo", timeout=30)
        output = result.stdout.lower()

        assert "create" in output or "entity" in output, \
            "Entity creation not demonstrated"

    def test_demonstrates_inheritance(self, demo_runner):
        """Verify blueprint inheritance is demonstrated."""
        result = demo_runner.run_demo("blueprints-demo", timeout=30)
        output = result.stdout.lower()

        assert "inherit" in output or "base" in output, \
            "Blueprint inheritance not demonstrated"
