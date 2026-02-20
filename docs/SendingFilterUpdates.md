# Sending Filter Updates

This document explains how to send progress and status messages from filter
algorithms using the `MessageHelper` system defined in
`src/simplnx/Utilities/MessageHelper.hpp`.

## Overview

The messaging system provides three mechanisms for sending messages from filters:

1. **Synchronous messages** -- guaranteed delivery, used for infrequent events
   (e.g., algorithm start, phase transitions, completion)
2. **Throttled messages** -- rate-limited to avoid performance overhead in tight
   loops. A background timer thread handles clock checks and string formatting.
3. **Progress tracking** -- atomic counter for multi-threaded progress
   accumulation with throttled output.

## Architecture

A singleton `MessageDispatcher` owns a background timer thread that wakes
every 100ms and flushes any registered throttled channels. When you create
a `ThrottledMessenger` or `ProgressHelper`, an internal channel is registered
with the dispatcher. Your filter's hot loop only stores raw values -- the
expensive clock checking and `fmt::format()` string construction happen on
the background thread.

```
Filter Thread (hot loop)           Background Timer Thread (100ms tick)
  |                                   |
  |  sendMessage(args...)             |
  |    -> storeArgs()  [~10-25ns]     |
  |                                   |  tryFlush()
  |                                   |    -> check atomic flag
  |                                   |    -> check interval
  |                                   |    -> format string
  |                                   |    -> call MessageHandler
```

## Quick Start

Include the header and create a `MessageHelper` from the filter's message
handler:

```cpp
#include "simplnx/Utilities/MessageHelper.hpp"

// In your algorithm's operator()():
MessageHelper messageHelper(m_MessageHandler);
```

## Synchronous Messages

Use `sendMessage()` for guaranteed delivery of infrequent messages. This calls
the `MessageHandler` directly on the calling thread:

```cpp
MessageHelper messageHelper(m_MessageHandler);
messageHelper.sendMessage("Starting Phase 1: Initialization");
// ... do work ...
messageHelper.sendMessage("Phase 1 complete. Starting Phase 2.");
```

## Throttled Messages

Use `createThrottledMessenger()` for messages inside tight loops. The formatter
lambda defines both the message format and the argument types:

### Single Argument (Atomic Specialization)

When the formatter takes a single `usize` argument, the system uses an
optimized lock-free atomic path (~10ns per call):

```cpp
MessageHelper messageHelper(m_MessageHandler);
usize totalVoxels = imageGeom.getNumberOfCells();

auto throttledMessenger = messageHelper.createThrottledMessenger(
    [totalVoxels](usize current)
    {
      return fmt::format("{}/{} ({:.1f}%)", current, totalVoxels,
                         CalculatePercentComplete(current, totalVoxels));
    });

for(usize voxelIndex = 0; voxelIndex < totalVoxels; voxelIndex++)
{
  // ... do work ...
  throttledMessenger.sendMessage(voxelIndex);  // ~10ns, no string construction
}
// On destruction, finalFlush() sends the last pending message automatically.
```

### Multiple Arguments

When the formatter takes multiple arguments, the system uses a mutex-guarded
tuple to store the latest values (~15-25ns per call):

```cpp
auto throttledMessenger = messageHelper.createThrottledMessenger(
    [](usize iteration, usize voxel, float32 error)
    {
      return fmt::format("Iteration {}: Voxel {} | Error: {:.4f}",
                         iteration, voxel, error);
    });

for(usize iter = 0; iter < maxIterations; iter++)
{
  for(usize voxel = 0; voxel < totalVoxels; voxel++)
  {
    // ... do work ...
    throttledMessenger.sendMessage(iter, voxel, currentError);
  }
}
```

### Custom Interval

The default throttle interval is 1000ms (1 second). You can customize it:

```cpp
// Send at most every 500ms
auto messenger = messageHelper.createThrottledMessenger(
    [total](usize current)
    {
      return fmt::format("{}/{}", current, total);
    },
    std::chrono::milliseconds(500));
```

## Progress Tracking (Multi-Threaded)

Use `createProgressHelper()` when multiple threads need to contribute to a
shared progress counter. This is the recommended pattern for parallel
algorithms:

```cpp
MessageHelper messageHelper(m_MessageHandler);
usize totalWork = numFeatures;

auto progressHelper = messageHelper.createProgressHelper(
    totalWork,
    [](usize current, usize max)
    {
      return fmt::format("Processing features: {}/{} ({:.1f}%)",
                         current, max,
                         CalculatePercentComplete(current, max));
    });

// Create one worker handle per thread
std::vector<std::thread> threads;
for(int t = 0; t < numThreads; t++)
{
  auto worker = progressHelper.createWorkerHandle();
  threads.emplace_back([w = std::move(worker), ...]() mutable
  {
    for(usize i = rangeStart; i < rangeEnd; i++)
    {
      // ... do work ...
      w.incrementProgress(1);  // Atomic increment, ~10ns
    }
  });
}

for(auto& th : threads)
{
  th.join();
}
// On destruction, finalFlush() reports the final progress count.
```

### Resetting Progress

If your algorithm has multiple phases that each need fresh progress tracking,
you can reset the counter. Only call this when no workers are active:

```cpp
progressHelper.resetProgress();
```

## Utility: CalculatePercentComplete

A convenience template for computing percentage values:

```cpp
#include "simplnx/Utilities/MessageHelper.hpp"

// Returns float32 by default
float32 pct = CalculatePercentComplete(current, total);

// Or specify a different type
int32 pctInt = CalculatePercentComplete<int32>(current, total);
```

## API Reference

### MessageHelper

| Method | Description |
|--------|-------------|
| `MessageHelper(const IFilter::MessageHandler& handler)` | Constructor |
| `void sendMessage(std::string message)` | Synchronous send (guaranteed delivery) |
| `auto createThrottledMessenger(formatter, interval)` | Create a throttled messenger |
| `ProgressHelper createProgressHelper(max, formatter, interval)` | Create a progress helper |

### ThrottledMessenger\<Args...\>

| Method | Description |
|--------|-------------|
| `void sendMessage(Args... args)` | Store latest values (hot path) |

- Move-only (non-copyable)
- `finalFlush()` is called automatically on destruction
- Argument types are deduced from the formatter lambda

### ProgressHelper

| Method | Description |
|--------|-------------|
| `ProgressWorker createWorkerHandle()` | Create a per-thread worker |
| `void resetProgress()` | Reset counter to zero |

### ProgressWorker

| Method | Description |
|--------|-------------|
| `void incrementProgress(usize amount = 1)` | Atomic increment (hot path) |

## Migration from Old API

If you are updating code that uses the old `ProgressMessenger`,
`ProgressMessageHelper`, or old `ThrottledMessenger` APIs, here is the mapping:

### Old ThrottledMessenger Pattern

```cpp
// OLD:
MessageHelper messageHelper(m_MessageHandler, throttleRate);
auto messenger = messageHelper.createThrottledMessenger(throttleRate);
messenger.sendThrottledMessage([&]() {
  return fmt::format("{}/{}", current, total);
});

// NEW:
MessageHelper messageHelper(m_MessageHandler);
auto messenger = messageHelper.createThrottledMessenger(
    [total](usize current) {
      return fmt::format("{}/{}", current, total);
    });
messenger.sendMessage(current);
```

### Old ProgressMessenger Pattern

```cpp
// OLD:
MessageHelper messageHelper(m_MessageHandler, throttleRate);
auto progressMessenger = messageHelper.createProgressMessenger(totalWork);
progressMessenger.sendProgressMessage(currentIndex);

// NEW:
MessageHelper messageHelper(m_MessageHandler);
auto progressHelper = messageHelper.createProgressHelper(
    totalWork,
    [](usize current, usize max) {
      return fmt::format("{}/{} ({:.1f}%)", current, max,
                         CalculatePercentComplete(current, max));
    });
auto worker = progressHelper.createWorkerHandle();
worker.incrementProgress(1);
```

### Key Migration Points

1. `MessageHelper` constructor no longer takes a `throttleRate` parameter
2. The formatter lambda is passed to `createThrottledMessenger()`, not to `sendMessage()`
3. `sendThrottledMessage(lambda)` becomes `sendMessage(values...)`
4. String construction is deferred -- the formatter is only called by the
   background thread when a message is actually sent
5. `ProgressMessageHelper` is replaced by `ProgressHelper` + `ProgressWorker`
