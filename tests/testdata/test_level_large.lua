-- Large Test Level
-- Level with many entities for testing

return {
  name = "Large Test Level",
  width = 3840,
  height = 2160,

  spawnPoints = {
    default = { x = 100, y = 2000, rotation = 0 }
  },

  entities = {
    -- Ground
    { type = "platform", x = 0, y = 2100, width = 3840, height = 60 },

    -- 10 enemies
    { type = "enemy", x = 400, y = 2000 },
    { type = "enemy", x = 800, y = 2000 },
    { type = "enemy", x = 1200, y = 2000 },
    { type = "enemy", x = 1600, y = 2000 },
    { type = "enemy", x = 2000, y = 2000 },
    { type = "enemy", x = 2400, y = 2000 },
    { type = "enemy", x = 2800, y = 2000 },
    { type = "enemy", x = 3200, y = 2000 },
    { type = "enemy", x = 3600, y = 2000 },
    { type = "enemy", x = 500, y = 1500 },

    -- 5 platforms
    { type = "platform", x = 200, y = 1800, width = 300, height = 50 },
    { type = "platform", x = 700, y = 1600, width = 300, height = 50 },
    { type = "platform", x = 1200, y = 1400, width = 300, height = 50 },
    { type = "platform", x = 1700, y = 1200, width = 300, height = 50 },
    { type = "platform", x = 2200, y = 1000, width = 300, height = 50 }
  }
}
