# Threaded Throttled Messenger - Change Summary

## Problem

The existing `ThrottledMessenger::sendThrottledMessage()` called
`std::chrono::steady_clock::now()` on **every invocation** in tight voxel loops,
adding 50-200ns of pure overhead per call. For a 10M voxel loop, this meant
0.5-2.0 seconds of wasted time on clock syscalls alone. Additionally, the spdlog
async backend introduced unnecessary complexity with redundant throttle layers.

## Solution

A singleton background timer thread (`MessageDispatcher`) now handles all clock
checking and string formatting. The filter's hot path (`sendMessage()`) only
stores raw argument values -- no clock check, no `fmt::format()` call.

### Performance Improvement

| Operation | Old Cost | New Cost |
|-----------|----------|----------|
| Hot-path call (single `usize`) | ~50-200ns (clock syscall) | ~10ns (2 atomic stores, lock-free) |
| Hot-path call (multi-arg) | ~50-200ns (clock syscall) | ~15-25ns (mutex + atomic store) |
| String construction | Every call (if throttle passes) | Only on timer tick (~every 1s) |

## Architecture

```
Filter Thread (hot loop)              Background Timer Thread (100ms tick)
  |                                     |
  |  sendMessage(args...)               |
  |    -> storeArgs()  [~10-25ns]       |
  |    -> set atomic flag               |
  |                                     |  timerLoop() wakes
  |                                     |    -> iterate channels
  |                                     |    -> check atomic flag (fast bail)
  |                                     |    -> check interval elapsed
  |                                     |    -> fmt::format(args...)
  |                                     |    -> call MessageHandler
```

### Key Classes

| Class | Role |
|-------|------|
| `MessageHelper` | Top-level factory; creates messengers from `IFilter::MessageHandler` |
| `ThrottledMessenger<Args...>` | User-facing handle; `sendMessage()` stores args cheaply |
| `ThrottledChannel<Args...>` | Internal channel; mutex+tuple storage for general case |
| `ThrottledChannel<usize>` | Atomic specialization; lock-free for the most common case |
| `ProgressHelper` | Multi-threaded progress tracking factory |
| `ProgressWorker` | Per-thread handle with atomic `incrementProgress()` |
| `ProgressChannel` | Internal channel; `std::atomic<usize>` progress counter |
| `MessageDispatcher` | Singleton; background timer thread flushes channels every 100ms |
| `IThrottledChannel` | Type-erased interface for channel registration |
| `callable_traits<T>` | Template metaprogramming for formatter argument deduction |

## Files Changed

### Core Implementation (2 files, rewritten)
- `src/simplnx/Utilities/MessageHelper.hpp` -- All public API classes
- `src/simplnx/Utilities/MessageHelper.cpp` -- MessageDispatcher singleton + pimpl

### New Files (2 files)
- `test/MessageHelperTest.cpp` -- 10 Catch2 unit tests
- `docs/SendingFilterUpdates.md` -- Developer documentation

### Build System (3 files)
- `CMakeLists.txt` -- Removed `find_package(spdlog)` and link dependency
- `vcpkg.json` -- Removed spdlog from dependencies
- `test/CMakeLists.txt` -- Added `MessageHelperTest.cpp`

### Migrated Callers (37 files)

#### Core Utilities (7 files)
- `src/simplnx/Utilities/SegmentFeatures.hpp`
- `src/simplnx/Utilities/SegmentFeatures.cpp`
- `src/simplnx/Utilities/SampleSurfaceMesh.hpp`
- `src/simplnx/Utilities/SampleSurfaceMesh.cpp`
- `src/simplnx/Utilities/OStreamUtilities.cpp`
- `src/simplnx/Utilities/AlignSections.hpp`
- `src/simplnx/Utilities/AlignSections.cpp`

#### SimplnxCore Filter Algorithms (17 files)
- `Filters/Algorithms/FillBadData.cpp`
- `Filters/Algorithms/ErodeDilateBadData.cpp`
- `Filters/Algorithms/RequireMinimumSizeFeatures.cpp`
- `Filters/Algorithms/ComputeNeighborhoods.hpp`
- `Filters/Algorithms/ComputeNeighborhoods.cpp`
- `Filters/Algorithms/ComputeArrayStatistics.cpp`
- `Filters/Algorithms/ComputeVertexToTriangleDistances.cpp`
- `Filters/Algorithms/CalculateTriangleGroupCurvatures.hpp`
- `Filters/Algorithms/CalculateTriangleGroupCurvatures.cpp`
- `Filters/Algorithms/RemoveFlaggedFeatures.cpp`
- `Filters/Algorithms/DBSCAN.cpp`
- `Filters/Algorithms/ReadStlFile.cpp`
- `Filters/Algorithms/FeatureFaceCurvature.cpp`
- `Filters/Algorithms/ComputeNeighborListStatistics.cpp`
- `Filters/Algorithms/ComputeArrayHistogramByFeature.cpp`
- `Filters/Algorithms/AlignSectionsFeatureCentroid.cpp`
- `Filters/Algorithms/WriteLAMMPSFile.cpp`

#### SimplnxCore Filter Files (2 files)
- `Filters/IterativeClosestPointFilter.cpp`
- `Filters/ComputeFeatureNeighborsFilter.cpp`

#### OrientationAnalysis Plugin (6 files)
- `Filters/Algorithms/ComputeGBCD.cpp`
- `Filters/Algorithms/RotateEulerRefFrame.cpp`
- `Filters/Algorithms/NeighborOrientationCorrelation.cpp`
- `Filters/Algorithms/BadDataNeighborOrientationCheck.cpp`
- `Filters/Algorithms/ComputeKernelAvgMisorientations.cpp`
- `Filters/Algorithms/AlignSectionsMisorientation.cpp`

#### SimplnxReview Plugin (3 files, separate repository)
- `Filters/Algorithms/GroupMicroTextureRegions.cpp`
- `Filters/Algorithms/ComputeGroupingDensity.cpp`
- `Filters/Algorithms/MergeColonies.cpp`

## What Was Removed

- `spdlog` dependency (library, includes, vcpkg entry, CMake linkage)
- `Messenger` class (hpp + cpp) and `Messenger::Impl`
- `MessageHandlerSink` (spdlog custom sink)
- `ThrottleSink` (spdlog rate-limiting sink)
- `ThreadPool` singleton (spdlog async thread pool)
- `ThrottledMessageFunctor` concept
- `ProgressMessageFunctor` concept
- `ProgressMessageData` struct
- Old non-templated `ThrottledMessenger` class
- `ProgressMessenger` class
- `ProgressMessageHelper` class

## What Was Retained

- `IFilter::Message`, `IFilter::ProgressMessage`, `IFilter::MessageHandler` -- unchanged
- `CalculatePercentComplete<T>()` -- retained as-is

## Migration Pattern

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

## Testing

- 10 new MessageHelper unit tests: all pass
- 58 tests for migrated filter files: 100% pass
- Full test suite: 936/1071 pass (87%), all 135 failures are pre-existing
- Filter cancellation: verified compatible, zero coupling to `shouldCancel`
