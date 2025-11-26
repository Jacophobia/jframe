-- Test Level
-- Simple platformer level layout

return {
  name = "Test Level 1",
  width = 1920,
  height = 1080,

  entities = {
    {
      type = "player",
      x = 100,
      y = 500
    },
    {
      type = "enemy",
      x = 800,
      y = 500
    },
    {
      type = "platform",
      x = 0,
      y = 900,
      width = 1920,
      height = 180
    }
  }
}
