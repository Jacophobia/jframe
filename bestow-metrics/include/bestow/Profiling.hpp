// bestow-metrics/include/bestow/Profiling.hpp
// Convenience header for Tracy profiling macros
//
// Include this header in implementation files (.cpp) to get access to
// zero-overhead profiling macros. When TRACY_ENABLE is not defined,
// all macros expand to nothing.
//
// Usage:
//   #include <bestow/Profiling.hpp>
//
//   void MySystem::update(float dt) {
//       BESTOW_ZONE_SCOPED;  // Profile entire function
//
//       {
//           BESTOW_ZONE_NAMED("SubTask");  // Profile named section
//           doSubTask();
//       }
//
//       BESTOW_PLOT("EntityCount", entities.size());  // Plot value over time
//       BESTOW_MESSAGE("Update complete");            // Log message to timeline
//   }

#pragma once

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>

// Profile the current scope (uses function name automatically)
#define BESTOW_ZONE_SCOPED ZoneScoped

// Profile a named zone
#define BESTOW_ZONE_NAMED(name) ZoneScopedN(name)

// Add text context to the current zone
#define BESTOW_ZONE_TEXT(text, len) ZoneText(text, len)

// Mark frame boundary (call once per frame in main loop)
#define BESTOW_FRAME_MARK FrameMark

// Plot a value over time (visible in Tracy timeline)
#define BESTOW_PLOT(name, value) TracyPlot(name, value)

// Log a message to the Tracy timeline
#define BESTOW_MESSAGE(text) TracyMessage(text, strlen(text))

// Log a message with explicit length
#define BESTOW_MESSAGE_L(text, len) TracyMessage(text, len)

// Memory allocation tracking
#define BESTOW_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define BESTOW_FREE(ptr) TracyFree(ptr)

// Named memory allocations (for tracking different allocators)
#define BESTOW_ALLOC_N(ptr, size, name) TracyAllocN(ptr, size, name)
#define BESTOW_FREE_N(ptr, name) TracyFreeN(ptr, name)

// GPU profiling (requires separate GPU context setup)
#define BESTOW_GPU_ZONE(name) TracyGpuZone(name)
#define BESTOW_GPU_COLLECT TracyGpuCollect

#else  // TRACY_ENABLE not defined

// No-op versions when Tracy is disabled
#define BESTOW_ZONE_SCOPED
#define BESTOW_ZONE_NAMED(name)
#define BESTOW_ZONE_TEXT(text, len)
#define BESTOW_FRAME_MARK
#define BESTOW_PLOT(name, value)
#define BESTOW_MESSAGE(text)
#define BESTOW_MESSAGE_L(text, len)
#define BESTOW_ALLOC(ptr, size)
#define BESTOW_FREE(ptr)
#define BESTOW_ALLOC_N(ptr, size, name)
#define BESTOW_FREE_N(ptr, name)
#define BESTOW_GPU_ZONE(name)
#define BESTOW_GPU_COLLECT

#endif  // TRACY_ENABLE

// Color definitions for zones (Tracy supports colored zones)
#ifdef TRACY_ENABLE
#define BESTOW_ZONE_COLOR(color) ZoneScopedC(color)

// Predefined colors for common system types
#define BESTOW_COLOR_PHYSICS    0xFF6384    // Red-ish
#define BESTOW_COLOR_GRAPHICS   0x4BC0C0    // Teal
#define BESTOW_COLOR_AUDIO      0x36A2EB    // Blue
#define BESTOW_COLOR_INPUT      0x9966FF    // Purple
#define BESTOW_COLOR_EVENTS     0xFF9F40    // Orange
#define BESTOW_COLOR_AI         0x00BFFF    // Deep Sky Blue
#define BESTOW_COLOR_ASSETS     0xFF6347    // Tomato
#define BESTOW_COLOR_SAVE       0x32CD32    // Lime Green
#define BESTOW_COLOR_LEVEL      0xFFD700    // Gold
#define BESTOW_COLOR_APPLICATION 0x2E8B57  // Sea Green

#else

#define BESTOW_ZONE_COLOR(color)
#define BESTOW_COLOR_PHYSICS    0
#define BESTOW_COLOR_GRAPHICS   0
#define BESTOW_COLOR_AUDIO      0
#define BESTOW_COLOR_INPUT      0
#define BESTOW_COLOR_EVENTS     0
#define BESTOW_COLOR_AI         0
#define BESTOW_COLOR_ASSETS     0
#define BESTOW_COLOR_SAVE       0
#define BESTOW_COLOR_LEVEL      0
#define BESTOW_COLOR_APPLICATION 0

#endif  // TRACY_ENABLE
