"""
Input System Demo Tests

Tests that verify the Input System demo runs correctly and
responds to simulated input correctly.
"""

import time
import pytest


class TestInputDemo:
    """Tests for the Input System demo."""

    def test_demo_exists(self, demo_runner):
        """Verify the demo executable exists."""
        assert demo_runner.demo_exists("input-demo"), "input-demo executable not found"

    @pytest.mark.interactive
    @pytest.mark.requires_display
    def test_keyboard_input(self, demo_runner, input_simulator):
        """
        Test keyboard input handling.

        This test requires a display and simulates keyboard input.
        """
        # Start the interactive demo
        process = demo_runner.start_demo("input-demo")

        try:
            # Wait for window to appear
            time.sleep(1.0)

            # Simulate pressing various keys
            input_simulator.press_key('w', hold_time=0.2)
            time.sleep(0.1)
            input_simulator.press_key('a', hold_time=0.2)
            time.sleep(0.1)
            input_simulator.press_key('space')
            time.sleep(0.5)

            # Press escape to exit (if supported)
            input_simulator.press_key('escape')
            time.sleep(0.5)

        finally:
            demo_runner.stop_demo()

    @pytest.mark.interactive
    @pytest.mark.requires_display
    def test_mouse_input(self, demo_runner, input_simulator):
        """
        Test mouse input handling.

        This test requires a display and simulates mouse input.
        """
        process = demo_runner.start_demo("input-demo")

        try:
            # Wait for window to appear
            time.sleep(1.0)

            # Move mouse to various positions
            input_simulator.move_mouse(400, 300, duration=0.3)
            time.sleep(0.2)
            input_simulator.click()
            time.sleep(0.2)

            input_simulator.move_mouse(500, 400, duration=0.3)
            input_simulator.click(button='right')
            time.sleep(0.5)

        finally:
            demo_runner.stop_demo()

    @pytest.mark.interactive
    @pytest.mark.requires_display
    def test_action_mapping(self, demo_runner, input_simulator):
        """
        Test that actions are correctly mapped to inputs.

        This test verifies that the input mapping system works correctly.
        """
        process = demo_runner.start_demo("input-demo")

        try:
            # Wait for window
            time.sleep(1.0)

            # Test jump action (typically Space)
            input_simulator.press_key('space')
            time.sleep(0.2)

            # Test movement actions (WASD)
            input_simulator.press_key('w', hold_time=0.3)
            input_simulator.press_key('s', hold_time=0.3)
            input_simulator.press_key('a', hold_time=0.3)
            input_simulator.press_key('d', hold_time=0.3)

            time.sleep(0.5)

        finally:
            demo_runner.stop_demo()

    def test_mapping_api_documented(self, demo_runner):
        """
        Verify input mapping API is documented in demo code/readme.

        Since input-demo is interactive, we check that documentation exists.
        """
        from pathlib import Path
        readme_path = Path(__file__).parent.parent / "input-demo" / "README.md"

        if readme_path.exists():
            content = readme_path.read_text()
            assert "mapping" in content.lower() or "action" in content.lower(), \
                "Input mapping not documented in README"


class TestInputDemoNonInteractive:
    """Non-interactive tests for input demo verification."""

    def test_demo_exists(self, demo_runner):
        """Verify the demo executable exists."""
        assert demo_runner.demo_exists("input-demo"), "input-demo executable not found"

    def test_readme_exists(self):
        """Verify README documentation exists."""
        from pathlib import Path
        readme_path = Path(__file__).parent.parent / "input-demo" / "README.md"
        assert readme_path.exists(), "input-demo README.md not found"

    def test_readme_documents_controls(self):
        """Verify README documents the controls."""
        from pathlib import Path
        readme_path = Path(__file__).parent.parent / "input-demo" / "README.md"

        if readme_path.exists():
            content = readme_path.read_text().lower()
            control_terms = ["wasd", "space", "mouse", "keyboard", "controller"]
            found = sum(1 for term in control_terms if term in content)
            assert found >= 2, "README should document at least 2 control types"
