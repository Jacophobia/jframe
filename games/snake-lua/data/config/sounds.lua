-- Sound Configuration
-- Defines all sound effect and music paths for the snake game
-- Use :library:/ prefix for engine assets, :assets:/ for game-local assets

return {
    -- Sound Effects
    sfx = {
        eat = ":library:/sounds/25_item.wav",
        death = ":library:/sounds/63_lose1.wav",
        level_complete = ":library:/sounds/24_levelclear.wav",
        menu_select = ":library:/sounds/59_confirm.wav",
        menu_move = ":library:/sounds/05_cursor1.wav",
        enemy_hit = ":library:/sounds/15_hit.wav",
        chain_break = ":library:/sounds/69_explode.wav",
        pause = ":library:/sounds/07_pause1.wav",
        game_over = ":library:/sounds/64_lose2.wav",
    },

    -- Music
    music = {
        game = ":library:/music/game.ogg",
        menu = ":library:/music/menu.ogg",
    },

    -- Volume Settings (0.0 to 1.0)
    volumes = {
        master = 1.0,
        sfx = 0.8,
        music = 0.6,
    }
}
