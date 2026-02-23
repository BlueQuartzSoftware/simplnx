# Sending Filter Updates

This document explains how to send progress and status messages from filter
algorithms using the `MessageHelper` system defined in
`src/simplnx/Utilities/MessageHelper.hpp`.

## Overview

The messaging system provides three mechanisms for sending messages from filters:

1. **Synchronous messages** -- guaranteed delivery, used for infrequent events
   (algorithm start, phase transitions, completion)
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

Use this for messages that must not be dropped -- algorithm start/end markers,
phase transitions, or per-array status lines before dispatching parallel work.

## Throttled Messages (Single-Threaded Loops)

Use `createThrottledMessenger()` for messages inside tight loops. The formatter
lambda defines both the message format and the argument types. Only the latest
values are kept; the background thread formats and sends at most once per
interval.

### Single Argument (Atomic Specialization)

When the formatter takes a single `usize` argument, the system uses an
optimized lock-free atomic path (~10ns per call):

```cpp
// From BadDataNeighborOrientationCheck
MessageHelper messageHelper(m_MessageHandler);
usize totalPoints = imageGeom.getNumberOfCells();

auto throttledMessenger = messageHelper.createThrottledMessenger(
    [totalPoints](usize voxelIdx)
    {
      return fmt::format("Processing Data {:.2f}% completed",
                         CalculatePercentComplete(voxelIdx, totalPoints));
    });

for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
{
  // ... do work ...
  throttledMessenger.sendMessage(static_cast<usize>(voxelIndex));  // ~10ns
}
// On destruction, finalFlush() sends the last pending message automatically.
```

### Multiple Arguments

When the formatter takes multiple arguments, the system uses a mutex-guarded
tuple to store the latest values (~15-25ns per call):

```cpp
// From FillBadData -- two arguments
auto throttledMessenger = messageHelper.createThrottledMessenger(
    [](usize iteration, usize count)
    {
      return fmt::format("  Iteration {}: {} voxels remaining to fill",
                         iteration, count);
    });

for(usize iter = 0; iter < maxIterations; iter++)
{
  // ... fill pass ...
  throttledMessenger.sendMessage(iter, remainingCount);
}
```

```cpp
// From NeighborOrientationCorrelation -- three arguments with captures
auto levelMessenger = messageHelper.createThrottledMessenger(
    [totalLevels, totalPoints](int32 levelNum, int32 loopNum, usize voxelIdx)
    {
      return fmt::format("Level '{}' of '{}' || Processing Data ('{}') {:.2f}% completed",
                         levelNum, totalLevels, loopNum,
                         CalculatePercentComplete(voxelIdx, totalPoints));
    });
```

### Custom Interval

The default throttle interval is 1000ms (1 second). You can customize it:

```cpp
auto messenger = messageHelper.createThrottledMessenger(
    [total](usize current)
    {
      return fmt::format("{}/{}", current, total);
    },
    std::chrono::milliseconds(500));  // Send at most every 500ms
```

### Per-Task String Messages

For task-parallel workers that send descriptive per-slice or per-step messages,
create a local `ThrottledMessenger<std::string>` inside `operator()`:

```cpp
// From RotateImageGeometryWithNearestNeighbor
void convert() const
{
  ThrottledMessenger<std::string> messenger(
      [](const std::string& msg) { return msg; },
      m_MessageHandler, std::chrono::milliseconds(1000));

  for(int64 k = 0; k < m_Params.outputDims[2]; k++)
  {
    messenger.sendMessage(fmt::format("{}: Interpolating values for slice '{}/{}'",
                                      m_SourceArray->getName(), k, m_Params.outputDims[2]));
    // ... process slice ...
  }
  // Use direct handler for guaranteed-delivery messages
  m_MessageHandler(fmt::format("{}: Transform Ending", m_SourceArray->getName()));
}
```

## Progress Tracking (Multi-Threaded)

Use `createProgressHelper()` when multiple worker threads need to contribute
to a shared progress counter. This is the recommended pattern for
`ParallelTaskAlgorithm` and `ParallelDataAlgorithm` workloads.

The `ProgressHelper` owns an atomic counter and registers it with the
background timer thread. Each `ProgressWorker` handle shares the same counter
and can safely call `incrementProgress()` from any thread.

### With ParallelTaskAlgorithm (Task-Parallel)

Each task gets its own `ProgressWorker` handle via `createWorkerHandle()`.
All workers atomically contribute to one shared counter:

```cpp
// From ResampleImageGeom -- multiple arrays processed in parallel
MessageHelper messageHelper(m_MessageHandler);

usize totalArrays = srcCellDataAM.getSize();
usize numVoxels = destImageGeom.getNumberOfCells();
usize totalProgress = numVoxels * totalArrays;

ProgressHelper progressHelper = messageHelper.createProgressHelper(
    totalProgress,
    [](usize current, usize max)
    {
      return fmt::format("Resampling: {:.0f}% Complete",
                         CalculatePercentComplete(current, max));
    });

ParallelTaskAlgorithm taskRunner;
taskRunner.setParallelizationEnabled(true);

for(const auto& [dataId, oldDataObject] : srcCellDataAM)
{
  const auto& oldDataArray = dynamic_cast<const IDataArray&>(*oldDataObject);
  auto& newDataArray = dynamic_cast<IDataArray&>(destCellDataAM.at(oldDataArray.getName()));

  // Each task gets its own ProgressWorker handle
  ExecuteParallelFunction<ResampleImageGeomArrayImpl>(
      oldDataArray.getDataType(), taskRunner,
      progressHelper.createWorkerHandle(),
      oldDataArray, newDataArray, selectedImageGeom, destImageGeom, m_ShouldCancel);
}

taskRunner.wait();
// On destruction, progressHelper sends the final progress count.
```

Inside the worker class:

```cpp
template <typename T>
class ResampleImageGeomArrayImpl
{
public:
  ResampleImageGeomArrayImpl(ProgressWorker progressWorker, /* other args */)
  : m_ProgressWorker(std::move(progressWorker))
  // ...
  {
  }

  void operator()() const
  {
    for(usize idx = 0; idx < numVoxels; idx++)
    {
      // ... do work ...
      m_ProgressWorker.incrementProgress(1);  // Atomic increment, ~10ns
    }
  }

private:
  mutable ProgressWorker m_ProgressWorker;  // mutable because operator() is const
  // ...
};
```

### With ParallelDataAlgorithm (Range-Parallel)

For range-split parallelism, each range chunk shares the same `ProgressWorker`:

```cpp
// From InterpolateValuesToUnstructuredGrid
MessageHelper messageHelper(m_MessageHandler);
usize totalVertices = destGeometry.getNumberOfVertices();

ProgressHelper progressHelper = messageHelper.createProgressHelper(
    totalVertices,
    [](usize current, usize max)
    {
      return fmt::format("Calculating Closest Vertices || {:.0f}%",
                         CalculatePercentComplete(current, max));
    });

ParallelDataAlgorithm dataAlg;
dataAlg.setRange(0ULL, totalVertices);
dataAlg.execute(CalculateClosestVerticesImpl(
    progressHelper.createWorkerHandle(),
    srcGeometry, destGeometry, closestSrcIds, m_ShouldCancel));

messageHelper.sendMessage("Calculating Closest Vertices || 100%");
```

Inside the range worker:

```cpp
class CalculateClosestVerticesImpl
{
public:
  CalculateClosestVerticesImpl(ProgressWorker progressWorker, /* other args */)
  : m_ProgressWorker(std::move(progressWorker))
  // ...
  {
  }

  void operator()(const Range& range) const
  {
    for(usize i = range.min(); i < range.max(); i++)
    {
      // ... do work ...
      m_ProgressWorker.incrementProgress(1);
    }
  }

private:
  mutable ProgressWorker m_ProgressWorker;
  // ...
};
```

### Resetting Progress

If your algorithm has multiple phases that each need fresh progress tracking,
you can reset the counter. Only call this when no workers are active:

```cpp
progressHelper.resetProgress();
```

## Choosing the Right Pattern

| Scenario | Mechanism | Hot-path cost |
|----------|-----------|---------------|
| Infrequent messages (start/end) | `messageHelper.sendMessage()` | N/A (not in hot loop) |
| Single-threaded tight loop | `ThrottledMessenger<usize>` | ~10ns (atomic store) |
| Single-threaded loop, multiple values | `ThrottledMessenger<Args...>` | ~15-25ns (mutex + store) |
| Multi-threaded, shared counter | `ProgressHelper` + `ProgressWorker` | ~10ns (atomic fetch_add) |
| Task-parallel, descriptive strings | Local `ThrottledMessenger<std::string>` | ~15-25ns (mutex + store) |

**Key design principle**: `ThrottledMessenger` is for single-threaded (or
single-owner) use where one thread stores values and the background thread
sends. `ProgressHelper`/`ProgressWorker` is for multi-threaded use where
many threads atomically increment a shared counter.

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
| `auto createThrottledMessenger(formatter, interval)` | Create a throttled messenger (default 1000ms) |
| `ProgressHelper createProgressHelper(max, formatter, interval)` | Create a progress helper (default 1000ms) |

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
| `ProgressWorker createWorkerHandle()` | Create a per-thread worker (copyable) |
| `void resetProgress()` | Reset counter to zero (no active workers) |

- Move-only (non-copyable)
- `finalFlush()` is called automatically on destruction

### ProgressWorker

| Method | Description |
|--------|-------------|
| `void incrementProgress(usize amount = 1)` | Atomic increment (hot path) |

- Copyable (shares the underlying channel via `shared_ptr`)
- Mark as `mutable` when stored in a class with a `const operator()`

## Migration from Old API

### Old ThrottledMessenger (lambda-per-call)

```cpp
// OLD:
auto messenger = messageHelper.createThrottledMessenger();
messenger.sendThrottledMessage([&]() {
  return fmt::format("{}/{}", current, total);
});

// NEW:
auto messenger = messageHelper.createThrottledMessenger(
    [total](usize current) {
      return fmt::format("{}/{}", current, total);
    });
messenger.sendMessage(current);
```

### Old sendThreadSafeProgressMessage Pattern

```cpp
// OLD: Algorithm class with manual mutex + clock checks
class MyAlgorithm {
  std::mutex m_Mutex;
  std::chrono::steady_clock::time_point m_InitialPoint;
  size_t m_ProgressCounter = 0;
  size_t m_TotalElements = 0;

  void sendThreadSafeProgressMessage(usize counter) {
    std::lock_guard<std::mutex> guard(m_Mutex);
    m_ProgressCounter += counter;
    auto now = std::chrono::steady_clock::now();
    // ... time check, format, send ...
  }
};

// Workers stored algorithm pointer:
class MyWorker {
  MyAlgorithm* m_Algorithm;
  void operator()(const Range& range) const {
    for(usize i = range.min(); i < range.max(); i++) {
      // ... work ...
      m_Algorithm->sendThreadSafeProgressMessage(1);
    }
  }
};

// NEW: Workers store ProgressWorker directly
class MyWorker {
  mutable ProgressWorker m_ProgressWorker;
  void operator()(const Range& range) const {
    for(usize i = range.min(); i < range.max(); i++) {
      // ... work ...
      m_ProgressWorker.incrementProgress(1);
    }
  }
};

// Algorithm creates ProgressHelper, passes worker handles:
ProgressHelper progressHelper = messageHelper.createProgressHelper(
    totalElements,
    [](usize current, usize max) {
      return fmt::format("Processing: {:.0f}%", CalculatePercentComplete(current, max));
    });
dataAlg.execute(MyWorker(progressHelper.createWorkerHandle(), ...));
```

### Old FilterProgressCallback Pattern

```cpp
// OLD: Shared callback object with static mutexes
ImageRotationUtilities::FilterProgressCallback callback(m_MessageHandler, m_ShouldCancel);
ExecuteParallelFunction<MyWorker>(..., &callback);

// NEW (image geometry transforms): Pass handler + cancel directly
ExecuteParallelFunction<MyWorker>(..., m_MessageHandler, m_ShouldCancel);
// Worker creates a local ThrottledMessenger<std::string> in operator()

// NEW (node geometry transforms): Use ProgressHelper
ProgressHelper progressHelper = messageHelper.createProgressHelper(totalVertices, formatter);
dataAlg.execute(MyWorker(vertexList, matrix, progressHelper.createWorkerHandle(), m_ShouldCancel));
```

### Key Migration Points

1. `MessageHelper` constructor no longer takes a `throttleRate` parameter
2. The formatter lambda is passed to `createThrottledMessenger()`, not to `sendMessage()`
3. `sendThrottledMessage(lambda)` becomes `sendMessage(values...)`
4. String construction is deferred -- the formatter is only called by the
   background thread when a message is actually sent
5. `sendThreadSafeProgressMessage()` methods on algorithm classes are replaced
   by `ProgressWorker` handles passed directly to worker classes
6. `FilterProgressCallback` is removed -- use `ThrottledMessenger<std::string>`
   for string messages or `ProgressHelper`/`ProgressWorker` for counters
