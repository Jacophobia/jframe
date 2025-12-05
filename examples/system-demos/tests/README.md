# Bestow System Demos Tests

Automated testing framework for Bestow system demos using pytest.

## Overview

This test suite verifies that all system demos:
1. **Exist** - Executables were built successfully
2. **Run** - Execute without crashes or errors
3. **Demonstrate APIs** - Output shows API methods being exercised
4. **Render correctly** - Visual output matches expected (visual tests)
5. **Handle input** - Respond correctly to keyboard/mouse (interactive tests)

## Test Categories

### Non-Interactive Tests
Tests that run demos to completion and verify output:
- Entity, Events, Config, Assets, Save demos
- Audio, Physics, Level, AI, Camera, GAS, Blueprints demos

### Interactive Tests
Tests requiring display and user input simulation:
- Input demo (keyboard, mouse, controller)
- Graphics demo (visual rendering verification)

### Visual Tests
Tests that capture screenshots and compare against references:
- Graphics demo rendering
- Any demo with visual output

## Setup

### Install Dependencies

```bash
cd examples/system-demos/tests
pip install -r requirements.txt
```

### Build Demos First

```bash
# From project root
cmake --preset macos-debug
cmake --build --preset macos-debug
```

## Running Tests

### Quick Start

```bash
# Run all non-interactive tests
./run_tests.sh

# Run all tests (requires display)
./run_tests.sh --all

# Run only visual tests
./run_tests.sh --visual

# Quick existence check
./run_tests.sh --quick

# Run in parallel
./run_tests.sh --parallel
```

### Direct pytest Commands

```bash
# Run all tests
pytest -v

# Run specific demo tests
pytest -v test_entity_demo.py
pytest -v test_graphics_demo.py

# Run only non-interactive tests
pytest -v -m "not interactive and not visual"

# Run with HTML report
pytest -v --html=report.html

# Run in parallel (4 workers)
pytest -v -n 4

# Run specific test
pytest -v -k "test_demo_exists"
```

## Test Structure

```
tests/
├── conftest.py           # Fixtures and utilities
├── requirements.txt      # Python dependencies
├── run_tests.sh          # Test runner script
├── README.md             # This file
├── test_all_demos.py     # Comprehensive tests for all demos
├── test_entity_demo.py   # Entity-specific tests
├── test_events_demo.py   # Events-specific tests
├── test_graphics_demo.py # Graphics + visual tests
├── test_input_demo.py    # Input + interactive tests
├── reference_images/     # Expected visual output (auto-generated)
└── test_output/          # Test artifacts and screenshots
```

## Fixtures

### `demo_runner`
Runs demo executables and captures output.

```python
def test_demo(demo_runner):
    result = demo_runner.run_demo("entity-demo", timeout=30)
    assert result.returncode == 0
```

### `screenshot_capture`
Captures and compares screenshots.

```python
@pytest.mark.visual
def test_visual(demo_runner, screenshot_capture):
    process = demo_runner.start_demo("graphics-demo")
    time.sleep(1.0)
    path = screenshot_capture.capture_window("graphics-demo", "output")
    passed, diff = screenshot_capture.compare_images(path, "reference")
    assert passed
```

### `input_simulator`
Simulates keyboard and mouse input.

```python
@pytest.mark.interactive
def test_input(demo_runner, input_simulator):
    demo_runner.start_demo("input-demo")
    input_simulator.press_key('space')
    input_simulator.move_mouse(400, 300)
    input_simulator.click()
```

### `texture_finder`
Find specific textures/images within screenshots using OpenCV.

```python
@pytest.mark.visual
def test_player_visible(demo_runner, screenshot_capture, texture_finder):
    demo_runner.start_demo("graphics-demo")
    time.sleep(1.0)
    screenshot = screenshot_capture.capture_window("graphics-demo", "test")

    # Find a single texture
    match = texture_finder.find_texture(screenshot, "assets/player.png", threshold=0.8)
    assert match.found
    print(f"Player at {match.center} with {match.confidence:.0%} confidence")

    # Find multiple instances (e.g., coins)
    matches = texture_finder.find_texture_multiple(screenshot, "assets/coin.png")
    assert matches.count >= 5, f"Expected 5 coins, found {matches.count}"

    # Find scaled texture (0.5x to 2x size)
    match = texture_finder.find_texture_scaled(screenshot, "assets/enemy.png",
                                                min_scale=0.5, max_scale=2.0)

    # Find rotated/transformed texture using SIFT features
    match = texture_finder.find_texture_rotated(screenshot, "assets/boss.png")

    # Find colored regions (e.g., red health bar)
    red_regions = texture_finder.find_color_region(screenshot,
                                                    color_bgr=(0, 0, 255),  # BGR format
                                                    tolerance=30)
```

**TextureFinder Methods:**

| Method | Use Case |
|--------|----------|
| `find_texture()` | Exact match - texture appears at same size |
| `find_texture_multiple()` | Find all instances of a texture |
| `find_texture_scaled()` | Texture may be rendered at different scale |
| `find_texture_rotated()` | Texture may be rotated (uses SIFT features) |
| `find_color_region()` | Find UI elements by color |

All methods save debug images showing matches to `test_output/`.

## Markers

- `@pytest.mark.visual` - Tests that verify visual output
- `@pytest.mark.interactive` - Tests requiring user input simulation
- `@pytest.mark.slow` - Tests that take a long time
- `@pytest.mark.requires_display` - Tests needing a display

## Reference Images

Visual tests compare against reference images in `reference_images/`.

On first run, if no reference exists, the actual screenshot is saved as the new reference. On subsequent runs, screenshots are compared against references.

To update references, delete the existing reference image and run the test again.

## CI/CD Integration

For headless CI environments:

```bash
# Skip interactive/visual tests
pytest -v -m "not interactive and not visual"

# Or use xvfb for virtual display (Linux)
xvfb-run pytest -v
```

## Troubleshooting

### "Demo not found"
Build the demos first:
```bash
cmake --build --preset macos-debug
```

### "Display required"
Interactive tests need a display. Skip them in headless environments:
```bash
pytest -v -m "not interactive"
```

### "pyautogui error"
macOS requires accessibility permissions for input simulation.
Grant Terminal/IDE access in System Preferences > Security & Privacy > Accessibility.

### "Screenshot comparison failed"
- Check `test_output/` for actual vs reference images
- Review `*_diff.png` files to see pixel differences
- Update reference if the change is intentional

## Adding New Tests

1. Create `test_<demo>_demo.py` file
2. Use `demo_runner` fixture to run demos
3. Check output for expected API demonstrations
4. Add visual/interactive tests as needed

Example:

```python
class TestNewDemo:
    def test_demo_exists(self, demo_runner):
        assert demo_runner.demo_exists("new-demo")

    def test_demo_runs(self, demo_runner):
        result = demo_runner.run_demo("new-demo", timeout=30)
        assert result.returncode == 0
        assert "expected output" in result.stdout
```
