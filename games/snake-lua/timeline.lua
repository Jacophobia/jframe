-- timeline.lua - Timeline/Sequencer utility for scheduling actions over time
-- Similar to Unreal's Timeline system
--
-- Usage:
--   local tl = app.timeline
--
--   -- Simple delay
--   tl.after(1.5, function() print("1.5 seconds later!") end)
--
--   -- Sequence of actions
--   tl.sequence({
--       {delay = 0.0, action = function() player.startAnimation("attack") end},
--       {delay = 0.3, action = function() audio.playSound("swoosh") end},
--       {delay = 0.5, action = function() enemy.takeDamage(10) end},
--       {delay = 1.0, action = function() player.endAnimation() end},
--   })
--
--   -- Lerp a value over time
--   tl.lerp(0, 100, 2.0, function(value)
--       player.health = value
--   end, function()
--       print("Health restored!")
--   end)
--
--   -- Create a named timeline for control
--   local id = tl.create("deathSequence")
--   tl.addAction(id, 0.0, function() explodeSnake() end)
--   tl.addAction(id, 1.5, function() showGameOver() end)
--   tl.start(id)
--   -- Later: tl.cancel(id) or tl.pause(id)

local timeline = {}

-- Active timers and sequences
local activeTimers = {}
local activeSequences = {}
local activeLerps = {}
local namedTimelines = {}

local nextId = 1

-- Generate unique ID
local function genId()
    local id = nextId
    nextId = nextId + 1
    return id
end

-- Easing functions
timeline.easing = {
    linear = function(t) return t end,
    easeInQuad = function(t) return t * t end,
    easeOutQuad = function(t) return t * (2 - t) end,
    easeInOutQuad = function(t)
        if t < 0.5 then return 2 * t * t end
        return -1 + (4 - 2 * t) * t
    end,
    easeInCubic = function(t) return t * t * t end,
    easeOutCubic = function(t)
        local t1 = t - 1
        return t1 * t1 * t1 + 1
    end,
    easeInOutCubic = function(t)
        if t < 0.5 then return 4 * t * t * t end
        local t1 = 2 * t - 2
        return (t1 * t1 * t1 + 2) / 2
    end,
    easeOutBounce = function(t)
        if t < 1/2.75 then
            return 7.5625 * t * t
        elseif t < 2/2.75 then
            t = t - 1.5/2.75
            return 7.5625 * t * t + 0.75
        elseif t < 2.5/2.75 then
            t = t - 2.25/2.75
            return 7.5625 * t * t + 0.9375
        else
            t = t - 2.625/2.75
            return 7.5625 * t * t + 0.984375
        end
    end,
    easeOutElastic = function(t)
        if t == 0 or t == 1 then return t end
        return math.pow(2, -10 * t) * math.sin((t - 0.075) * (2 * math.pi) / 0.3) + 1
    end,
}

--=============================================================================
-- Simple Timer API
--=============================================================================

--- Schedule an action to run after a delay
-- @param delay Time in seconds
-- @param callback Function to call
-- @return Timer ID (for cancellation)
function timeline.after(delay, callback)
    local id = genId()
    activeTimers[id] = {
        remaining = delay,
        callback = callback,
        paused = false,
    }
    return id
end

--- Cancel a timer
-- @param id Timer ID returned from after()
function timeline.cancelTimer(id)
    activeTimers[id] = nil
end

--=============================================================================
-- Sequence API
--=============================================================================

--- Run a sequence of timed actions
-- @param actions Array of {delay = seconds, action = function}
-- @return Sequence ID
function timeline.sequence(actions)
    local id = genId()

    -- Sort by delay
    local sorted = {}
    for i, a in ipairs(actions) do
        sorted[i] = {delay = a.delay, action = a.action, fired = false}
    end
    table.sort(sorted, function(a, b) return a.delay < b.delay end)

    activeSequences[id] = {
        actions = sorted,
        elapsed = 0,
        paused = false,
        onComplete = nil,
    }
    return id
end

--- Cancel a sequence
function timeline.cancelSequence(id)
    activeSequences[id] = nil
end

--=============================================================================
-- Lerp API
--=============================================================================

--- Lerp a value over time
-- @param from Starting value
-- @param to Ending value
-- @param duration Time in seconds
-- @param onUpdate Called each frame with current value: function(value)
-- @param onComplete Called when done (optional)
-- @param easing Easing function (optional, defaults to linear)
-- @return Lerp ID
function timeline.lerp(from, to, duration, onUpdate, onComplete, easing)
    local id = genId()
    activeLerps[id] = {
        from = from,
        to = to,
        duration = duration,
        elapsed = 0,
        onUpdate = onUpdate,
        onComplete = onComplete,
        easing = easing or timeline.easing.linear,
        paused = false,
    }
    return id
end

--- Lerp a Vec3 over time
function timeline.lerpVec3(from, to, duration, onUpdate, onComplete, easing)
    local id = genId()
    activeLerps[id] = {
        from = {x = from.x, y = from.y, z = from.z},
        to = {x = to.x, y = to.y, z = to.z},
        duration = duration,
        elapsed = 0,
        onUpdate = function(t, easedT)
            local x = from.x + (to.x - from.x) * easedT
            local y = from.y + (to.y - from.y) * easedT
            local z = from.z + (to.z - from.z) * easedT
            onUpdate(Vec3.new(x, y, z))
        end,
        onComplete = onComplete,
        easing = easing or timeline.easing.linear,
        paused = false,
        isVec3 = true,
    }
    return id
end

--- Lerp a Color over time
function timeline.lerpColor(from, to, duration, onUpdate, onComplete, easing)
    local id = genId()
    activeLerps[id] = {
        duration = duration,
        elapsed = 0,
        onUpdate = function(t, easedT)
            local r = from.r + (to.r - from.r) * easedT
            local g = from.g + (to.g - from.g) * easedT
            local b = from.b + (to.b - from.b) * easedT
            local a = from.a + (to.a - from.a) * easedT
            onUpdate(Color.new(r, g, b, a))
        end,
        onComplete = onComplete,
        easing = easing or timeline.easing.linear,
        paused = false,
        isColor = true,
    }
    return id
end

--- Cancel a lerp
function timeline.cancelLerp(id)
    activeLerps[id] = nil
end

--=============================================================================
-- Named Timeline API (for complex sequences)
--=============================================================================

--- Create a named timeline
-- @param name Unique name for the timeline
-- @return Timeline ID
function timeline.create(name)
    local id = name or genId()
    namedTimelines[id] = {
        actions = {},
        elapsed = 0,
        running = false,
        paused = false,
        loop = false,
        duration = 0,
        onComplete = nil,
    }
    return id
end

--- Add an action to a named timeline
-- @param id Timeline ID
-- @param time Time in seconds when action should fire
-- @param action Function to call
function timeline.addAction(id, time, action)
    local tl = namedTimelines[id]
    if not tl then return end

    table.insert(tl.actions, {time = time, action = action, fired = false})
    tl.duration = math.max(tl.duration, time)

    -- Keep sorted
    table.sort(tl.actions, function(a, b) return a.time < b.time end)
end

--- Set timeline to loop
function timeline.setLoop(id, loop)
    local tl = namedTimelines[id]
    if tl then tl.loop = loop end
end

--- Set completion callback
function timeline.setOnComplete(id, callback)
    local tl = namedTimelines[id]
    if tl then tl.onComplete = callback end
end

--- Start a named timeline
function timeline.start(id)
    local tl = namedTimelines[id]
    if not tl then return end

    tl.running = true
    tl.paused = false
    tl.elapsed = 0
    for _, a in ipairs(tl.actions) do
        a.fired = false
    end
end

--- Pause a timeline
function timeline.pause(id)
    local tl = namedTimelines[id]
    if tl then tl.paused = true end
end

--- Resume a timeline
function timeline.resume(id)
    local tl = namedTimelines[id]
    if tl then tl.paused = false end
end

--- Stop and reset a timeline
function timeline.stop(id)
    local tl = namedTimelines[id]
    if not tl then return end

    tl.running = false
    tl.elapsed = 0
    for _, a in ipairs(tl.actions) do
        a.fired = false
    end
end

--- Cancel (remove) a timeline entirely
function timeline.cancel(id)
    namedTimelines[id] = nil
end

--- Check if timeline is running
function timeline.isRunning(id)
    local tl = namedTimelines[id]
    return tl and tl.running and not tl.paused
end

--=============================================================================
-- Update (call every frame)
--=============================================================================

--- Update all timelines - call this every frame with delta time
-- @param dt Delta time in seconds
function timeline.update(dt)
    -- Update simple timers
    for id, timer in pairs(activeTimers) do
        if not timer.paused then
            timer.remaining = timer.remaining - dt
            if timer.remaining <= 0 then
                timer.callback()
                activeTimers[id] = nil
            end
        end
    end

    -- Update sequences
    for id, seq in pairs(activeSequences) do
        if not seq.paused then
            seq.elapsed = seq.elapsed + dt

            local allFired = true
            for _, action in ipairs(seq.actions) do
                if not action.fired then
                    if seq.elapsed >= action.delay then
                        action.action()
                        action.fired = true
                    else
                        allFired = false
                    end
                end
            end

            if allFired then
                if seq.onComplete then seq.onComplete() end
                activeSequences[id] = nil
            end
        end
    end

    -- Update lerps
    for id, lerp in pairs(activeLerps) do
        if not lerp.paused then
            lerp.elapsed = lerp.elapsed + dt
            local t = math.min(1.0, lerp.elapsed / lerp.duration)
            local easedT = lerp.easing(t)

            if lerp.isVec3 or lerp.isColor then
                lerp.onUpdate(t, easedT)
            else
                local value = lerp.from + (lerp.to - lerp.from) * easedT
                lerp.onUpdate(value)
            end

            if t >= 1.0 then
                if lerp.onComplete then lerp.onComplete() end
                activeLerps[id] = nil
            end
        end
    end

    -- Update named timelines
    for id, tl in pairs(namedTimelines) do
        if tl.running and not tl.paused then
            tl.elapsed = tl.elapsed + dt

            for _, action in ipairs(tl.actions) do
                if not action.fired and tl.elapsed >= action.time then
                    action.action()
                    action.fired = true
                end
            end

            -- Check if complete
            if tl.elapsed >= tl.duration then
                local allFired = true
                for _, a in ipairs(tl.actions) do
                    if not a.fired then allFired = false break end
                end

                if allFired then
                    if tl.loop then
                        -- Reset for loop
                        tl.elapsed = 0
                        for _, a in ipairs(tl.actions) do
                            a.fired = false
                        end
                    else
                        tl.running = false
                        if tl.onComplete then tl.onComplete() end
                    end
                end
            end
        end
    end
end

--=============================================================================
-- Utility: Clear all
--=============================================================================

--- Clear all active timers, sequences, lerps, and timelines
function timeline.clearAll()
    activeTimers = {}
    activeSequences = {}
    activeLerps = {}
    namedTimelines = {}
end

--- Get counts for debugging
function timeline.getStats()
    local timerCount = 0
    for _ in pairs(activeTimers) do timerCount = timerCount + 1 end

    local seqCount = 0
    for _ in pairs(activeSequences) do seqCount = seqCount + 1 end

    local lerpCount = 0
    for _ in pairs(activeLerps) do lerpCount = lerpCount + 1 end

    local tlCount = 0
    for _ in pairs(namedTimelines) do tlCount = tlCount + 1 end

    return {
        timers = timerCount,
        sequences = seqCount,
        lerps = lerpCount,
        timelines = tlCount,
    }
end

return timeline
