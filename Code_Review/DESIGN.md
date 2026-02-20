# Threaded Throttled Messenger - Design Document

## 1. Executive Summary

Redesign the filter messaging system (`MessageHelper.hpp/.cpp`) to eliminate the
`std::chrono::steady_clock::now()` bottleneck in tight loops. The new design uses
a **singleton background timer thread** that periodically checks for pending
messages, allowing the hot-path `sendMessage()` call to be reduced to a cheap
atomic/mutex-guarded argument store with **zero clock checks and zero string
construction** in the caller's loop.

---

## 2. Problem Statement

### Current Performance Bottleneck

`ThrottledMessenger::sendThrottledMessage()` calls `std::chrono::steady_clock::now()`
on **every invocation**, even when the message is discarded:

```cpp
// Called millions of times in tight voxel loops
void sendThrottledMessage(ThrottledMessageFunctor auto functor)
{
  auto now = std::chrono::steady_clock::now();   // ~50-200ns syscall
  m_LastTimeDiff = now - m_LastTime;
  if(m_LastTimeDiff >= m_Interval)
  {
    m_LastTime = now;
    m_Messenger->trySendMessage(functor());      // String construction only here
  }
}
```

- `steady_clock::now()` is a system call on most platforms: **50-200ns per call**
- In a loop processing 10M voxels, this adds **0.5-2.0 seconds** of pure overhead
- The lambda is constructed every iteration (though the string is deferred)

### Additional Issues

1. **Double throttling**: Both `ThrottledMessenger` (clock check) and `ThrottleSink`
   (spdlog sink-level rate limit) perform redundant time checks
2. **Per-worker messenger instances**: Each parallel worker creates its own
   `ThrottledMessenger`, each independently calling `steady_clock::now()`
3. **spdlog complexity**: The spdlog async infrastructure (thread pool, custom sinks)
   adds complexity that becomes unnecessary with a dedicated timer thread

---

## 3. Current Architecture

```
User Code (Filter Algorithm)
    |
    v
MessageHelper              -- Top-level factory, owns shared Messenger
    |
    +-- sendMessage()       -- Guaranteed delivery (sync -> spdlog async block)
    |
    +-- createThrottledMessenger()
    |       |
    |       v
    |   ThrottledMessenger  -- Per-instance clock check on EVERY call
    |       |
    |       +-- sendThrottledMessage(lambda)
    |               |
    |               +-- steady_clock::now()     *** BOTTLENECK ***
    |               +-- if elapsed >= interval:
    |                     lambda() -> string
    |                     Messenger::trySendMessage()
    |
    +-- createProgressMessageHelper()
            |
            v
        ProgressMessageHelper -- Factory for ProgressMessenger
            |
            v
        ProgressMessenger     -- Wraps ThrottledMessenger + atomic counter
            |
            v
        ThrottledMessenger    -- Same bottleneck

Backend (MessageHelper.cpp):
    Messenger::Impl
        +-- m_MandatoryLogger  (spdlog async, overflow: block)
        +-- m_ThrottledLogger  (spdlog async, overflow: drop-oldest)
        +-- ThreadPool singleton (shared spdlog::details::thread_pool)
        +-- ThrottleSink (second throttle layer in spdlog)
        +-- MessageHandlerSink (bridges spdlog -> IFilter::MessageHandler)
```

### Current Usage Pattern (FillBadData.cpp example)

```cpp
MessageHelper messageHelper(m_MessageHandler, std::chrono::milliseconds(1000));
auto throttledMessenger = messageHelper.createThrottledMessenger(
    std::chrono::milliseconds(1000));

while(count != 0)
{
  // ... work ...

  // Lambda constructed EVERY iteration, clock checked EVERY iteration
  throttledMessenger.sendThrottledMessage([iteration, count]() {
    return fmt::format("  Iteration {}: {} voxels remaining to fill",
                       iteration, count);
  });
}
```

---

## 4. Proposed Architecture

### 4.1 Overview

The core idea: **move the clock check and string construction off the hot path
entirely**. The caller's `sendMessage()` just stores raw argument values. A
singleton background thread wakes periodically, checks which channels have new
data, and handles the clock check, string formatting, and message dispatch.

```
User Code (Filter Algorithm)
    |
    v
MessageHelper                    -- Top-level factory (simplified)
    |
    +-- sendMessage()            -- Guaranteed, synchronous, calls MessageHandler directly
    |
    +-- createThrottledMessenger(formatter, interval)
    |       |
    |       v
    |   ThrottledMessenger<Args...>  -- Lightweight handle (user-facing)
    |       |
    |       +-- sendMessage(args...)  -- Just stores args + bumps counter
    |       |                            NO clock check, NO string construction
    |       |
    |       +-- owns shared_ptr<ThrottledChannel<Args...>>
    |               |
    |               +-- registered with MessageDispatcher (weak_ptr)
    |               +-- tryFlush() called by dispatcher thread
    |               +-- finalFlush() called on destruction
    |
    +-- createProgressHelper(max, formatter, interval)
            |
            v
        ProgressHelper               -- Multi-thread progress tracking
            |
            +-- createWorkerHandle() --> ProgressWorker
            |
            +-- owns shared_ptr<ProgressChannel>
                    |
                    +-- registered with MessageDispatcher

MessageDispatcher (Singleton)
    |
    +-- Single background std::thread
    +-- Wakes every k_TickInterval (default 100ms)
    +-- Iterates all registered channels
    +-- Calls tryFlush() on each channel with new data
    +-- Auto-cleans expired channels (weak_ptr)
```

### 4.2 MessageDispatcher (Singleton)

A process-wide singleton that owns a single background timer thread.

```cpp
class SIMPLNX_EXPORT MessageDispatcher
{
public:
  static MessageDispatcher& instance();

  // Register a channel. The dispatcher holds a weak_ptr,
  // so the channel auto-deregisters when destroyed.
  void registerChannel(std::weak_ptr<IThrottledChannel> channel);

  // Allow manual early shutdown (e.g., for testing)
  void shutdown();

private:
  MessageDispatcher();
  ~MessageDispatcher() noexcept;

  void timerLoop();

  std::thread m_TimerThread;
  std::atomic<bool> m_Running{true};
  std::mutex m_ChannelsMutex;
  std::condition_variable m_ShutdownCv;
  std::vector<std::weak_ptr<IThrottledChannel>> m_Channels;

  static constexpr auto k_TickInterval = std::chrono::milliseconds(100);
};
```

**Key design decisions:**
- **Tick interval of 100ms**: The timer wakes 10x/second regardless of channel
  intervals. Each channel decides in `tryFlush()` whether enough time has passed
  for its own interval. This adds at most 100ms of latency to progress messages,
  which is imperceptible.
- **weak_ptr registration**: Channels auto-deregister when the owning
  `ThrottledMessenger` handle is destroyed. No explicit deregistration needed.
- **condition_variable for shutdown**: The timer thread sleeps via
  `m_ShutdownCv.wait_for()`, allowing clean shutdown without busy-waiting.
- **Lazy startup**: The timer thread is spawned on first `registerChannel()` call
  and stopped when the singleton is destroyed.

### 4.3 IThrottledChannel (Type-Erased Interface)

```cpp
class IThrottledChannel
{
public:
  virtual ~IThrottledChannel() = default;

  // Called by the dispatcher thread. Returns true if a message was sent.
  virtual bool tryFlush() = 0;

  // Called on destruction to send the last pending message.
  virtual void finalFlush() = 0;
};
```

### 4.4 ThrottledChannel\<Args...\> (Internal, Templated)

The actual channel implementation, templated on the argument types.

```cpp
template <typename... Args>
class ThrottledChannel : public IThrottledChannel
{
public:
  using FormatterFunc = std::function<std::string(Args...)>;

  ThrottledChannel(FormatterFunc formatter,
                   const IFilter::MessageHandler& handler,
                   std::chrono::milliseconds interval)
  : m_Formatter(std::move(formatter))
  , m_Handler(handler)
  , m_Interval(interval)
  , m_LastSendTime(std::chrono::steady_clock::now())
  {
  }

  // Called by ThrottledMessenger::sendMessage() -- THE HOT PATH
  void storeArgs(Args... args)
  {
    {
      std::lock_guard lock(m_ArgsMutex);
      m_LatestArgs = std::make_tuple(args...);
    }
    m_HasNewData.store(true, std::memory_order_release);
  }

  // Called by dispatcher thread -- OFF the hot path
  bool tryFlush() override
  {
    // Fast path: no new data
    if(!m_HasNewData.load(std::memory_order_acquire))
    {
      return false;
    }

    // Check interval
    auto now = std::chrono::steady_clock::now();
    if(now - m_LastSendTime < m_Interval)
    {
      return false;
    }

    return doFlush(now);
  }

  void finalFlush() override
  {
    if(m_HasNewData.load(std::memory_order_acquire))
    {
      doFlush(std::chrono::steady_clock::now());
    }
  }

private:
  bool doFlush(std::chrono::steady_clock::time_point now)
  {
    m_LastSendTime = now;
    m_HasNewData.store(false, std::memory_order_relaxed);

    std::tuple<std::decay_t<Args>...> args;
    {
      std::lock_guard lock(m_ArgsMutex);
      args = m_LatestArgs;
    }

    auto message = std::apply(m_Formatter, args);
    m_Handler(message);
    return true;
  }

  FormatterFunc m_Formatter;
  const IFilter::MessageHandler& m_Handler;
  std::chrono::milliseconds m_Interval;
  std::chrono::steady_clock::time_point m_LastSendTime;

  std::mutex m_ArgsMutex;
  std::tuple<std::decay_t<Args>...> m_LatestArgs;
  std::atomic<bool> m_HasNewData{false};
};
```

**Performance characteristics of `storeArgs()` (the hot path):**
- One uncontended mutex lock/unlock: ~10-20ns
- One atomic store (release): ~5ns
- **Total: ~15-25ns** vs. old `steady_clock::now()`: **~50-200ns**
- **3-10x faster** than the current implementation
- No string construction, no lambda construction, no clock check

### 4.5 ThrottledMessenger\<Args...\> (User-Facing Handle)

A lightweight, movable/copyable handle that wraps a shared channel.

```cpp
template <typename... Args>
class ThrottledMessenger
{
public:
  using FormatterFunc = std::function<std::string(Args...)>;

  ThrottledMessenger(FormatterFunc formatter,
                     const IFilter::MessageHandler& handler,
                     std::chrono::milliseconds interval)
  : m_Channel(std::make_shared<ThrottledChannel<Args...>>(
        std::move(formatter), handler, interval))
  {
    MessageDispatcher::instance().registerChannel(m_Channel);
  }

  ~ThrottledMessenger()
  {
    if(m_Channel)
    {
      m_Channel->finalFlush();  // Send last pending message
    }
  }

  // Move-only (shared_ptr semantics would also work, but move-only is safer)
  ThrottledMessenger(ThrottledMessenger&&) noexcept = default;
  ThrottledMessenger& operator=(ThrottledMessenger&&) noexcept = default;
  ThrottledMessenger(const ThrottledMessenger&) = delete;
  ThrottledMessenger& operator=(const ThrottledMessenger&) = delete;

  // THE HOT PATH - called millions of times in tight loops
  void sendMessage(Args... args)
  {
    m_Channel->storeArgs(args...);
  }

private:
  std::shared_ptr<ThrottledChannel<Args...>> m_Channel;
};
```

### 4.6 ProgressHelper and ProgressWorker (Multi-Thread Progress)

For filters that use multiple worker threads contributing to a shared progress
counter (e.g., parallel data algorithms):

```cpp
class ProgressHelper
{
public:
  using FormatterFunc = std::function<std::string(usize, usize)>;

  ProgressHelper(FormatterFunc formatter,
                 const IFilter::MessageHandler& handler,
                 usize maxProgress,
                 std::chrono::milliseconds interval);

  // Each worker thread gets its own lightweight handle
  ProgressWorker createWorkerHandle();

  void resetProgress();

private:
  std::shared_ptr<ProgressChannel> m_Channel;
};

class ProgressWorker
{
public:
  // THE HOT PATH for parallel workers
  void incrementProgress(usize amount = 1)
  {
    m_Channel->incrementAndStore(amount);
  }

private:
  friend class ProgressHelper;
  std::shared_ptr<ProgressChannel> m_Channel;
};
```

`ProgressChannel` extends `IThrottledChannel` with an internal
`std::atomic<usize>` for thread-safe progress accumulation. Multiple
`ProgressWorker` handles share the same channel and atomic counter.

### 4.7 MessageHelper (Updated Factory)

```cpp
class MessageHelper
{
public:
  MessageHelper() = delete;

  MessageHelper(const IFilter::MessageHandler& messageHandler)
  : m_MessageHandler(messageHandler)
  {
  }

  ~MessageHelper() noexcept = default;

  MessageHelper(const MessageHelper&) = delete;
  MessageHelper(MessageHelper&&) noexcept = default;
  MessageHelper& operator=(const MessageHelper&) = delete;
  MessageHelper& operator=(MessageHelper&&) noexcept = default;

  // Guaranteed synchronous message delivery
  void sendMessage(std::string message)
  {
    m_MessageHandler(std::move(message));
  }

  // Create a throttled messenger with a formatter lambda
  // Args... are deduced from the formatter's parameter types
  template <typename Formatter>
  auto createThrottledMessenger(
      Formatter&& formatter,
      std::chrono::milliseconds interval = std::chrono::milliseconds(1000));

  // Create a progress helper for multi-threaded progress tracking
  ProgressHelper createProgressHelper(
      usize maxProgress,
      ProgressHelper::FormatterFunc formatter,
      std::chrono::milliseconds interval = std::chrono::milliseconds(1000));

private:
  const IFilter::MessageHandler& m_MessageHandler;
};
```

**Changes from current `MessageHelper`:**
- No longer owns a `shared_ptr<Messenger>` -- the `Messenger` class is removed
- `sendMessage()` is synchronous (directly calls `MessageHandler`)
- `createThrottledMessenger()` is templated, returns `ThrottledMessenger<Args...>`
- `createProgressHelper()` replaces `createProgressMessageHelper()`
- Constructor no longer takes `throttleRate` (interval is per-messenger)

### 4.8 Template Argument Deduction

To make `createThrottledMessenger()` work with auto-deduction from lambda
parameter types, we use callable traits:

```cpp
// Extracts parameter types from a callable's operator()
template <typename T>
struct callable_traits;

template <typename R, typename C, typename... Args>
struct callable_traits<R(C::*)(Args...) const>
{
  using args_tuple = std::tuple<std::decay_t<Args>...>;
};

template <typename T>
struct callable_traits : callable_traits<decltype(&T::operator())> {};
```

Then `createThrottledMessenger()` uses this to deduce `Args...`:

```cpp
template <typename Formatter>
auto MessageHelper::createThrottledMessenger(Formatter&& formatter,
    std::chrono::milliseconds interval)
{
  using traits = callable_traits<std::decay_t<Formatter>>;
  return createThrottledMessengerFromTuple(
      std::forward<Formatter>(formatter), interval,
      typename traits::args_tuple{});
}

template <typename Formatter, typename... Args>
ThrottledMessenger<Args...> MessageHelper::createThrottledMessengerFromTuple(
    Formatter&& formatter, std::chrono::milliseconds interval,
    std::tuple<Args...>)
{
  return ThrottledMessenger<Args...>(
      std::forward<Formatter>(formatter), m_MessageHandler, interval);
}
```

This allows the user to write:
```cpp
auto messenger = messageHelper.createThrottledMessenger(
    [total](usize current) { return fmt::format("{}%", current); });
// Deduces: ThrottledMessenger<usize>
```

---

## 5. API Examples

### 5.1 Simple Progress Loop (Most Common)

```cpp
// OLD API:
MessageHelper messageHelper(m_MessageHandler, std::chrono::milliseconds(1000));
auto throttledMessenger = messageHelper.createThrottledMessenger(
    std::chrono::milliseconds(1000));
for(usize i = 0; i < totalVoxels; i++)
{
  // Lambda constructed every iteration, clock checked every iteration
  throttledMessenger.sendThrottledMessage([i, totalVoxels]() {
    return fmt::format("{:.2f}% complete",
        CalculatePercentComplete(i, totalVoxels));
  });
}

// NEW API:
MessageHelper messageHelper(m_MessageHandler);
auto throttledMessenger = messageHelper.createThrottledMessenger(
    [totalVoxels](usize current) {
      return fmt::format("{:.2f}% complete",
          CalculatePercentComplete(current, totalVoxels));
    },
    std::chrono::milliseconds(1000));
for(usize i = 0; i < totalVoxels; i++)
{
  throttledMessenger.sendMessage(i);  // Just stores 'i', ~15ns
}
```

### 5.2 Multi-Argument Messages

```cpp
auto messenger = messageHelper.createThrottledMessenger(
    [](usize iteration, usize voxelCount, float32 errorValue) {
      return fmt::format("Iteration {}: {} voxels, error = {:.6f}",
          iteration, voxelCount, errorValue);
    },
    std::chrono::milliseconds(1000));

for(usize iter = 0; iter < maxIterations; iter++)
{
  for(usize v = 0; v < totalVoxels; v++)
  {
    // ... compute error ...
    messenger.sendMessage(iter, v, currentError);
  }
}
```

### 5.3 Multi-Threaded Progress Tracking

```cpp
MessageHelper messageHelper(m_MessageHandler);
auto progressHelper = messageHelper.createProgressHelper(
    totalElements,
    [](usize current, usize max) {
      return fmt::format("{:.1f}% complete",
          CalculatePercentComplete(current, max));
    },
    std::chrono::milliseconds(1000));

// In parallel workers:
class MyWorker
{
  ProgressWorker m_ProgressWorker;

  void operator()(const Range& range) const
  {
    for(usize i = range.min(); i < range.max(); i++)
    {
      // ... work ...
      m_ProgressWorker.incrementProgress(1);  // Atomic add, ~5ns
    }
  }
};
```

### 5.4 Guaranteed Messages (Non-Throttled)

```cpp
MessageHelper messageHelper(m_MessageHandler);
messageHelper.sendMessage("Starting algorithm...");    // Synchronous
// ... run algorithm ...
messageHelper.sendMessage(fmt::format("Found {} features", count));  // Synchronous
```

### 5.5 FillBadData Migration Example

```cpp
// OLD (FillBadData.cpp):
MessageHelper messageHelper(m_MessageHandler, std::chrono::milliseconds(1000));
auto throttledMessenger = messageHelper.createThrottledMessenger(
    std::chrono::milliseconds(1000));
while(count != 0)
{
  // ... work ...
  throttledMessenger.sendThrottledMessage([iteration, count]() {
    return fmt::format("  Iteration {}: {} voxels remaining to fill",
                       iteration, count);
  });
}

// NEW (FillBadData.cpp):
MessageHelper messageHelper(m_MessageHandler);
auto throttledMessenger = messageHelper.createThrottledMessenger(
    [](usize iteration, usize count) {
      return fmt::format("  Iteration {}: {} voxels remaining to fill",
                         iteration, count);
    },
    std::chrono::milliseconds(1000));
while(count != 0)
{
  // ... work ...
  throttledMessenger.sendMessage(iteration, count);  // ~15ns
}
```

---

## 6. Performance Analysis

### Hot-Path Cost Comparison

| Operation | Old Design | New Design |
|-----------|-----------|------------|
| Clock check | ~50-200ns (`steady_clock::now()`) | 0ns (none) |
| Lambda construction | ~5-20ns (capture + create) | 0ns (none) |
| Argument storage | N/A | ~15-25ns (mutex + store) |
| **Total per call** | **~55-220ns** | **~15-25ns** |
| **10M iterations** | **~0.55-2.2 seconds** | **~0.15-0.25 seconds** |

### Background Thread Cost

- Timer thread wakes 10x/second (100ms interval)
- Each wake: iterate channels, check atomic flags, maybe format 1 string
- Negligible CPU cost: <0.01% of one core

### Memory Overhead

- MessageDispatcher singleton: ~200 bytes + thread stack
- Per ThrottledMessenger channel: ~200-400 bytes (formatter + args + synchronization)
- Per ProgressWorker: ~16 bytes (shared_ptr to channel)

---

## 7. spdlog Removal

The current implementation uses spdlog for:
1. **Async message dispatch** via `spdlog::async_logger` + `spdlog::details::thread_pool`
2. **Rate limiting** via custom `ThrottleSink`
3. **Message delivery** via custom `MessageHandlerSink`

In the new design, all three are handled by our own `MessageDispatcher`:
1. Async dispatch -> dispatcher timer thread
2. Rate limiting -> per-channel interval check in `tryFlush()`
3. Message delivery -> direct `MessageHandler` callback from dispatcher thread

**Recommendation:** Remove the spdlog dependency from the messaging system entirely.
This eliminates:
- `MessageHandlerSink` (custom spdlog sink)
- `ThrottleSink` (custom spdlog sink)
- `ThreadPool` singleton (spdlog thread pool wrapper)
- `Messenger::Impl` (spdlog logger management)
- The `Messenger` class itself

**Action item:** Verify that spdlog is not used elsewhere in the codebase before
removing the dependency entirely from vcpkg/CMakeLists.

---

## 8. Migration Guide

### API Changes Summary

| Old API | New API |
|---------|---------|
| `MessageHelper(handler, throttleRate)` | `MessageHelper(handler)` |
| `messageHelper.createThrottledMessenger(interval)` | `messageHelper.createThrottledMessenger(formatter, interval)` |
| `throttledMessenger.sendThrottledMessage(lambda)` | `throttledMessenger.sendMessage(args...)` |
| `messageHelper.createProgressMessageHelper()` | `messageHelper.createProgressHelper(max, formatter, interval)` |
| `progressHelper.createProgressMessenger(interval)` | `progressHelper.createWorkerHandle()` |
| `progressMessenger.sendProgressMessage(incr, lambda)` | `worker.incrementProgress(incr)` |
| `progressMessenger.sendProgressMessage(incr)` | `worker.incrementProgress(incr)` |

### Removed Classes/Concepts

- `Messenger` - replaced by direct `MessageHandler` calls + dispatcher
- `ThrottledMessageFunctor` concept - no longer needed (formatter set at construction)
- `ProgressMessageFunctor` concept - no longer needed
- `ProgressMessageData` struct - internalized in `ProgressChannel`
- `ProgressMessenger` - replaced by `ProgressWorker`
- `ProgressMessageHelper` - replaced by `ProgressHelper`

### Migration Steps for Each Caller

1. **Move the lambda/functor** from inside the loop to `createThrottledMessenger()`:
   - Values that change in the loop become **function parameters** (passed via `sendMessage`)
   - Values that are constant become **captured by value** in the formatter lambda

2. **Change `sendThrottledMessage(lambda)` to `sendMessage(values...)`**

3. **For ProgressMessenger users**: Switch to `ProgressHelper`/`ProgressWorker` API

4. **Remove `throttleRate` from `MessageHelper` constructor** (no longer needed)

### Files Requiring Migration

Full codebase search confirms **37 files** need migration across 4 repositories:

**Core utilities (7 files):**
- `SegmentFeatures.hpp/.cpp`, `SampleSurfaceMesh.hpp/.cpp`,
  `OStreamUtilities.cpp`, `AlignSections.hpp/.cpp`

**SimplnxCore (19 files):**
- `FillBadData`, `ErodeDilateBadData`, `RequireMinimumSizeFeatures`,
  `ComputeNeighborhoods` (hpp+cpp), `ComputeArrayStatistics`,
  `ComputeVertexToTriangleDistances`, `CalculateTriangleGroupCurvatures` (hpp+cpp),
  `RemoveFlaggedFeatures`, `DBSCAN`, `ReadStlFile`, `FeatureFaceCurvature`,
  `ComputeNeighborListStatistics`, `ComputeArrayHistogramByFeature`,
  `AlignSectionsFeatureCentroid`, `WriteLAMMPSFile`,
  `IterativeClosestPointFilter`, `ComputeFeatureNeighborsFilter`

**OrientationAnalysis (6 files):**
- `ComputeGBCD`, `RotateEulerRefFrame`, `NeighborOrientationCorrelation`,
  `BadDataNeighborOrientationCheck`, `ComputeKernelAvgMisorientations`,
  `AlignSectionsMisorientation`

**SimplnxReview (3 files):**
- `GroupMicroTextureRegions`, `ComputeGroupingDensity`, `MergeColonies`

**Confirmed no usage:** FileStore, Synthetic, DREAM3DNX

**spdlog dependency:** Confirmed used ONLY in `MessageHelper.cpp` -- safe to remove.

---

## 9. Open Questions

### Q1: spdlog Dependency

Should we remove spdlog entirely from the messaging system? It becomes redundant
with the dispatcher thread approach. Need to verify it's not used elsewhere.

**Recommendation:** Remove from messaging; verify no other usage in codebase.

### Q2: Synchronous vs. Asynchronous Guaranteed Messages

Should `MessageHelper::sendMessage()` be synchronous (direct callback) or
asynchronous (queued to dispatcher)?

- **Synchronous** (recommended): Simpler, guaranteed delivery, minimal latency.
  Guaranteed messages are rare (start/end of algorithm), so blocking the filter
  thread briefly is acceptable.
- **Asynchronous**: Would need a thread-safe queue in the dispatcher. Adds
  complexity for little benefit since these messages are infrequent.

### Q3: Thread Safety of MessageHandler Callback

In the new design, the `IFilter::MessageHandler` callback is invoked from the
dispatcher's background thread, not the calling filter's thread. Is this safe
with the current DREAM3DNX UI / pipeline runner?

**Risk:** If the callback updates UI elements, it may need to be marshaled to the
UI thread. The current spdlog-based design already calls the handler from a
background thread (spdlog's async thread pool), so this should already be handled.

### Q4: Atomic Specialization for Single Integral Arguments

For the very common case of `ThrottledMessenger<usize>`, should we provide a
template specialization that uses `std::atomic<usize>` instead of
`std::mutex + std::tuple<usize>`? This would reduce hot-path cost from ~15-25ns
to ~10ns.

**Recommendation:** Start with the general mutex approach. Profile. Specialize
only if the mutex overhead is measurable in practice.

### Q5: ProgressHelper Formatter Flexibility

Should `ProgressHelper::FormatterFunc` always take `(usize current, usize max)`,
or should it support arbitrary additional arguments?

**Recommendation:** Keep it fixed at `(usize current, usize max)` for simplicity.
If a filter needs additional context in progress messages, it can capture values
in the formatter lambda.

---

## 10. File Layout

After implementation, the messaging system files will be:

```
src/simplnx/Utilities/
  MessageHelper.hpp          -- MessageHelper, ThrottledMessenger<Args...>,
                                ProgressHelper, ProgressWorker, IThrottledChannel
  MessageHelper.cpp          -- MessageDispatcher singleton, ThrottledChannel<Args...>,
                                ProgressChannel implementations
```

The public API surface remains in a single header (`MessageHelper.hpp`), keeping
the include graph simple. Internal implementation details (dispatcher, channels)
are in the `.cpp` file or forward-declared.
