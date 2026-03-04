# Sending Filter Updates

This document explains how to send progress and status messages from filter
algorithms using the `FilterMessenger` class defined in
`src/simplnx/Filter/FilterMessenger.hpp`.

## Overview

The messaging system provides three mechanisms for sending messages from filters:

1. **Synchronous messages** — guaranteed delivery, used for infrequent events
   (algorithm start, phase transitions, completion)
2. **Throttled messages** — rate-limited to avoid performance overhead in tight
   loops. A background timer thread handles clock checks and string formatting.
3. **Progress tracking** — atomic counter for multi-threaded progress
   accumulation with throttled output.

## Architecture

Each `FilterMessenger` instance owns a background timer thread (started
lazily on the first throttled call). The thread wakes every 100ms and flushes
any registered channels. Your filter's hot loop only stores a raw `usize`
value atomically — the expensive clock check and `fmt::format()` string
construction happen on the background thread.

```
Filter Thread (hot loop)           Background Timer Thread (100ms tick)
  |                                   |
  |  sendThrottledMessage(i)          |
  |    -> atomic store  [~10ns]       |
  |                                   |  tryFlush()
  |                                   |    -> check atomic flag
  |                                   |    -> check interval
  |                                   |    -> formatter(value)
  |                                   |    -> call MessageHandler
```

The timer thread is destroyed (and a final flush is performed) when the
`FilterMessenger` goes out of scope — no singleton, no shared state between
filters.

## Quick Start

Include the header and construct a `FilterMessenger` from the filter's message
handler:

```cpp
#include "simplnx/Filter/FilterMessenger.hpp"

// In your algorithm's operator()():
FilterMessenger filterMessenger(m_MessageHandler);
```

## Synchronous Messages

Use the typed send methods for guaranteed delivery of infrequent messages.
These call the `MessageHandler` directly on the calling thread:

```cpp
FilterMessenger filterMessenger(m_MessageHandler);

filterMessenger.sendInfo("Starting Phase 1: Initialization");
// ... do work ...
filterMessenger.sendInfo("Phase 1 complete. Starting Phase 2.");
filterMessenger.sendWarning("Optional mask array not found; skipping masking.");
filterMessenger.sendError("Geometry dimensions do not match.");
filterMessenger.sendProgress("Loading data...", 25);  // 25% complete
```

| Method | `Message::Type` | Use when |
|--------|-----------------|----------|
| `sendInfo(msg)` | `Info` | Normal status lines |
| `sendDebug(msg)` | `Debug` | Verbose diagnostic output |
| `sendWarning(msg)` | `Warning` | Non-fatal issues |
| `sendError(msg)` | `Error` | Fatal errors (also return a Result error) |
| `sendProgress(msg, pct)` | `Progress` | Explicit percentage updates |

Use synchronous sends for messages that must not be dropped — algorithm
start/end markers, phase transitions, or per-array status lines before
dispatching parallel work.

## Throttled Messages (Single-Threaded Loops)

For messages inside tight loops, set the formatter once with
`setThrottledFormatter()` and then call `sendThrottledMessage(i)` on every
iteration. The hot path is a single atomic store (~10ns). The background
thread calls the formatter and delivers the message at most once per interval.

### Basic Pattern

```cpp
FilterMessenger filterMessenger(m_MessageHandler);
usize totalPoints = imageGeom.getNumberOfCells();

filterMessenger.setThrottledFormatter(
    [totalPoints](usize voxelIdx)
    {
      return fmt::format("Processing Data {:.2f}% completed",
                         CalculatePercentComplete(voxelIdx, totalPoints));
    });

for(usize voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
{
  // ... do work ...
  filterMessenger.sendThrottledMessage(voxelIndex);  // ~10ns
}
// On destruction, FilterMessenger sends the last pending value automatically.
```

`setThrottledFormatter` must be called before the first `sendThrottledMessage`
call. If you need different messages for different phases of the same algorithm,
call `setThrottledFormatter` again before each phase's loop — it replaces the
previous formatter and channel.

### Custom Interval

The default throttle interval is 1000ms (1 second). Pass a second argument
to `setThrottledFormatter` to change it:

```cpp
filterMessenger.setThrottledFormatter(
    [total](usize current)
    {
      return fmt::format("{}/{}", current, total);
    },
    std::chrono::milliseconds(500));  // Send at most every 500ms
```

### Multi-Value Messages (Capturing Secondary Values)

`sendThrottledMessage` takes a single `usize`. If your message logically
depends on a second value that changes each iteration, capture a shared
atomic or reformulate the message around the primary progress value:

```cpp
// Option A: show percentage only (simplest)
filterMessenger.setThrottledFormatter(
    [maxIterations](usize iter)
    {
      return fmt::format("{:.2f}% Complete", CalculatePercentComplete(iter, maxIterations));
    });

// Option B: capture a loop-constant secondary value in the closure
usize chunkSize = computeChunkSize();
filterMessenger.setThrottledFormatter(
    [chunkSize, total](usize processed)
    {
      return fmt::format("Processed {} of {} (chunk size {})", processed, total, chunkSize);
    });
```

### Per-Slice String Messages (Image Transform Workers)

For worker classes that compute individual slices, construct a local
`FilterMessenger` inside `operator()` or `convert()` and capture the
loop-constant values (array name, total slices) in the formatter lambda:

```cpp
// From RotateImageGeometryWithNearestNeighbor
void convert() const
{
  FilterMessenger filterMessenger(m_MessageHandler);
  const auto arrayName = m_SourceArray->getName();
  const int64 totalSlices = m_Params.outputDims[2];

  filterMessenger.setThrottledFormatter(
      [arrayName, totalSlices](usize slice)
      {
        return fmt::format("{}: Interpolating values for slice '{}/{}'",
                           arrayName, slice, totalSlices);
      });

  for(int64 k = 0; k < m_Params.outputDims[2]; k++)
  {
    filterMessenger.sendThrottledMessage(static_cast<usize>(k));
    // ... process slice ...
  }
  // Use the direct handler for guaranteed-delivery messages
  m_MessageHandler(fmt::format("{}: Transform Ending", m_SourceArray->getName()));
}
```

## Progress Tracking (Multi-Threaded)

Use `createProgressHelper()` when multiple worker threads need to contribute
to a shared progress counter. This is the recommended pattern for
`ParallelTaskAlgorithm` and `ParallelDataAlgorithm` workloads.

`createProgressHelper` registers a `ProgressChannel` with the
`FilterMessenger`'s background timer thread — no second thread is created.
Each `ProgressWorker` handle shares the same atomic counter and can safely
call `incrementProgress()` from any thread.

### With ParallelTaskAlgorithm (Task-Parallel)

Each task gets its own `ProgressWorker` handle via `createWorkerHandle()`.
All workers atomically contribute to one shared counter:

```cpp
// From ResampleImageGeom -- multiple arrays processed in parallel
FilterMessenger filterMessenger(m_MessageHandler);

usize totalArrays = srcCellDataAM.getSize();
usize numVoxels = destImageGeom.getNumberOfCells();
usize totalProgress = numVoxels * totalArrays;

ProgressHelper progressHelper = filterMessenger.createProgressHelper(
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
      progressHelper.createWorkerHandle(),  // copyable
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

For range-split parallelism, pass a `ProgressWorker` into the range functor.
Because `ProgressWorker` is **copyable**, TBB can copy the functor freely:

```cpp
FilterMessenger filterMessenger(m_MessageHandler);
usize totalVertices = destGeometry.getNumberOfVertices();

ProgressHelper progressHelper = filterMessenger.createProgressHelper(
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

## Member Variable Pattern

For algorithm classes where the `FilterMessenger` must outlive a single
function (e.g. `SegmentFeatures`, `AlignSections`), store it as a member:

```cpp
// In the header:
#include "simplnx/Filter/FilterMessenger.hpp"

class MyAlgorithm
{
public:
  MyAlgorithm(DataStructure& ds, const std::atomic_bool& cancel,
              const IFilter::MessageHandler& msgHandler)
  : m_DataStructure(ds)
  , m_ShouldCancel(cancel)
  , m_FilterMessenger(msgHandler)  // construct from handler
  {}

protected:
  FilterMessenger m_FilterMessenger;
  // ...
};

// In execute():
m_FilterMessenger.sendInfo("Starting segmentation...");
m_FilterMessenger.setThrottledFormatter([total](usize i) {
  return fmt::format("{:.2f}% complete", CalculatePercentComplete(i, total));
});
for(usize i = 0; i < total; i++)
{
  m_FilterMessenger.sendThrottledMessage(i);
  // ...
}
```

## Choosing the Right Pattern

| Scenario | Mechanism | Hot-path cost |
|----------|-----------|---------------|
| Infrequent messages (start/end/phase) | `sendInfo()` / `sendWarning()` etc. | N/A (not in hot loop) |
| Single-threaded tight loop | `setThrottledFormatter()` + `sendThrottledMessage()` | ~10ns (atomic store) |
| Multi-threaded, shared counter | `createProgressHelper()` + `ProgressWorker` | ~10ns (atomic fetch_add) |
| Per-task descriptive string (image transforms) | Local `FilterMessenger` in `operator()` | ~10ns (atomic store) |

**Key design principle**: `sendThrottledMessage` is for single-threaded (or
single-owner) use where one thread stores the value and the background thread
sends. `ProgressHelper`/`ProgressWorker` is for multi-threaded use where
many threads atomically increment a shared counter.

## Utility: CalculatePercentComplete

A convenience template for computing percentage values, available from
`MessageHelper.hpp` (transitively included via `FilterMessenger.hpp`):

```cpp
// Returns float32 by default
float32 pct = CalculatePercentComplete(current, total);

// Or specify a different return type
int32 pctInt = CalculatePercentComplete<int32>(current, total);
```

## API Reference

### FilterMessenger

| Method | Description |
|--------|-------------|
| `FilterMessenger(const MessageHandler& handler)` | Constructor — stores reference to handler |
| `void sendInfo(std::string message)` | Synchronous Info send (guaranteed delivery) |
| `void sendDebug(std::string message)` | Synchronous Debug send |
| `void sendWarning(std::string message)` | Synchronous Warning send |
| `void sendError(std::string message)` | Synchronous Error send |
| `void sendProgress(std::string message, int32 progress = 0)` | Synchronous Progress send |
| `void setThrottledFormatter(formatter, interval = 1000ms)` | Set formatter for `sendThrottledMessage` |
| `void sendThrottledMessage(usize current)` | Store latest loop index (hot path, ~10ns) |
| `ProgressHelper createProgressHelper(max, formatter, interval = 1000ms)` | Create multi-threaded progress helper |

- Move-only (non-copyable)
- Background thread starts lazily on first `sendThrottledMessage` or `createProgressHelper` call
- On destruction: joins the timer thread and calls `finalFlush()` on all channels

### ProgressHelper

| Method | Description |
|--------|-------------|
| `ProgressWorker createWorkerHandle()` | Create a per-thread worker handle |
| `void resetProgress()` | Reset counter to zero (no active workers) |

- Move-only (non-copyable)
- `finalFlush()` is called automatically on destruction

### ProgressWorker

| Method | Description |
|--------|-------------|
| `void incrementProgress(usize amount = 1)` | Atomic increment (hot path, ~10ns) |

- **Copyable** — shares the underlying channel via `shared_ptr`; safe to copy into TBB/parallel-for functors
- Mark as `mutable` when stored in a class with a `const operator()`

## Migration from MessageHelper

The `MessageHelper` class has been replaced by `FilterMessenger`. The
`MessageDispatcher` singleton is gone — each `FilterMessenger` owns its own
timer thread.

### Synchronous send

```cpp
// OLD
MessageHelper messageHelper(m_MessageHandler);
messageHelper.sendMessage("Starting Phase 1");

// NEW
FilterMessenger filterMessenger(m_MessageHandler);
filterMessenger.sendInfo("Starting Phase 1");
```

### Single-argument throttled loop

```cpp
// OLD
auto messenger = messageHelper.createThrottledMessenger(
    [total](usize i) { return fmt::format("{:.2f}%", CalculatePercentComplete(i, total)); });
for(usize i = 0; i < total; i++) messenger.sendMessage(i);

// NEW
filterMessenger.setThrottledFormatter(
    [total](usize i) { return fmt::format("{:.2f}%", CalculatePercentComplete(i, total)); });
for(usize i = 0; i < total; i++) filterMessenger.sendThrottledMessage(i);
```

### Multi-argument throttled loop

The multi-argument `ThrottledMessenger<Args...>` is removed. Refactor to a
single `usize` primary value by capturing secondary values in the closure, or
by simplifying the message to show only percentage:

```cpp
// OLD (two args)
auto messenger = messageHelper.createThrottledMessenger(
    [](usize iter, usize count) { return fmt::format("Iter {}: {} remaining", iter, count); });
messenger.sendMessage(iter, remainingCount);

// NEW — simplify to percentage, or capture secondary if it's loop-constant
filterMessenger.setThrottledFormatter(
    [maxIterations](usize iter)
    {
      return fmt::format("{:.2f}% Complete", CalculatePercentComplete(iter, maxIterations));
    });
filterMessenger.sendThrottledMessage(iter);
```

### ProgressHelper (unchanged user API)

```cpp
// OLD
MessageHelper messageHelper(m_MessageHandler);
auto progressHelper = messageHelper.createProgressHelper(total, formatter);
auto worker = progressHelper.createWorkerHandle();

// NEW
FilterMessenger filterMessenger(m_MessageHandler);
auto progressHelper = filterMessenger.createProgressHelper(total, formatter);
auto worker = progressHelper.createWorkerHandle();
// worker.incrementProgress(1) — unchanged
```

### Member variable

```cpp
// OLD header:  MessageHelper m_MessageHelper;
// NEW header:  FilterMessenger m_FilterMessenger;

// OLD constructor: , m_MessageHelper(mesgHandler)
// NEW constructor: , m_FilterMessenger(mesgHandler)
```
