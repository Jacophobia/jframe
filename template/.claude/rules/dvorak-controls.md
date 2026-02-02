---
paths: "**/*.lua"
---

# Dvorak Keyboard Layout Support

The project owner uses **Dvorak keyboard layout**. All movement controls MUST support both Dvorak and QWERTY.

## Movement Key Mapping

| Action | Dvorak | QWERTY | KeyCode |
|--------|--------|--------|---------|
| Forward | `,` (comma) | W | `KeyCode.Comma`, `KeyCode.W` |
| Back | O | S | `KeyCode.O`, `KeyCode.S` |
| Left | A | A | `KeyCode.A` (same) |
| Right | E | D | `KeyCode.E`, `KeyCode.D` |

## Required Pattern (Action Builder)

Always register BOTH layouts using the Action Builder:

```lua
local k = KeyCode
-- Forward
bestow.action.builder():duringPhase("gameplay"):whenActive(k.Comma):emitAction("MoveForward"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.W):emitAction("MoveForward"):continuously()
-- Back
bestow.action.builder():duringPhase("gameplay"):whenActive(k.O):emitAction("MoveBack"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.S):emitAction("MoveBack"):continuously()
-- Left (same on both)
bestow.action.builder():duringPhase("gameplay"):whenActive(k.A):emitAction("MoveLeft"):continuously()
-- Right
bestow.action.builder():duringPhase("gameplay"):whenActive(k.E):emitAction("MoveRight"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.D):emitAction("MoveRight"):continuously()
```

## For Direct Polling (Legacy)

If using direct key checks instead of Action Builder, always check both:

```lua
local forward = bestow.input.isKeyDown(KeyCode.Comma) or bestow.input.isKeyDown(KeyCode.W)
local back = bestow.input.isKeyDown(KeyCode.O) or bestow.input.isKeyDown(KeyCode.S)
local left = bestow.input.isKeyDown(KeyCode.A)
local right = bestow.input.isKeyDown(KeyCode.E) or bestow.input.isKeyDown(KeyCode.D)
```
