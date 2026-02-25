# Metrics

> **Visibility:** Protected (peer-system only, not exposed to Lua)
> **Tier:** 1
> **Dependencies:** Types
> **Consumers:** All systems (for performance tracking), Dev Tools (for overlay display)

## Purpose

The Metrics system provides a centralized, low-overhead telemetry interface for the entire engine. Systems report counters (monotonic event tallies), gauges (point-in-time values like memory usage or entity count), and timing histograms (duration distributions with percentile breakdowns). The metrics core aggregates these measurements and exposes them through snapshot queries that dev tool overlays, profiling exporters, and diagnostic loggers consume. It is a protected system because game code should not directly manipulate engine telemetry -- metrics are produced by engine systems as a side effect of their operation and consumed by dev tools for visualization and analysis.

## High-Level API

There is no high-level API for the Metrics system. Engine metrics are not exposed to Lua game code. Instead, each engine system records its own performance data through `IMetricsCore` (e.g. the physics system records step timing, the asset system records load counts), and the Dev Tools system reads aggregated snapshots to render debug overlays and performance graphs. Game developers observe metrics through the dev overlay UI, not through script calls.

## Low-Level API: `IMetricsCore`

Full telemetry API for recording counters, gauges, and timing histograms, querying individual metrics, enumerating all registered metric names, and capturing complete snapshots for overlay rendering.

### Counters

| Method | Returns | Description |
|--------|---------|-------------|
| `increment(std::string_view name, float value)` | `void` | Add `value` to the named counter. The default increment is `1.0f`. Counters start at zero on first use. Suitable for tallying events like draw calls, cache hits, or entities spawned. |
| `decrement(std::string_view name, float value)` | `void` | Subtract `value` from the named counter. The default decrement is `1.0f`. Useful for tracking net quantities like active entity count where entities are both created and destroyed. |
| `getCounter(std::string_view name)` | `float` | Return the current value of the named counter. Returns `0.0f` if the counter has never been used. |
| `resetCounter(std::string_view name)` | `void` | Reset the named counter to zero. The counter name remains registered but its value is cleared. |

### Gauges

| Method | Returns | Description |
|--------|---------|-------------|
| `setGauge(std::string_view name, float value)` | `void` | Set the named gauge to an absolute value. Gauges represent point-in-time measurements like current FPS, memory usage in MB, or active particle count. Unlike counters, gauges are overwritten each time they are set rather than accumulated. |
| `getGauge(std::string_view name)` | `float` | Return the current value of the named gauge. Returns `0.0f` if the gauge has never been set. |

### Histograms (Timing / Distribution)

| Method | Returns | Description |
|--------|---------|-------------|
| `recordTiming(std::string_view name, float milliseconds)` | `void` | Record a single duration sample for the named timing histogram. The system maintains a rolling window of samples and computes aggregate statistics (average, min, max, percentiles). Suitable for measuring frame times, system update durations, and asset load times. |
| `beginScope(std::string_view name)` | `void` | Start a named timing scope. Captures the current high-resolution timestamp internally. Must be paired with a matching `endScope()` call using the same name. Scopes may not overlap with the same name on the same thread. |
| `endScope(std::string_view name)` | `void` | End a previously started timing scope and automatically record the elapsed duration as a timing sample. If no matching `beginScope()` was called, this is a no-op with a diagnostic warning. |

### Enumeration

| Method | Returns | Description |
|--------|---------|-------------|
| `getAllCounterNames()` | `std::vector<std::string>` | Return the names of all registered counters, in no particular order. Includes counters that have been reset to zero. |
| `getAllGaugeNames()` | `std::vector<std::string>` | Return the names of all registered gauges, in no particular order. |
| `getAllTimingNames()` | `std::vector<std::string>` | Return the names of all registered timing histograms, in no particular order. |

### Snapshots

| Method | Returns | Description |
|--------|---------|-------------|
| `getSnapshot()` | `MetricsSnapshot` | Capture a consistent, point-in-time snapshot of all counters, gauges, and timing statistics. The snapshot is a deep copy -- it remains valid and stable even as new samples are recorded. Designed for dev overlay rendering, where the overlay reads the snapshot once per frame. |

### Reset

| Method | Returns | Description |
|--------|---------|-------------|
| `resetAll()` | `void` | Reset all counters, gauges, and timing histograms to their initial state. Metric names are preserved (they remain registered) but all accumulated values and samples are cleared. Useful for benchmarking runs where a clean baseline is needed. |

## Types

### MetricsSnapshot

```cpp
struct MetricsSnapshot {
    std::unordered_map<std::string, float> counters;
    std::unordered_map<std::string, float> gauges;
    std::unordered_map<std::string, TimingStats> timings;
};
```

| Field | Type | Description |
|-------|------|-------------|
| `counters` | `std::unordered_map<std::string, float>` | All registered counters and their current values at snapshot time. |
| `gauges` | `std::unordered_map<std::string, float>` | All registered gauges and their current values at snapshot time. |
| `timings` | `std::unordered_map<std::string, TimingStats>` | All registered timing histograms with aggregated statistics computed from the rolling sample window. |

### TimingStats

```cpp
struct TimingStats {
    float avg;
    float min;
    float max;
    float p95;
    float p99;
    int sampleCount;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `avg` | `float` | `0.0f` | Mean duration in milliseconds across all samples in the rolling window. |
| `min` | `float` | `0.0f` | Minimum recorded duration in milliseconds. |
| `max` | `float` | `0.0f` | Maximum recorded duration in milliseconds. |
| `p95` | `float` | `0.0f` | 95th percentile duration in milliseconds. 95% of samples are at or below this value. Useful for identifying tail latency. |
| `p99` | `float` | `0.0f` | 99th percentile duration in milliseconds. Captures worst-case behavior outside of true outliers. |
| `sampleCount` | `int` | `0` | Number of samples in the current rolling window used to compute these statistics. |

## Examples

### Recording system update timings

```cpp
void PhysicsSystem::update(float dt) {
    metrics_->beginScope("physics.step");

    world_->Step(dt, velocityIterations_, positionIterations_);
    syncTransforms();

    metrics_->endScope("physics.step");

    // Track entity counts as a gauge
    metrics_->setGauge("physics.active_bodies", static_cast<float>(world_->GetBodyCount()));
}
```

### Tracking asset loading metrics

```cpp
Result<AssetHandle> AssetSystem::loadAsset(std::string_view path) {
    metrics_->increment("assets.load_requests");

    auto start = std::chrono::high_resolution_clock::now();
    auto result = loadAssetInternal(path);
    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    float ms = std::chrono::duration<float, std::milli>(elapsed).count();
    metrics_->recordTiming("assets.load_time", ms);

    if (!result) {
        metrics_->increment("assets.load_failures");
    }

    return result;
}
```

### Dev overlay snapshot rendering

```cpp
void DevOverlay::renderMetricsPanel(IMetricsCore& metrics) {
    auto snapshot = metrics.getSnapshot();

    // Display frame timing
    if (auto it = snapshot.timings.find("frame.total"); it != snapshot.timings.end()) {
        const auto& stats = it->second;
        drawText("Frame: avg={:.2f}ms  p95={:.2f}ms  p99={:.2f}ms",
                 stats.avg, stats.p95, stats.p99);
        drawText("  min={:.2f}ms  max={:.2f}ms  samples={}",
                 stats.min, stats.max, stats.sampleCount);
    }

    // Display counters
    for (const auto& [name, value] : snapshot.counters) {
        drawText("{}: {:.0f}", name, value);
    }

    // Display gauges
    for (const auto& [name, value] : snapshot.gauges) {
        drawText("{}: {:.2f}", name, value);
    }
}
```

### Benchmarking with clean baseline

```cpp
void BenchmarkRunner::runBenchmark(IMetricsCore& metrics) {
    // Clear all previous data
    metrics.resetAll();

    // Run the benchmark workload
    for (int i = 0; i < 1000; ++i) {
        metrics.beginScope("benchmark.iteration");
        runWorkload();
        metrics.endScope("benchmark.iteration");
    }

    // Read results
    auto snapshot = metrics.getSnapshot();
    auto& results = snapshot.timings.at("benchmark.iteration");
    LOG_INFO("Benchmark complete: avg={:.2f}ms p95={:.2f}ms ({} iterations)",
             results.avg, results.p95, results.sampleCount);
}
```
