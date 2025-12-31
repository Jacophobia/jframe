-- games/snake3d/data/config/sounds.lua
-- Audio configuration matching C++ snake game exactly
--
-- From games/game1/data/config/sounds.lua

return {
    -- Volume settings
    volumes = {
        master = 1.0,      -- 100% master volume
        sfx = 0.8,         -- 80% for sound effects
        music = 0.6,       -- 60% for music tracks
    },

    -- Sound effects
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

    -- Music tracks
    music = {
        game = ":library:/music/game.ogg",
        menu = ":library:/music/menu.ogg",
    },
}
