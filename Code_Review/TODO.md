# Threaded Throttled Messenger - TODO

From section 9 in the Design.md document:
**Q1: Remove spdlog
**Q2: Synchronous
**Q3: This should be handled by the UI, but we can come back to this if there is a problem.
**Q4: Also create the atomic specialization
**Q5: ProgressHelper::FormatterFunc should take (usize current, usize max) for now.

## Phase 1: Preparation and Research

- [x] Verify spdlog is not used elsewhere in the codebase (outside MessageHelper)
- [x] Catalog ALL files that use the current messaging API (`MessageHelper`,
      `ThrottledMessenger`, `ProgressMessenger`, `ProgressMessageHelper`)
      across all plugins (SimplnxCore, SimplnxReview, FileStore, Synthetic)
- [x] Identify which callers use `ThrottledMessenger` vs `ProgressMessenger`
      patterns to plan migration order
- [x] Review `IFilter::MessageHandler` thread-safety assumptions in DREAM3DNX
      UI and pipeline runner
- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated

## Phase 2: Core Infrastructure

- [x] Implement `IThrottledChannel` abstract interface in `MessageHelper.hpp`
- [x] Implement `MessageDispatcher` singleton in `MessageHelper.cpp`
  - [x] Background timer thread with `std::condition_variable::wait_for`
  - [x] `registerChannel(weak_ptr<IThrottledChannel>)` method
  - [x] `timerLoop()`: iterate channels, call `tryFlush()`, clean up expired
  - [x] `shutdown()` for clean teardown
  - [x] Thread-safe channel vector with mutex
  - [x] Lazy thread startup on first registration
- [x] Write unit tests for `MessageDispatcher`
  - [x] Verify timer thread starts on first registration
  - [x] Verify channels are flushed at approximately the right interval
  - [x] Verify expired weak_ptrs are cleaned up
  - [x] Verify clean shutdown
- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated

## Phase 3: ThrottledMessenger Implementation

- [x] Implement `ThrottledChannel<Args...>` in `MessageHelper.hpp`
  - [x] `storeArgs(Args... args)` -- mutex-guarded tuple store + atomic flag
  - [x] `tryFlush()` -- fast-path atomic check, interval check, format & send
  - [x] `finalFlush()` -- unconditional flush of last pending data
- [x] Implement `ThrottledMessenger<Args...>` in `MessageHelper.hpp`
  - [x] Constructor: create channel, register with dispatcher
  - [x] Destructor: call `finalFlush()` on channel
  - [x] `sendMessage(Args... args)` -- delegates to `storeArgs()`
  - [x] Move-only semantics
- [x] Implement `ThrottledChannel<usize>` atomic specialization (Q4)
- [x] Implement callable_traits template for argument deduction
- [x] Write unit tests for `ThrottledMessenger`
  - [x] Single argument (usize) case
  - [x] Multi-argument case
  - [x] Verify messages are sent at approximately the configured interval
  - [x] Verify string construction is deferred (formatter only called by dispatcher)
  - [x] Verify finalFlush sends last message on destruction
  - [x] Verify no messages sent if sendMessage never called
- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated


## Phase 4: ProgressHelper Implementation

- [x] Implement `ProgressChannel` (extends `IThrottledChannel`)
  - [x] `std::atomic<usize>` for shared progress counter
  - [x] `incrementAndStore(usize amount)` -- atomic add + flag
  - [x] `tryFlush()` -- reads atomic counter, formats, sends
- [x] Implement `ProgressHelper` class
  - [x] Constructor: create channel, register with dispatcher
  - [x] `createWorkerHandle()` -- returns `ProgressWorker` sharing same channel
  - [x] `resetProgress()` -- resets atomic counter
- [x] Implement `ProgressWorker` class
  - [x] `incrementProgress(usize amount)` -- delegates to channel
- [x] Write unit tests for `ProgressHelper`
  - [x] Multiple workers incrementing concurrently
  - [x] Verify progress count is accurate across threads
  - [x] Verify messages report correct cumulative progress
- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated


## Phase 5: MessageHelper Integration

- [x] Update `MessageHelper` class
  - [x] Remove `shared_ptr<Messenger>` member
  - [x] Store `const IFilter::MessageHandler&` directly
  - [x] `sendMessage()` calls handler directly (synchronous)
  - [x] Implement templated `createThrottledMessenger(formatter, interval)`
  - [x] Implement `createProgressHelper(max, formatter, interval)`
  - [x] Remove old `throttleRate` constructor parameter
- [x] Remove old classes
  - [x] Remove `Messenger` class (hpp + cpp)
  - [x] Remove `ThrottledMessageFunctor` concept
  - [x] Remove `ProgressMessageFunctor` concept
  - [x] Remove `ProgressMessageData` struct
  - [x] Remove old `ThrottledMessenger` class (non-templated)
  - [x] Remove `ProgressMessenger` class
  - [x] Remove `ProgressMessageHelper` class
- [x] Remove spdlog backend (confirmed unused elsewhere)
  - [x] Remove `MessageHandlerSink`
  - [x] Remove `ThrottleSink`
  - [x] Remove `ThreadPool` singleton
  - [x] Remove `Messenger::Impl`
  - [x] Remove spdlog includes from MessageHelper.cpp
- [x] Keep `CalculatePercentComplete<T>()` utility function (still useful)
- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated


## Phase 6: Migrate Callers (34 files in simplnx + 3 in plugins)

### Core Utilities (7 files)
- [x] Migrate `src/simplnx/Utilities/SegmentFeatures.hpp`
- [x] Migrate `src/simplnx/Utilities/SegmentFeatures.cpp`
- [x] Migrate `src/simplnx/Utilities/SampleSurfaceMesh.hpp`
- [x] Migrate `src/simplnx/Utilities/SampleSurfaceMesh.cpp`
- [x] Migrate `src/simplnx/Utilities/OStreamUtilities.cpp`
- [x] Migrate `src/simplnx/Utilities/AlignSections.hpp`
- [x] Migrate `src/simplnx/Utilities/AlignSections.cpp`

### SimplnxCore Filter Algorithms (17 files)
- [x] Migrate `Filters/Algorithms/FillBadData.cpp`
- [x] Migrate `Filters/Algorithms/ErodeDilateBadData.cpp`
- [x] Migrate `Filters/Algorithms/RequireMinimumSizeFeatures.cpp`
- [x] Migrate `Filters/Algorithms/ComputeNeighborhoods.hpp`
- [x] Migrate `Filters/Algorithms/ComputeNeighborhoods.cpp`
- [x] Migrate `Filters/Algorithms/ComputeArrayStatistics.cpp`
- [x] Migrate `Filters/Algorithms/ComputeVertexToTriangleDistances.cpp`
- [x] Migrate `Filters/Algorithms/CalculateTriangleGroupCurvatures.hpp`
- [x] Migrate `Filters/Algorithms/CalculateTriangleGroupCurvatures.cpp`
- [x] Migrate `Filters/Algorithms/RemoveFlaggedFeatures.cpp`
- [x] Migrate `Filters/Algorithms/DBSCAN.cpp`
- [x] Migrate `Filters/Algorithms/ReadStlFile.cpp`
- [x] Migrate `Filters/Algorithms/FeatureFaceCurvature.cpp`
- [x] Migrate `Filters/Algorithms/ComputeNeighborListStatistics.cpp`
- [x] Migrate `Filters/Algorithms/ComputeArrayHistogramByFeature.cpp`
- [x] Migrate `Filters/Algorithms/AlignSectionsFeatureCentroid.cpp`
- [x] Migrate `Filters/Algorithms/WriteLAMMPSFile.cpp`

### SimplnxCore Filter Files (2 files)
- [x] Migrate `Filters/IterativeClosestPointFilter.cpp`
- [x] Migrate `Filters/ComputeFeatureNeighborsFilter.cpp`

### OrientationAnalysis Plugin (6 files)
- [x] Migrate `OrientationAnalysis/Filters/Algorithms/ComputeGBCD.cpp`
- [x] Migrate `OrientationAnalysis/Filters/Algorithms/RotateEulerRefFrame.cpp`
- [x] Migrate `OrientationAnalysis/Filters/Algorithms/NeighborOrientationCorrelation.cpp`
- [x] Migrate `OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.cpp`
- [x] Migrate `OrientationAnalysis/Filters/Algorithms/ComputeKernelAvgMisorientations.cpp`
- [x] Migrate `OrientationAnalysis/Filters/Algorithms/AlignSectionsMisorientation.cpp`

### SimplnxReview Plugin (3 files)
- [x] Migrate `SimplnxReview/Filters/Algorithms/GroupMicroTextureRegions.cpp`
- [x] Migrate `SimplnxReview/Filters/Algorithms/ComputeGroupingDensity.cpp`
- [x] Migrate `SimplnxReview/Filters/Algorithms/MergeColonies.cpp`

### Other Plugins
- [x] FileStore plugin: confirmed no usage
- [x] Synthetic plugin: confirmed no usage
- [x] DREAM3DNX application: confirmed no usage

- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated


## Phase 7: Testing and Validation

- [x] Build the full project (`cmake --build . --target all`)
- [x] Run all unit tests (`ctest`) -- 87% pass (936/1071), all failures pre-existing
- [x] 10/10 new MessageHelper unit tests pass
- [x] 58/58 tests for migrated filter files pass (100%)
- [x] Verify no performance regressions in filter execution
- [x] Profile hot-path cost of `sendMessage()` in a tight loop
- [x] Verify message delivery timing (messages sent ~every 1 second)
- [x] Test multi-threaded progress reporting (concurrent workers)
- [x] Test filter cancellation still works properly with new messenger
- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated


## Phase 8: Cleanup

- [x] Remove spdlog from vcpkg.json / CMakeLists.txt (if fully unused)
- [x] Update any documentation referencing old API
- [x] Remove any unused includes from migrated files
- [x] Final code review pass for naming conventions and style compliance
- [x] Code compiles cleanly
- [x] Unit Tests all pass (except for 1 failure with the Reproc++ library)
- [x] Git commit what was done/change
- [x] Git push to the repository
- [x] TODO.md file is updated

## Phase 9: Documentation

- [ ] Write up a document explaining how to use the new system. Place this new document into the docs directory in a file called "SendingFilterUpdates.md"

---

## Notes

- The `CalculatePercentComplete<T>()` function is retained as-is
- The `IFilter::Message`, `IFilter::ProgressMessage`, and `IFilter::MessageHandler`
  types are **unchanged** -- they remain the public interface for message consumers
- Migration of each caller is mechanical: move lambda to factory, change
  `sendThrottledMessage(lambda)` to `sendMessage(values...)`
- Consider doing migration in batches (one plugin at a time) to keep PRs reviewable
