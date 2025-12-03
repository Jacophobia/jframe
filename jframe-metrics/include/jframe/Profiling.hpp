// jframe-metrics/include/jframe/Profiling.hpp
// Convenience header for Tracy profiling macros
//
// Include this header in implementation files (.cpp) to get access to
// zero-overhead profiling macros. When TRACY_ENABLE is not defined,
// all macros expand to nothing.
//
// Usage:
//   #include <jframe/Profiling.hpp>
//
//   void MySystem::update(float dt) {
//       JFRAME_ZONE_SCOPED;  // Profile entire function
//
//       {
//           JFRAME_ZONE_NAMED("SubTask");  // Profile named section
//           doSubTask();
//       }
//
//       JFRAME_PLOT("EntityCount", entities.size());  // Plot value over time
//       JFRAME_MESSAGE("Update complete");            // Log message to timeline
//   }

#pragma once

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>

// Profile the current scope (uses function name automatically)
#define JFRAME_ZONE_SCOPED ZoneScoped

// Profile a named zone
#define JFRAME_ZONE_NAMED(name) ZoneScopedN(name)

// Add text context to the current zone
#define JFRAME_ZONE_TEXT(text, len) ZoneText(text, len)

// Mark frame boundary (call once per frame in main loop)
#define JFRAME_FRAME_MARK FrameMark

// Plot a value over time (visible in Tracy timeline)
#define JFRAME_PLOT(name, value) TracyPlot(name, value)

// Log a message to the Tracy timeline
#define JFRAME_MESSAGE(text) TracyMessage(text, strlen(text))

// Log a message with explicit length
#define JFRAME_MESSAGE_L(text, len) TracyMessage(text, len)

// Memory allocation tracking
#define JFRAME_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define JFRAME_FREE(ptr) TracyFree(ptr)

// Named memory allocations (for tracking different allocators)
#define JFRAME_ALLOC_N(ptr, size, name) TracyAllocN(ptr, size, name)
#define JFRAME_FREE_N(ptr, name) TracyFreeN(ptr, name)

// GPU profiling (requires separate GPU context setup)
#define JFRAME_GPU_ZONE(name) TracyGpuZone(name)
#define JFRAME_GPU_COLLECT TracyGpuCollect

#else  // TRACY_ENABLE not defined

// No-op versions when Tracy is disabled
#define JFRAME_ZONE_SCOPED
#define JFRAME_ZONE_NAMED(name)
#define JFRAME_ZONE_TEXT(text, len)
#define JFRAME_FRAME_MARK
#define JFRAME_PLOT(name, value)
#define JFRAME_MESSAGE(text)
#define JFRAME_MESSAGE_L(text, len)
#define JFRAME_ALLOC(ptr, size)
#define JFRAME_FREE(ptr)
#define JFRAME_ALLOC_N(ptr, size, name)
#define JFRAME_FREE_N(ptr, name)
#define JFRAME_GPU_ZONE(name)
#define JFRAME_GPU_COLLECT

#endif  // TRACY_ENABLE

// Color definitions for zones (Tracy supports colored zones)
#ifdef TRACY_ENABLE
#define JFRAME_ZONE_COLOR(color) ZoneScopedC(color)

// Predefined colors for common system types
#define JFRAME_COLOR_PHYSICS    0xFF6384    // Red-ish
#define JFRAME_COLOR_GRAPHICS   0x4BC0C0    // Teal
#define JFRAME_COLOR_AUDIO      0x36A2EB    // Blue
#define JFRAME_COLOR_INPUT      0x9966FF    // Purple
#define JFRAME_COLOR_EVENTS     0xFF9F40    // Orange
#define JFRAME_COLOR_AI         0x00BFFF    // Deep Sky Blue
#define JFRAME_COLOR_ASSETS     0xFF6347    // Tomato
#define JFRAME_COLOR_SAVE       0x32CD32    // Lime Green
#define JFRAME_COLOR_LEVEL      0xFFD700    // Gold
#define JFRAME_COLOR_APPLICATION 0x2E8B57  // Sea Green

#else

#define JFRAME_ZONE_COLOR(color)
#define JFRAME_COLOR_PHYSICS    0
#define JFRAME_COLOR_GRAPHICS   0
#define JFRAME_COLOR_AUDIO      0
#define JFRAME_COLOR_INPUT      0
#define JFRAME_COLOR_EVENTS     0
#define JFRAME_COLOR_AI         0
#define JFRAME_COLOR_ASSETS     0
#define JFRAME_COLOR_SAVE       0
#define JFRAME_COLOR_LEVEL      0
#define JFRAME_COLOR_APPLICATION 0

#endif  // TRACY_ENABLE
