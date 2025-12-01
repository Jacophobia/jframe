"""
Graphics System Demo Tests

Tests that verify the Graphics System demo runs correctly,
produces expected output, and renders correctly (visual tests).
"""

import time
from pathlib import Path
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

    @pytest.mark.visual
    @pytest.mark.requires_display
    def test_texture_on_screen(self, demo_runner, screenshot_capture, texture_finder):
        """
        Visual test: Verify a specific texture appears on screen.

        This demonstrates how to check for a specific image/texture
        being rendered by the demo.
        """
        process = demo_runner.start_demo("graphics-demo")

        try:
            time.sleep(2.0)

            screenshot_path = screenshot_capture.capture_window(
                "graphics-demo",
                "graphics_texture_test"
            )

            if screenshot_path:
                # Example: Look for a player texture
                # Replace with actual texture path when testing real assets
                texture_path = Path(__file__).parent.parent / "graphics-demo" / "data" / "player.png"

                if texture_path.exists():
                    match = texture_finder.find_texture(
                        screenshot_path,
                        texture_path,
                        threshold=0.8
                    )
                    assert match.found, \
                        f"Player texture not found (best confidence: {match.confidence:.2%})"
                    assert match.center is not None, "Texture location not determined"
        finally:
            demo_runner.stop_demo()

    @pytest.mark.visual
    @pytest.mark.requires_display
    def test_multiple_textures(self, demo_runner, screenshot_capture, texture_finder):
        """
        Visual test: Find multiple instances of a texture (e.g., coins, enemies).
        """
        process = demo_runner.start_demo("graphics-demo")

        try:
            time.sleep(2.0)

            screenshot_path = screenshot_capture.capture_window(
                "graphics-demo",
                "graphics_multi_texture_test"
            )

            if screenshot_path:
                # Example: Look for collectible coins
                texture_path = Path(__file__).parent.parent / "graphics-demo" / "data" / "coin.png"

                if texture_path.exists():
                    matches = texture_finder.find_texture_multiple(
                        screenshot_path,
                        texture_path,
                        threshold=0.75,
                        min_distance=20  # At least 20px apart
                    )
                    # Verify expected number of coins
                    assert matches.count >= 3, \
                        f"Expected at least 3 coins, found {matches.count}"
        finally:
            demo_runner.stop_demo()

    @pytest.mark.visual
    @pytest.mark.requires_display
    def test_scaled_texture(self, demo_runner, screenshot_capture, texture_finder):
        """
        Visual test: Find a texture that may be rendered at different scale.
        """
        process = demo_runner.start_demo("graphics-demo")

        try:
            time.sleep(2.0)

            screenshot_path = screenshot_capture.capture_window(
                "graphics-demo",
                "graphics_scaled_test"
            )

            if screenshot_path:
                texture_path = Path(__file__).parent.parent / "graphics-demo" / "data" / "enemy.png"

                if texture_path.exists():
                    match = texture_finder.find_texture_scaled(
                        screenshot_path,
                        texture_path,
                        min_scale=0.5,
                        max_scale=2.0,
                        threshold=0.7
                    )
                    if match.found:
                        print(f"Found enemy at scale, size: {match.size}")
        finally:
            demo_runner.stop_demo()

    @pytest.mark.visual
    @pytest.mark.requires_display
    def test_color_regions(self, demo_runner, screenshot_capture, texture_finder):
        """
        Visual test: Find regions of a specific color (e.g., health bar, UI elements).
        """
        process = demo_runner.start_demo("graphics-demo")

        try:
            time.sleep(2.0)

            screenshot_path = screenshot_capture.capture_window(
                "graphics-demo",
                "graphics_color_test"
            )

            if screenshot_path:
                # Look for red regions (e.g., health bar when damaged)
                # Note: OpenCV uses BGR, so red is (0, 0, 255)
                red_regions = texture_finder.find_color_region(
                    screenshot_path,
                    color_bgr=(0, 0, 255),  # Red in BGR
                    tolerance=30,
                    min_area=50
                )

                # Look for green regions (e.g., health bar, safe zones)
                green_regions = texture_finder.find_color_region(
                    screenshot_path,
                    color_bgr=(0, 255, 0),  # Green in BGR
                    tolerance=30,
                    min_area=50
                )

                print(f"Found {len(red_regions)} red regions, {len(green_regions)} green regions")
        finally:
            demo_runner.stop_demo()

    def test_no_errors_in_output(self, demo_runner):
        """Verify no error messages in output."""
        result = demo_runner.run_demo("graphics-demo", timeout=30)
        output = result.stdout + result.stderr

        assert "error" not in output.lower() or "error handling" in output.lower(), \
            f"Errors found in output: {output}"
