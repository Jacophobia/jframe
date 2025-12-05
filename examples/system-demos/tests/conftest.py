"""
Bestow System Demos Test Configuration

Pytest configuration and fixtures for testing Bestow system demos.
Provides utilities for:
- Running demo executables
- Capturing screenshots
- Comparing visual output
- Simulating input
- Texture/image detection on screen
"""

import os
import subprocess
import time
from pathlib import Path
from typing import Optional, Generator, NamedTuple

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


class TextureMatch(NamedTuple):
    """Result of a texture search operation."""
    found: bool
    confidence: float  # 0.0 to 1.0
    location: Optional[tuple[int, int]]  # (x, y) of top-left corner if found
    center: Optional[tuple[int, int]]  # (x, y) of center if found
    size: Optional[tuple[int, int]]  # (width, height) of matched region


class TextureMatchMultiple(NamedTuple):
    """Result of finding multiple texture instances."""
    count: int
    locations: list[tuple[int, int]]  # List of (x, y) top-left corners
    centers: list[tuple[int, int]]  # List of (x, y) centers
    confidences: list[float]  # Confidence for each match


class TextureFinder:
    """
    Helper class for finding textures/images within screenshots.

    Uses OpenCV for template matching and feature detection.
    """

    def __init__(self, output_dir: Path):
        self.output_dir = output_dir
        self._cv2 = None
        self._np = None

    @property
    def cv2(self):
        """Lazy load OpenCV."""
        if self._cv2 is None:
            import cv2
            self._cv2 = cv2
        return self._cv2

    @property
    def np(self):
        """Lazy load numpy."""
        if self._np is None:
            import numpy as np
            self._np = np
        return self._np

    def find_texture(
        self,
        screenshot_path: Path,
        texture_path: Path,
        threshold: float = 0.8,
        save_debug: bool = True
    ) -> TextureMatch:
        """
        Find a texture within a screenshot using template matching.

        Args:
            screenshot_path: Path to the screenshot image
            texture_path: Path to the texture to find
            threshold: Minimum confidence (0.0-1.0) to consider a match
            save_debug: Whether to save a debug image showing the match

        Returns:
            TextureMatch with found status, confidence, and location
        """
        # Load images
        screen = self.cv2.imread(str(screenshot_path))
        template = self.cv2.imread(str(texture_path))

        if screen is None:
            raise FileNotFoundError(f"Could not load screenshot: {screenshot_path}")
        if template is None:
            raise FileNotFoundError(f"Could not load texture: {texture_path}")

        # Get template dimensions
        h, w = template.shape[:2]

        # Perform template matching
        result = self.cv2.matchTemplate(screen, template, self.cv2.TM_CCOEFF_NORMED)
        min_val, max_val, min_loc, max_loc = self.cv2.minMaxLoc(result)

        confidence = max_val
        found = confidence >= threshold

        if found:
            top_left = max_loc
            center = (top_left[0] + w // 2, top_left[1] + h // 2)

            if save_debug:
                # Draw rectangle on debug image
                debug_img = screen.copy()
                bottom_right = (top_left[0] + w, top_left[1] + h)
                self.cv2.rectangle(debug_img, top_left, bottom_right, (0, 255, 0), 2)
                self.cv2.putText(
                    debug_img, f"{confidence:.2f}",
                    (top_left[0], top_left[1] - 10),
                    self.cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2
                )
                debug_path = self.output_dir / f"texture_match_{texture_path.stem}.png"
                self.cv2.imwrite(str(debug_path), debug_img)

            return TextureMatch(
                found=True,
                confidence=confidence,
                location=top_left,
                center=center,
                size=(w, h)
            )
        else:
            return TextureMatch(
                found=False,
                confidence=confidence,
                location=None,
                center=None,
                size=None
            )

    def find_texture_multiple(
        self,
        screenshot_path: Path,
        texture_path: Path,
        threshold: float = 0.8,
        min_distance: int = 10,
        save_debug: bool = True
    ) -> TextureMatchMultiple:
        """
        Find all instances of a texture within a screenshot.

        Args:
            screenshot_path: Path to the screenshot image
            texture_path: Path to the texture to find
            threshold: Minimum confidence (0.0-1.0) to consider a match
            min_distance: Minimum pixel distance between matches (to avoid duplicates)
            save_debug: Whether to save a debug image showing all matches

        Returns:
            TextureMatchMultiple with count and all locations
        """
        screen = self.cv2.imread(str(screenshot_path))
        template = self.cv2.imread(str(texture_path))

        if screen is None:
            raise FileNotFoundError(f"Could not load screenshot: {screenshot_path}")
        if template is None:
            raise FileNotFoundError(f"Could not load texture: {texture_path}")

        h, w = template.shape[:2]

        # Perform template matching
        result = self.cv2.matchTemplate(screen, template, self.cv2.TM_CCOEFF_NORMED)

        # Find all locations above threshold
        locations = self.np.where(result >= threshold)
        points = list(zip(*locations[::-1]))  # Convert to (x, y) format

        # Filter out duplicates that are too close together
        filtered_points = []
        filtered_confidences = []
        for pt in points:
            too_close = False
            for existing in filtered_points:
                dist = ((pt[0] - existing[0]) ** 2 + (pt[1] - existing[1]) ** 2) ** 0.5
                if dist < min_distance:
                    too_close = True
                    break
            if not too_close:
                filtered_points.append(pt)
                filtered_confidences.append(float(result[pt[1], pt[0]]))

        # Calculate centers
        centers = [(pt[0] + w // 2, pt[1] + h // 2) for pt in filtered_points]

        if save_debug and filtered_points:
            debug_img = screen.copy()
            for i, pt in enumerate(filtered_points):
                bottom_right = (pt[0] + w, pt[1] + h)
                self.cv2.rectangle(debug_img, pt, bottom_right, (0, 255, 0), 2)
                self.cv2.putText(
                    debug_img, f"{filtered_confidences[i]:.2f}",
                    (pt[0], pt[1] - 10),
                    self.cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1
                )
            debug_path = self.output_dir / f"texture_matches_{texture_path.stem}.png"
            self.cv2.imwrite(str(debug_path), debug_img)

        return TextureMatchMultiple(
            count=len(filtered_points),
            locations=filtered_points,
            centers=centers,
            confidences=filtered_confidences
        )

    def find_texture_scaled(
        self,
        screenshot_path: Path,
        texture_path: Path,
        min_scale: float = 0.5,
        max_scale: float = 2.0,
        scale_steps: int = 10,
        threshold: float = 0.8,
        save_debug: bool = True
    ) -> TextureMatch:
        """
        Find a texture that may be scaled differently than the reference.

        Args:
            screenshot_path: Path to the screenshot image
            texture_path: Path to the texture to find
            min_scale: Minimum scale factor to try
            max_scale: Maximum scale factor to try
            scale_steps: Number of scale steps to try
            threshold: Minimum confidence to consider a match
            save_debug: Whether to save debug image

        Returns:
            TextureMatch with the best match found
        """
        screen = self.cv2.imread(str(screenshot_path))
        template = self.cv2.imread(str(texture_path))

        if screen is None or template is None:
            raise FileNotFoundError("Could not load images")

        best_match = TextureMatch(False, 0.0, None, None, None)
        best_scale = 1.0

        scales = self.np.linspace(min_scale, max_scale, scale_steps)

        for scale in scales:
            # Resize template
            new_w = int(template.shape[1] * scale)
            new_h = int(template.shape[0] * scale)
            if new_w < 1 or new_h < 1:
                continue
            if new_w > screen.shape[1] or new_h > screen.shape[0]:
                continue

            scaled_template = self.cv2.resize(template, (new_w, new_h))

            # Match
            result = self.cv2.matchTemplate(screen, scaled_template, self.cv2.TM_CCOEFF_NORMED)
            _, max_val, _, max_loc = self.cv2.minMaxLoc(result)

            if max_val > best_match.confidence:
                best_match = TextureMatch(
                    found=max_val >= threshold,
                    confidence=max_val,
                    location=max_loc,
                    center=(max_loc[0] + new_w // 2, max_loc[1] + new_h // 2),
                    size=(new_w, new_h)
                )
                best_scale = scale

        if save_debug and best_match.found:
            debug_img = screen.copy()
            top_left = best_match.location
            w, h = best_match.size
            bottom_right = (top_left[0] + w, top_left[1] + h)
            self.cv2.rectangle(debug_img, top_left, bottom_right, (0, 255, 0), 2)
            self.cv2.putText(
                debug_img, f"{best_match.confidence:.2f} @{best_scale:.2f}x",
                (top_left[0], top_left[1] - 10),
                self.cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2
            )
            debug_path = self.output_dir / f"texture_scaled_{texture_path.stem}.png"
            self.cv2.imwrite(str(debug_path), debug_img)

        return best_match

    def find_texture_rotated(
        self,
        screenshot_path: Path,
        texture_path: Path,
        threshold: float = 0.7,
        min_matches: int = 10,
        save_debug: bool = True
    ) -> TextureMatch:
        """
        Find a texture using feature matching (works with rotation/scale).

        Uses SIFT features for robust matching even when the texture
        is rotated or scaled.

        Args:
            screenshot_path: Path to the screenshot image
            texture_path: Path to the texture to find
            threshold: Lowe's ratio test threshold (lower = stricter)
            min_matches: Minimum good matches required
            save_debug: Whether to save debug image

        Returns:
            TextureMatch indicating if texture was found
        """
        screen = self.cv2.imread(str(screenshot_path), self.cv2.IMREAD_GRAYSCALE)
        template = self.cv2.imread(str(texture_path), self.cv2.IMREAD_GRAYSCALE)

        if screen is None or template is None:
            raise FileNotFoundError("Could not load images")

        # Create SIFT detector
        sift = self.cv2.SIFT_create()

        # Detect keypoints and compute descriptors
        kp1, desc1 = sift.detectAndCompute(template, None)
        kp2, desc2 = sift.detectAndCompute(screen, None)

        if desc1 is None or desc2 is None or len(desc1) < 2 or len(desc2) < 2:
            return TextureMatch(False, 0.0, None, None, None)

        # Match features
        bf = self.cv2.BFMatcher()
        matches = bf.knnMatch(desc1, desc2, k=2)

        # Apply Lowe's ratio test
        good_matches = []
        for m, n in matches:
            if m.distance < threshold * n.distance:
                good_matches.append(m)

        found = len(good_matches) >= min_matches
        confidence = len(good_matches) / max(len(kp1), 1)

        if found and save_debug:
            # Draw matches
            screen_color = self.cv2.imread(str(screenshot_path))
            template_color = self.cv2.imread(str(texture_path))
            debug_img = self.cv2.drawMatches(
                template_color, kp1, screen_color, kp2, good_matches, None,
                flags=self.cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS
            )
            debug_path = self.output_dir / f"texture_features_{texture_path.stem}.png"
            self.cv2.imwrite(str(debug_path), debug_img)

        # Estimate location from matched keypoints
        center = None
        if found:
            matched_pts = [kp2[m.trainIdx].pt for m in good_matches]
            avg_x = sum(p[0] for p in matched_pts) / len(matched_pts)
            avg_y = sum(p[1] for p in matched_pts) / len(matched_pts)
            center = (int(avg_x), int(avg_y))

        return TextureMatch(
            found=found,
            confidence=confidence,
            location=None,  # Not precise with feature matching
            center=center,
            size=None
        )

    def find_color_region(
        self,
        screenshot_path: Path,
        color_bgr: tuple[int, int, int],
        tolerance: int = 20,
        min_area: int = 100,
        save_debug: bool = True
    ) -> list[tuple[int, int, int, int]]:
        """
        Find regions of a specific color in the screenshot.

        Args:
            screenshot_path: Path to the screenshot image
            color_bgr: Color to find in BGR format (OpenCV default)
            tolerance: Color matching tolerance (0-255)
            min_area: Minimum contour area to consider
            save_debug: Whether to save debug image

        Returns:
            List of bounding boxes (x, y, w, h) for matching regions
        """
        screen = self.cv2.imread(str(screenshot_path))
        if screen is None:
            raise FileNotFoundError(f"Could not load screenshot: {screenshot_path}")

        # Create color range
        lower = self.np.array([max(0, c - tolerance) for c in color_bgr])
        upper = self.np.array([min(255, c + tolerance) for c in color_bgr])

        # Create mask
        mask = self.cv2.inRange(screen, lower, upper)

        # Find contours
        contours, _ = self.cv2.findContours(mask, self.cv2.RETR_EXTERNAL, self.cv2.CHAIN_APPROX_SIMPLE)

        # Filter by area and get bounding boxes
        boxes = []
        for contour in contours:
            area = self.cv2.contourArea(contour)
            if area >= min_area:
                x, y, w, h = self.cv2.boundingRect(contour)
                boxes.append((x, y, w, h))

        if save_debug:
            debug_img = screen.copy()
            for (x, y, w, h) in boxes:
                self.cv2.rectangle(debug_img, (x, y), (x + w, y + h), (0, 255, 0), 2)
            debug_path = self.output_dir / "color_regions.png"
            self.cv2.imwrite(str(debug_path), debug_img)

        return boxes


@pytest.fixture
def texture_finder(output_dir: Path) -> TextureFinder:
    """Fixture providing a TextureFinder instance."""
    return TextureFinder(output_dir)


# Markers for different test types
def pytest_configure(config):
    """Register custom markers."""
    config.addinivalue_line("markers", "visual: marks tests that verify visual output")
    config.addinivalue_line("markers", "interactive: marks tests that require user interaction")
    config.addinivalue_line("markers", "slow: marks tests that take a long time to run")
    config.addinivalue_line("markers", "requires_display: marks tests that require a display")
