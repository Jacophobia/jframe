"""
Graphics System Demo Tests

Tests that verify the Graphics System demo runs correctly,
produces expected output, and renders correctly (visual tests).
"""

import time
import pytest


class TestGraphicsDemo:
    """Tests for the Graphics System demo."""

    def test_demo_exists(self, demo_runner):
        """Verify the demo executable exists."""
        assert demo_runner.demo_exists("graphics-demo"), "graphics-demo executable not found"

    def test_demo_runs_successfully(self, demo_runner):
        """Verify the demo runs and exits cleanly."""
        result = demo_runner.run_demo("graphics-demo", timeout=30)
        assert result.returncode == 0, f"Demo failed with return code {result.returncode}"

    def test_rendering_api_output(self, demo_runner):
        """Verify rendering API methods are demonstrated."""
        result = demo_runner.run_demo("graphics-demo", timeout=30)
        output = result.stdout

        # Check for rendering demonstrations
        rendering_features = [
            "draw" in output.lower(),
            "sprite" in output.lower(),
            "rect" in output.lower(),
            "circle" in output.lower(),
            "line" in output.lower(),
        ]
        assert sum(rendering_features) >= 3, "Not enough rendering features demonstrated"

    def test_camera_api_output(self, demo_runner):
        """Verify camera API is demonstrated."""
        result = demo_runner.run_demo("graphics-demo", timeout=30)
        output = result.stdout

        assert "camera" in output.lower(), "Camera API not demonstrated"

    def test_window_api_output(self, demo_runner):
        """Verify window management API is demonstrated."""
        result = demo_runner.run_demo("graphics-demo", timeout=30)
        output = result.stdout

        assert "window" in output.lower() or "viewport" in output.lower(), \
            "Window API not demonstrated"

    @pytest.mark.visual
    @pytest.mark.requires_display
    def test_visual_output(self, demo_runner, screenshot_capture):
        """
        Visual test: Verify the rendered output matches expected.

        This test requires a display and captures a screenshot for comparison.
        """
        # Start the demo in background mode
        process = demo_runner.start_demo("graphics-demo")

        try:
            # Wait for window to appear and render
            time.sleep(2.0)

            # Capture screenshot
            screenshot_path = screenshot_capture.capture_window(
                "graphics-demo",
                "graphics_demo_output"
            )

            if screenshot_path:
                # Compare against reference
                passed, diff = screenshot_capture.compare_images(
                    screenshot_path,
                    "graphics_demo_reference",
                    threshold=0.1  # 10% difference allowed
                )
                assert passed, f"Visual comparison failed with {diff:.2%} difference"
        finally:
            demo_runner.stop_demo()

    def test_no_errors_in_output(self, demo_runner):
        """Verify no error messages in output."""
        result = demo_runner.run_demo("graphics-demo", timeout=30)
        output = result.stdout + result.stderr

        assert "error" not in output.lower() or "error handling" in output.lower(), \
            f"Errors found in output: {output}"
