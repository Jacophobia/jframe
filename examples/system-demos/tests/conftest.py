"""
JFrame System Demos Test Configuration

Pytest configuration and fixtures for testing JFrame system demos.
Provides utilities for:
- Running demo executables
- Capturing screenshots
- Comparing visual output
- Simulating input
"""

import os
import subprocess
import time
from pathlib import Path
from typing import Optional, Generator

import pytest
from PIL import Image

# Determine the build directory based on platform
BUILD_DIR = Path(__file__).parent.parent.parent.parent / "build" / "macos-debug"
DEMOS_BIN_DIR = BUILD_DIR / "bin"
REFERENCE_DIR = Path(__file__).parent / "reference_images"
OUTPUT_DIR = Path(__file__).parent / "test_output"


def pytest_configure(config):
    """Create output directories if they don't exist."""
    REFERENCE_DIR.mkdir(parents=True, exist_ok=True)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


@pytest.fixture(scope="session")
def build_dir() -> Path:
    """Return the build directory path."""
    return BUILD_DIR


@pytest.fixture(scope="session")
def demos_bin_dir() -> Path:
    """Return the demos binary directory path."""
    return DEMOS_BIN_DIR


@pytest.fixture
def reference_dir() -> Path:
    """Return the reference images directory."""
    return REFERENCE_DIR


@pytest.fixture
def output_dir() -> Path:
    """Return the test output directory."""
    return OUTPUT_DIR


class DemoRunner:
    """Helper class to run and manage demo executables."""

    def __init__(self, bin_dir: Path):
        self.bin_dir = bin_dir
        self.process: Optional[subprocess.Popen] = None

    def get_demo_path(self, demo_name: str) -> Path:
        """Get the full path to a demo executable."""
        return self.bin_dir / demo_name

    def demo_exists(self, demo_name: str) -> bool:
        """Check if a demo executable exists."""
        return self.get_demo_path(demo_name).exists()

    def run_demo(
        self,
        demo_name: str,
        timeout: float = 30.0,
        capture_output: bool = True,
        env: Optional[dict] = None
    ) -> subprocess.CompletedProcess:
        """
        Run a demo and wait for completion.

        Args:
            demo_name: Name of the demo executable
            timeout: Maximum time to wait (seconds)
            capture_output: Whether to capture stdout/stderr
            env: Additional environment variables

        Returns:
            CompletedProcess with return code and output
        """
        demo_path = self.get_demo_path(demo_name)
        if not demo_path.exists():
            raise FileNotFoundError(f"Demo not found: {demo_path}")

        run_env = os.environ.copy()
        if env:
            run_env.update(env)

        return subprocess.run(
            [str(demo_path)],
            capture_output=capture_output,
            timeout=timeout,
            cwd=self.bin_dir,
            env=run_env,
            text=True
        )

    def start_demo(
        self,
        demo_name: str,
        env: Optional[dict] = None
    ) -> subprocess.Popen:
        """
        Start a demo in the background (for interactive demos).

        Args:
            demo_name: Name of the demo executable
            env: Additional environment variables

        Returns:
            Popen process handle
        """
        demo_path = self.get_demo_path(demo_name)
        if not demo_path.exists():
            raise FileNotFoundError(f"Demo not found: {demo_path}")

        run_env = os.environ.copy()
        if env:
            run_env.update(env)

        self.process = subprocess.Popen(
            [str(demo_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=self.bin_dir,
            env=run_env,
            text=True
        )
        return self.process

    def stop_demo(self, timeout: float = 5.0) -> None:
        """Stop a running demo process."""
        if self.process:
            self.process.terminate()
            try:
                self.process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self.process.kill()
            self.process = None


@pytest.fixture
def demo_runner(demos_bin_dir: Path) -> Generator[DemoRunner, None, None]:
    """Fixture providing a DemoRunner instance."""
    runner = DemoRunner(demos_bin_dir)
    yield runner
    # Cleanup any running processes
    runner.stop_demo()


class ScreenshotCapture:
    """Helper class for capturing and comparing screenshots."""

    def __init__(self, reference_dir: Path, output_dir: Path):
        self.reference_dir = reference_dir
        self.output_dir = output_dir

    def capture_window(self, window_title: str, output_name: str) -> Optional[Path]:
        """
        Capture a screenshot of a specific window.

        Args:
            window_title: Title of the window to capture
            output_name: Name for the output file (without extension)

        Returns:
            Path to the captured screenshot, or None if capture failed
        """
        try:
            import pyautogui

            # Give the window time to render
            time.sleep(0.5)

            # Capture the entire screen (window-specific capture is platform-dependent)
            screenshot = pyautogui.screenshot()

            output_path = self.output_dir / f"{output_name}.png"
            screenshot.save(output_path)
            return output_path

        except Exception as e:
            print(f"Screenshot capture failed: {e}")
            return None

    def compare_images(
        self,
        actual_path: Path,
        reference_name: str,
        threshold: float = 0.05
    ) -> tuple[bool, float]:
        """
        Compare an actual screenshot against a reference image.

        Args:
            actual_path: Path to the actual screenshot
            reference_name: Name of the reference image (without extension)
            threshold: Maximum allowed difference (0.0-1.0)

        Returns:
            Tuple of (passed, difference_ratio)
        """
        reference_path = self.reference_dir / f"{reference_name}.png"

        if not reference_path.exists():
            # No reference yet - save actual as new reference
            actual = Image.open(actual_path)
            actual.save(reference_path)
            return True, 0.0

        try:
            from pixelmatch import pixelmatch

            actual = Image.open(actual_path).convert("RGBA")
            reference = Image.open(reference_path).convert("RGBA")

            # Resize if dimensions don't match
            if actual.size != reference.size:
                actual = actual.resize(reference.size, Image.Resampling.LANCZOS)

            # Compare pixels
            width, height = reference.size
            diff_image = Image.new("RGBA", (width, height))

            num_diff_pixels = pixelmatch(
                list(actual.getdata()),
                list(reference.getdata()),
                width,
                height,
                list(diff_image.getdata()),
                threshold=0.1
            )

            total_pixels = width * height
            diff_ratio = num_diff_pixels / total_pixels

            # Save diff image for debugging
            diff_path = self.output_dir / f"{reference_name}_diff.png"
            diff_image.save(diff_path)

            return diff_ratio <= threshold, diff_ratio

        except ImportError:
            # Fallback to simpler comparison using imagehash
            import imagehash

            actual = Image.open(actual_path)
            reference = Image.open(reference_path)

            hash_actual = imagehash.average_hash(actual)
            hash_reference = imagehash.average_hash(reference)

            diff = hash_actual - hash_reference
            max_diff = 64  # Maximum possible hash difference
            diff_ratio = diff / max_diff

            return diff_ratio <= threshold, diff_ratio


@pytest.fixture
def screenshot_capture(reference_dir: Path, output_dir: Path) -> ScreenshotCapture:
    """Fixture providing a ScreenshotCapture instance."""
    return ScreenshotCapture(reference_dir, output_dir)


class InputSimulator:
    """Helper class for simulating keyboard and mouse input."""

    def __init__(self):
        self._pyautogui = None

    @property
    def gui(self):
        """Lazy load pyautogui."""
        if self._pyautogui is None:
            import pyautogui
            pyautogui.PAUSE = 0.1  # Small delay between actions
            pyautogui.FAILSAFE = True  # Move mouse to corner to abort
            self._pyautogui = pyautogui
        return self._pyautogui

    def press_key(self, key: str, hold_time: float = 0.0) -> None:
        """
        Press and release a key.

        Args:
            key: Key name (e.g., 'space', 'a', 'enter')
            hold_time: How long to hold the key (seconds)
        """
        if hold_time > 0:
            self.gui.keyDown(key)
            time.sleep(hold_time)
            self.gui.keyUp(key)
        else:
            self.gui.press(key)

    def type_text(self, text: str, interval: float = 0.05) -> None:
        """
        Type a string of text.

        Args:
            text: Text to type
            interval: Delay between characters (seconds)
        """
        self.gui.typewrite(text, interval=interval)

    def move_mouse(self, x: int, y: int, duration: float = 0.0) -> None:
        """
        Move the mouse to a position.

        Args:
            x: Target X coordinate
            y: Target Y coordinate
            duration: Movement duration (seconds)
        """
        self.gui.moveTo(x, y, duration=duration)

    def click(self, x: Optional[int] = None, y: Optional[int] = None, button: str = 'left') -> None:
        """
        Click the mouse.

        Args:
            x: X coordinate (None for current position)
            y: Y coordinate (None for current position)
            button: 'left', 'right', or 'middle'
        """
        if x is not None and y is not None:
            self.gui.click(x, y, button=button)
        else:
            self.gui.click(button=button)

    def drag(self, start_x: int, start_y: int, end_x: int, end_y: int, duration: float = 0.5) -> None:
        """
        Drag the mouse from one position to another.

        Args:
            start_x: Starting X coordinate
            start_y: Starting Y coordinate
            end_x: Ending X coordinate
            end_y: Ending Y coordinate
            duration: Drag duration (seconds)
        """
        self.gui.moveTo(start_x, start_y)
        self.gui.drag(end_x - start_x, end_y - start_y, duration=duration)

    def hotkey(self, *keys: str) -> None:
        """
        Press a combination of keys.

        Args:
            keys: Keys to press together (e.g., 'ctrl', 'c')
        """
        self.gui.hotkey(*keys)


@pytest.fixture
def input_simulator() -> InputSimulator:
    """Fixture providing an InputSimulator instance."""
    return InputSimulator()


# Markers for different test types
def pytest_configure(config):
    """Register custom markers."""
    config.addinivalue_line("markers", "visual: marks tests that verify visual output")
    config.addinivalue_line("markers", "interactive: marks tests that require user interaction")
    config.addinivalue_line("markers", "slow: marks tests that take a long time to run")
    config.addinivalue_line("markers", "requires_display: marks tests that require a display")
