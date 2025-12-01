"""
Events System Demo Tests

Tests that verify the Events System demo runs correctly and
produces expected output demonstrating the full API.
"""

import pytest


class TestEventsDemo:
    """Tests for the Events System demo."""

    def test_demo_exists(self, demo_runner):
        """Verify the demo executable exists."""
        assert demo_runner.demo_exists("events-demo"), "events-demo executable not found"

    def test_demo_runs_successfully(self, demo_runner):
        """Verify the demo runs and exits cleanly."""
        result = demo_runner.run_demo("events-demo", timeout=30)
        assert result.returncode == 0, f"Demo failed with return code {result.returncode}"

    def test_publish_subscribe_output(self, demo_runner):
        """Verify publish/subscribe API is demonstrated."""
        result = demo_runner.run_demo("events-demo", timeout=30)
        output = result.stdout

        # Check for pub/sub demonstrations
        assert "subscribe" in output.lower() or "publish" in output.lower(), \
            "Publish/subscribe not demonstrated"

    def test_event_types_output(self, demo_runner):
        """Verify various event types are demonstrated."""
        result = demo_runner.run_demo("events-demo", timeout=30)
        output = result.stdout

        # Check for different event types
        event_types_found = sum([
            "collision" in output.lower(),
            "trigger" in output.lower(),
            "level" in output.lower(),
            "damage" in output.lower(),
        ])
        assert event_types_found >= 2, "Not enough event types demonstrated"

    def test_queue_processing_output(self, demo_runner):
        """Verify event queue processing is demonstrated."""
        result = demo_runner.run_demo("events-demo", timeout=30)
        output = result.stdout

        # Check for queue operations
        assert "queue" in output.lower() or "process" in output.lower(), \
            "Event queue processing not demonstrated"

    def test_no_errors_in_output(self, demo_runner):
        """Verify no error messages in output."""
        result = demo_runner.run_demo("events-demo", timeout=30)
        output = result.stdout + result.stderr

        assert "error" not in output.lower() or "error handling" in output.lower(), \
            f"Errors found in output: {output}"
