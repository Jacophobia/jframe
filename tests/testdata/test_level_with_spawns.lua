-- Test Level with Spawn Points
-- Level designed to test spawn point functionality

return {
  name = "Test Level With Spawns",
  width = 2000,
  height = 1200,

  spawnPoints = {
    default = { x = 100, y = 500, rotation = 0 },
    checkpoint1 = { x = 500, y = 400, rotation = 0 },
    checkpoint2 = { x = 1000, y = 300, rotation = 0 },
    boss_room = { x = 1800, y = 600, rotation = 180 },
    secret_area = { x = 200, y = 100, rotation = 90 }
  },

  entities = {
    {
      type = "platform",
      x = 0,
      y = 1000,
      width = 2000,
      height = 200
    }
  }
}
