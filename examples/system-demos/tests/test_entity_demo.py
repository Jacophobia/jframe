"""
Entity System Demo Tests

Tests that verify the Entity System demo runs correctly and
produces expected output demonstrating the full API.
"""

import pytest


class TestEntityDemo:
    """Tests for the Entity System demo."""

    def test_demo_exists(self, demo_runner):
        """Verify the demo executable exists."""
        assert demo_runner.demo_exists("entity-demo"), "entity-demo executable not found"

    def test_demo_runs_successfully(self, demo_runner):
        """Verify the demo runs and exits cleanly."""
        result = demo_runner.run_demo("entity-demo", timeout=30)
        assert result.returncode == 0, f"Demo failed with return code {result.returncode}"

    def test_entity_lifecycle_output(self, demo_runner):
        """Verify entity lifecycle API is demonstrated."""
        result = demo_runner.run_demo("entity-demo", timeout=30)
        output = result.stdout

        # Check for entity lifecycle demonstrations
        assert "createEntity" in output or "Entity Lifecycle" in output, \
            "Entity creation not demonstrated"
        assert "destroyEntity" in output or "destroy" in output.lower(), \
            "Entity destruction not demonstrated"
        assert "isValid" in output or "valid" in output.lower(), \
            "Entity validation not demonstrated"
        assert "entityCount" in output or "count" in output.lower(), \
            "Entity count not demonstrated"

    def test_component_access_output(self, demo_runner):
        """Verify component access API is demonstrated."""
        result = demo_runner.run_demo("entity-demo", timeout=30)
        output = result.stdout

        # Check for component operations
        assert "emplace" in output.lower() or "component" in output.lower(), \
            "Component emplacement not demonstrated"

    def test_entity_queries_output(self, demo_runner):
        """Verify entity query API is demonstrated."""
        result = demo_runner.run_demo("entity-demo", timeout=30)
        output = result.stdout

        # Check for query demonstrations
        assert "view" in output.lower() or "query" in output.lower(), \
            "Entity queries not demonstrated"

    def test_no_errors_in_output(self, demo_runner):
        """Verify no error messages in output."""
        result = demo_runner.run_demo("entity-demo", timeout=30)
        output = result.stdout + result.stderr

        # Check for common error indicators
        assert "error" not in output.lower() or "error handling" in output.lower(), \
            f"Errors found in output: {output}"
        assert "exception" not in output.lower(), \
            f"Exceptions found in output: {output}"
        assert "segfault" not in output.lower(), \
            f"Segfault found in output: {output}"
