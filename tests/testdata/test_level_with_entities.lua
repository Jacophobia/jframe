-- Test Level with Entity Definitions
-- Level designed to test entity parsing functionality

return {
  name = "Test Level With Entities",
  width = 1920,
  height = 1080,

  spawnPoints = {
    default = { x = 100, y = 500, rotation = 0 }
  },

  entities = {
    -- Platform entity
    {
      type = "platform",
      x = 0,
      y = 550,
      width = 800,
      height = 50
    },

    -- Enemy entity with properties
    {
      type = "enemy",
      x = 400,
      y = 500,
      patrolRange = 100,
      speed = 50,
      hostile = true
    },

    -- Collectible entity
    {
      type = "collectible",
      x = 200,
      y = 450,
      value = 10,
      collectType = "coin"
    },

    -- Entity with transform properties
    {
      type = "rotating_platform",
      x = 600,
      y = 300,
      rotation = 45,
      scaleX = 2.0,
      scaleY = 1.5,
      width = 100,
      height = 20
    },

    -- Entity with various property types
    {
      type = "trigger",
      x = 800,
      y = 400,
      radius = 50.5,
      active = true,
      message = "You found a secret!",
      triggerCount = 1
    }
  }
}
