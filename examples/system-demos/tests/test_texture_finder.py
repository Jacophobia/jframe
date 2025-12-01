"""
Texture Finder Unit Tests

Tests that verify the TextureFinder works correctly using generated test images.
These tests don't require running any demos - they test the OpenCV-based
detection logic in isolation.
"""

import pytest
from pathlib import Path
from PIL import Image, ImageDraw


class TestTextureFinder:
    """Unit tests for the TextureFinder class."""

    @pytest.fixture
    def test_images_dir(self, output_dir):
        """Create a directory for test images."""
        test_dir = output_dir / "texture_finder_tests"
        test_dir.mkdir(exist_ok=True)
        return test_dir

    @pytest.fixture
    def create_test_texture(self, test_images_dir):
        """Factory to create test textures with distinctive patterns."""
        def _create(name: str, size: tuple, color: tuple, with_border: bool = True) -> Path:
            img = Image.new("RGB", size, color)
            if with_border:
                # Add a distinctive border to make the texture uniquely identifiable
                draw = ImageDraw.Draw(img)
                # White border
                draw.rectangle([0, 0, size[0]-1, size[1]-1], outline=(255, 255, 255), width=2)
                # Inner black border
                draw.rectangle([3, 3, size[0]-4, size[1]-4], outline=(0, 0, 0), width=1)
            path = test_images_dir / f"{name}.png"
            img.save(path)
            return path
        return _create

    @pytest.fixture
    def create_screenshot_with_texture(self, test_images_dir):
        """Factory to create screenshots containing textures at known positions."""
        def _create(
            name: str,
            screen_size: tuple,
            background_color: tuple,
            texture_color: tuple,
            texture_size: tuple,
            texture_positions: list
        ) -> tuple[Path, Path]:
            # Create the texture with distinctive borders
            texture = Image.new("RGB", texture_size, texture_color)
            draw = ImageDraw.Draw(texture)
            # Add borders to make texture uniquely identifiable
            draw.rectangle([0, 0, texture_size[0]-1, texture_size[1]-1], outline=(255, 255, 255), width=2)
            draw.rectangle([3, 3, texture_size[0]-4, texture_size[1]-4], outline=(0, 0, 0), width=1)
            texture_path = test_images_dir / f"{name}_texture.png"
            texture.save(texture_path)

            # Create the screenshot with texture placed at positions
            screenshot = Image.new("RGB", screen_size, background_color)
            for pos in texture_positions:
                screenshot.paste(texture, pos)

            screenshot_path = test_images_dir / f"{name}_screenshot.png"
            screenshot.save(screenshot_path)

            return screenshot_path, texture_path
        return _create

    def test_find_single_texture(self, texture_finder, create_screenshot_with_texture):
        """Test finding a single texture in a screenshot."""
        screenshot_path, texture_path = create_screenshot_with_texture(
            name="single",
            screen_size=(800, 600),
            background_color=(50, 50, 50),
            texture_color=(255, 0, 0),  # Red square
            texture_size=(64, 64),
            texture_positions=[(200, 150)]
        )

        match = texture_finder.find_texture(screenshot_path, texture_path, threshold=0.9)

        assert match.found, f"Texture not found (confidence: {match.confidence:.2%})"
        assert match.confidence >= 0.95, f"Low confidence: {match.confidence:.2%}"
        assert match.location is not None
        assert abs(match.location[0] - 200) <= 2, f"X position off: {match.location[0]}"
        assert abs(match.location[1] - 150) <= 2, f"Y position off: {match.location[1]}"
        assert match.size == (64, 64)

    def test_find_texture_not_present(self, texture_finder, test_images_dir, create_test_texture):
        """Test that we correctly report texture not found."""
        # Create a screenshot without the texture
        screenshot = Image.new("RGB", (800, 600), (50, 50, 50))
        screenshot_path = test_images_dir / "empty_screenshot.png"
        screenshot.save(screenshot_path)

        # Create a texture that's not in the screenshot
        texture_path = create_test_texture("missing", (64, 64), (255, 0, 0))

        match = texture_finder.find_texture(screenshot_path, texture_path, threshold=0.9)

        assert not match.found, "Should not find texture that's not present"
        assert match.confidence < 0.9, f"Confidence too high for missing texture: {match.confidence:.2%}"

    def test_find_multiple_textures(self, texture_finder, create_screenshot_with_texture):
        """Test finding multiple instances of a texture."""
        positions = [(100, 100), (300, 100), (500, 100), (200, 300), (400, 300)]
        screenshot_path, texture_path = create_screenshot_with_texture(
            name="multiple",
            screen_size=(800, 600),
            background_color=(30, 30, 30),
            texture_color=(0, 255, 0),  # Green squares
            texture_size=(50, 50),
            texture_positions=positions
        )

        matches = texture_finder.find_texture_multiple(
            screenshot_path, texture_path,
            threshold=0.9,
            min_distance=40
        )

        assert matches.count == 5, f"Expected 5 textures, found {matches.count}"
        assert len(matches.locations) == 5
        assert len(matches.centers) == 5

    def test_find_texture_scaled(self, texture_finder, test_images_dir):
        """Test finding a texture rendered at a different scale."""
        # Create a small texture with distinctive pattern
        texture = Image.new("RGB", (32, 32), (0, 0, 255))  # Blue
        draw = ImageDraw.Draw(texture)
        # Add borders and cross pattern
        draw.rectangle([0, 0, 31, 31], outline=(255, 255, 0), width=2)
        draw.line([5, 16, 27, 16], fill=(255, 255, 255), width=2)
        draw.line([16, 5, 16, 27], fill=(255, 255, 255), width=2)
        texture_path = test_images_dir / "scaled_texture.png"
        texture.save(texture_path)

        # Create screenshot with texture at 2x scale (64x64)
        screenshot = Image.new("RGB", (800, 600), (100, 100, 100))
        scaled_texture = texture.resize((64, 64), Image.Resampling.NEAREST)
        screenshot.paste(scaled_texture, (350, 250))
        screenshot_path = test_images_dir / "scaled_screenshot.png"
        screenshot.save(screenshot_path)

        match = texture_finder.find_texture_scaled(
            screenshot_path, texture_path,
            min_scale=1.5,
            max_scale=2.5,
            scale_steps=5,
            threshold=0.8
        )

        assert match.found, f"Scaled texture not found (confidence: {match.confidence:.2%})"
        assert match.size is not None
        # Should find the 2x scaled version
        assert abs(match.size[0] - 64) <= 10, f"Width off: {match.size[0]}"
        assert abs(match.size[1] - 64) <= 10, f"Height off: {match.size[1]}"

    def test_find_color_region(self, texture_finder, test_images_dir):
        """Test finding regions of a specific color."""
        # Create screenshot with colored regions
        screenshot = Image.new("RGB", (800, 600), (50, 50, 50))
        draw = ImageDraw.Draw(screenshot)

        # Draw some red rectangles
        draw.rectangle([100, 100, 200, 150], fill=(255, 0, 0))
        draw.rectangle([400, 200, 500, 280], fill=(255, 0, 0))
        draw.rectangle([600, 400, 700, 500], fill=(255, 0, 0))

        # Draw a green rectangle (should not be found)
        draw.rectangle([300, 300, 380, 380], fill=(0, 255, 0))

        screenshot_path = test_images_dir / "color_regions.png"
        screenshot.save(screenshot_path)

        # Find red regions (BGR format: red = (0, 0, 255))
        red_regions = texture_finder.find_color_region(
            screenshot_path,
            color_bgr=(0, 0, 255),
            tolerance=20,
            min_area=100
        )

        assert len(red_regions) == 3, f"Expected 3 red regions, found {len(red_regions)}"

    def test_confidence_varies_with_match_quality(self, texture_finder, test_images_dir):
        """Test that confidence is higher for better matches."""
        # Create a texture with distinctive pattern
        texture = Image.new("RGB", (64, 64), (255, 128, 0))  # Orange
        draw = ImageDraw.Draw(texture)
        draw.rectangle([0, 0, 63, 63], outline=(255, 255, 255), width=2)
        draw.rectangle([3, 3, 60, 60], outline=(0, 0, 0), width=1)
        # Add a cross pattern
        draw.line([10, 32, 54, 32], fill=(0, 0, 0), width=2)
        draw.line([32, 10, 32, 54], fill=(0, 0, 0), width=2)
        texture_path = test_images_dir / "orange_texture.png"
        texture.save(texture_path)

        # Create screenshot with exact match
        exact_screenshot = Image.new("RGB", (800, 600), (50, 50, 50))
        exact_screenshot.paste(texture, (300, 200))
        exact_path = test_images_dir / "exact_match.png"
        exact_screenshot.save(exact_path)

        # Create screenshot with similar but not exact color (different fill but same pattern)
        similar_texture = Image.new("RGB", (64, 64), (240, 120, 10))  # Slightly different
        draw_similar = ImageDraw.Draw(similar_texture)
        draw_similar.rectangle([0, 0, 63, 63], outline=(255, 255, 255), width=2)
        draw_similar.rectangle([3, 3, 60, 60], outline=(0, 0, 0), width=1)
        # Cross pattern with slightly different lines
        draw_similar.line([10, 32, 54, 32], fill=(50, 50, 50), width=2)  # Different color
        draw_similar.line([32, 10, 32, 54], fill=(50, 50, 50), width=2)
        similar_screenshot = Image.new("RGB", (800, 600), (50, 50, 50))
        similar_screenshot.paste(similar_texture, (300, 200))
        similar_path = test_images_dir / "similar_match.png"
        similar_screenshot.save(similar_path)

        exact_match = texture_finder.find_texture(exact_path, texture_path, threshold=0.5)
        similar_match = texture_finder.find_texture(similar_path, texture_path, threshold=0.5)

        assert exact_match.confidence > similar_match.confidence, \
            f"Exact match ({exact_match.confidence:.2%}) should have higher confidence than similar ({similar_match.confidence:.2%})"

    def test_center_calculation(self, texture_finder, create_screenshot_with_texture):
        """Test that center point is calculated correctly."""
        screenshot_path, texture_path = create_screenshot_with_texture(
            name="center_test",
            screen_size=(800, 600),
            background_color=(0, 0, 0),
            texture_color=(255, 255, 255),
            texture_size=(100, 60),
            texture_positions=[(200, 150)]
        )

        match = texture_finder.find_texture(screenshot_path, texture_path, threshold=0.9)

        assert match.found
        # Center should be at (200 + 50, 150 + 30) = (250, 180)
        expected_center = (250, 180)
        assert abs(match.center[0] - expected_center[0]) <= 2
        assert abs(match.center[1] - expected_center[1]) <= 2


class TestTextureFinderwithRealImages:
    """Tests using more complex/realistic image scenarios."""

    @pytest.fixture
    def complex_screenshot(self, output_dir):
        """Create a more complex screenshot with multiple elements."""
        test_dir = output_dir / "complex_tests"
        test_dir.mkdir(exist_ok=True)

        # Create player texture with distinctive pattern FIRST
        player = Image.new("RGB", (40, 40), (255, 0, 0))
        draw_player = ImageDraw.Draw(player)
        # Add face/features to make it distinctive
        draw_player.rectangle([0, 0, 39, 39], outline=(255, 255, 255), width=2)
        draw_player.rectangle([10, 10, 20, 15], fill=(255, 255, 255))  # Left eye
        draw_player.rectangle([22, 10, 32, 15], fill=(255, 255, 255))  # Right eye
        draw_player.rectangle([15, 25, 25, 30], fill=(255, 255, 255))  # Mouth
        player_path = test_dir / "player.png"
        player.save(player_path)

        # Create coin texture
        coin = Image.new("RGB", (20, 20), (255, 215, 0))
        draw_coin = ImageDraw.Draw(coin)
        draw_coin.ellipse([0, 0, 19, 19], fill=(255, 215, 0))
        draw_coin.ellipse([5, 5, 14, 14], fill=(255, 180, 0))  # Inner circle
        coin_path = test_dir / "coin.png"
        coin.save(coin_path)

        # Create enemy texture with distinctive pattern
        enemy = Image.new("RGB", (50, 40), (128, 0, 128))
        draw_enemy = ImageDraw.Draw(enemy)
        draw_enemy.rectangle([0, 0, 49, 39], outline=(255, 255, 0), width=2)
        draw_enemy.rectangle([10, 8, 18, 16], fill=(255, 0, 0))  # Left eye
        draw_enemy.rectangle([32, 8, 40, 16], fill=(255, 0, 0))  # Right eye
        draw_enemy.polygon([(25, 25), (15, 35), (35, 35)], fill=(255, 0, 0))  # Angry mouth
        enemy_path = test_dir / "enemy.png"
        enemy.save(enemy_path)

        # Create a game-like screenshot
        screenshot = Image.new("RGB", (1280, 720), (135, 206, 235))  # Sky blue
        draw = ImageDraw.Draw(screenshot)

        # Ground
        draw.rectangle([0, 600, 1280, 720], fill=(34, 139, 34))  # Green

        # Platforms
        draw.rectangle([100, 500, 300, 530], fill=(139, 69, 19))  # Brown
        draw.rectangle([400, 400, 600, 430], fill=(139, 69, 19))
        draw.rectangle([700, 300, 900, 330], fill=(139, 69, 19))

        # Paste player at position
        screenshot.paste(player, (150, 450))

        # Paste coins at positions
        coin_positions = [(250, 460), (450, 360), (550, 360), (750, 260), (850, 260)]
        for x, y in coin_positions:
            screenshot.paste(coin, (x, y))

        # Paste enemy at position
        screenshot.paste(enemy, (500, 550))

        screenshot_path = test_dir / "game_screenshot.png"
        screenshot.save(screenshot_path)

        return {
            "screenshot": screenshot_path,
            "player": player_path,
            "coin": coin_path,
            "enemy": enemy_path,
        }

    def test_find_player_in_complex_scene(self, texture_finder, complex_screenshot):
        """Test finding player texture in a complex game scene."""
        match = texture_finder.find_texture(
            complex_screenshot["screenshot"],
            complex_screenshot["player"],
            threshold=0.8
        )

        assert match.found, f"Player not found (confidence: {match.confidence:.2%})"

    def test_find_coins_in_complex_scene(self, texture_finder, complex_screenshot):
        """Test finding multiple coins in a complex game scene."""
        matches = texture_finder.find_texture_multiple(
            complex_screenshot["screenshot"],
            complex_screenshot["coin"],
            threshold=0.7,
            min_distance=15
        )

        # We placed 5 coins
        assert matches.count >= 3, f"Expected at least 3 coins, found {matches.count}"

    def test_find_enemy_in_complex_scene(self, texture_finder, complex_screenshot):
        """Test finding enemy texture in a complex game scene."""
        match = texture_finder.find_texture(
            complex_screenshot["screenshot"],
            complex_screenshot["enemy"],
            threshold=0.8
        )

        assert match.found, f"Enemy not found (confidence: {match.confidence:.2%})"

    def test_find_ground_by_color(self, texture_finder, complex_screenshot):
        """Test finding the ground region by color."""
        # Green ground in BGR is (34, 139, 34)
        green_regions = texture_finder.find_color_region(
            complex_screenshot["screenshot"],
            color_bgr=(34, 139, 34),
            tolerance=10,
            min_area=1000
        )

        assert len(green_regions) >= 1, "Ground not found by color"
        # Ground should be a large region
        largest = max(green_regions, key=lambda r: r[2] * r[3])
        assert largest[2] > 500, "Ground region too small"
