---
paths: "**/*.lua"
---

# Dvorak Keyboard Layout Support

The project owner uses **Dvorak keyboard layout**. All movement controls MUST support both Dvorak and QWERTY.

## Movement Key Mapping

| Action | Dvorak | QWERTY | Key Constants |
|--------|--------|--------|---------------|
| Forward | `,` (comma) | W | `Keys.Comma`, `Keys.W` |
| Back | `O` | S | `Keys.O`, `Keys.S` |
| Left | `A` | A | `Keys.A` (same) |
| Right | `E` | D | `Keys.E`, `Keys.D` |

## Required Pattern

Always check BOTH layouts:

```lua
-- Movement input (supports both layouts)
local moveX = 0
local moveZ = 0

-- Left (same key on both layouts)
if bestow.input.isKeyDown(Keys.A) then moveX = moveX - 1 end

-- Right (Dvorak: E, QWERTY: D)
if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
    moveX = moveX + 1
end

-- Forward (Dvorak: comma, QWERTY: W)
if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
    moveZ = moveZ - 1
end

-- Back (Dvorak: O, QWERTY: S)
if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
    moveZ = moveZ + 1
end
```

## Action Mapping Alternative

For cleaner code, define action mappings in init:

```lua
-- In init()
bestow.input.registerMapping({
    binding = { deviceType = "Keyboard", keyCode = Keys.Comma },
    action = "move_forward"
})
bestow.input.registerMapping({
    binding = { deviceType = "Keyboard", keyCode = Keys.W },
    action = "move_forward"
})
-- ... etc

-- In update()
if bestow.input.isActionActive("move_forward") then
    moveZ = moveZ - 1
end
```
