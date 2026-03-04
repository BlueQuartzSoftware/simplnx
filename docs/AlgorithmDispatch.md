# Algorithm Dispatch for Out-of-Core Optimization

## Overview

Some filter algorithms that perform well on in-memory data become extremely slow when data is stored in disk-backed chunks (out-of-core / OOC mode). This document explains why this happens, when a filter needs separate algorithm implementations, and how to use the `DispatchAlgorithm` utility to select between them at runtime.

## Why Out-of-Core Needs Different Algorithms

### In-Core Storage (DataStore)

In-core arrays store data in a single contiguous memory buffer. Any element can be accessed in O(1) time via pointer arithmetic. Algorithms that use random access patterns (BFS flood fill, graph traversal, etc.) work efficiently because every `operator[]` call is a simple memory read.

### Out-of-Core Storage (ZarrStore)

Out-of-core arrays partition data into compressed chunks stored on disk. A fixed-size FIFO cache (6 chunks) keeps recently accessed chunks in memory. When code accesses an element, the system:

1. Determines which chunk contains that element
2. If the chunk is cached, returns the value directly
3. If not cached, evicts the oldest chunk (writing it to disk if dirty) and loads the needed chunk from disk

This means algorithms with random access patterns cause **chunk thrashing**: each random jump may load a new chunk and evict another, turning O(1) memory reads into O(disk) operations. A BFS flood fill that visits neighbors across chunk boundaries can trigger thousands of chunk load/evict cycles, making a sub-second algorithm take 10+ minutes.

### The Solution: Chunk-Sequential Algorithms

Algorithms designed for OOC data process chunks sequentially using the chunk API:

```cpp
for(uint64 chunkIdx = 0; chunkIdx < store.getNumberOfChunks(); chunkIdx++)
{
    store.loadChunk(chunkIdx);
    auto lower = store.getChunkLowerBounds(chunkIdx);
    auto upper = store.getChunkUpperBounds(chunkIdx);
    for(z = lower[0]; z <= upper[0]; z++)
        for(y = lower[1]; y <= upper[1]; y++)
            for(x = lower[2]; x <= upper[2]; x++) { /* process voxel */ }
}
```

This pattern ensures sequential disk access and keeps the working set within the cache. However, chunk-sequential algorithms often require different data structures (e.g., union-find instead of BFS queues) and may use more auxiliary memory.

## When to Use Algorithm Dispatch

Use the dispatch pattern when:

- The in-core algorithm uses random access (BFS, graph traversal, hash-map lookups by voxel index)
- The OOC-optimized algorithm uses fundamentally different data structures or traversal order
- The two approaches have different memory/performance trade-offs that make one unsuitable for the other's use case

Do **not** use dispatch when:

- The algorithm already uses sequential access patterns (a single algorithm with chunk-aware loops suffices)
- The only difference is minor (e.g., adding `loadChunk()` calls) rather than a fundamentally different approach
- The data is small enough that OOC overhead is negligible

## The DispatchAlgorithm Utility

**Header:** `simplnx/Utilities/AlgorithmDispatch.hpp`

### IsOutOfCore

```cpp
bool IsOutOfCore(const IDataArray& array);
```

Returns `true` if the array's data store has a chunk shape (indicating ZarrStore or similar chunked storage). Returns `false` for in-memory DataStore.

### AnyOutOfCore

```cpp
bool AnyOutOfCore(std::initializer_list<const IDataArray*> arrays);
```

Returns `true` if **any** of the given arrays uses OOC storage. Null pointers in the list are skipped. Use this when a filter operates on multiple input/output arrays and any one of them being OOC should trigger the chunk-sequential algorithm path.

### DispatchAlgorithm

```cpp
template <typename InCoreAlgo, typename OocAlgo, typename... ArgsT>
Result<> DispatchAlgorithm(std::initializer_list<const IDataArray*> arrays, ArgsT&&... args);
```

Checks whether any array in `arrays` uses OOC storage, or if the global `ForceOocAlgorithm()` flag is set. If either condition is true, constructs `OocAlgo(args...)` and calls `operator()()`. Otherwise, constructs `InCoreAlgo(args...)` and calls `operator()()`.

**Requirements for algorithm classes:**

1. Both must be constructible from the same argument types (`ArgsT...`)
2. Both must provide `Result<> operator()()` as their execution entry point
3. Both must produce identical results for the same input data

## Example: IdentifySample

The IdentifySample filter identifies the largest contiguous region of "good" voxels in a 3D volume. It has two algorithm implementations:

### IdentifySampleBFS (In-Core)

Uses BFS flood fill with `std::vector<bool>` tracking arrays:
- Memory: 2 bits per voxel (checked + sample flags)
- Access pattern: Random (BFS visits neighbors in all 6 directions)
- In-core performance: Fast (random access is O(1) in memory)
- OOC performance: Extremely slow (random neighbor access thrashes chunk cache)

### IdentifySampleCCL (Out-of-Core)

Uses scanline Connected Component Labeling with union-find:
- Memory: 8 bytes per voxel (int64 label array) + union-find overhead
- Access pattern: Sequential (processes chunks in order, only checks backward neighbors)
- In-core performance: Good but uses more RAM than BFS
- OOC performance: Fast (sequential chunk access, no thrashing)

### Dispatcher Code

The `IdentifySample` algorithm class acts as a thin dispatcher:

```cpp
// IdentifySample.cpp
#include "IdentifySampleBFS.hpp"
#include "IdentifySampleCCL.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

Result<> IdentifySample::operator()()
{
    auto* inputData = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
    return DispatchAlgorithm<IdentifySampleBFS, IdentifySampleCCL>(
        {inputData}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
```

The filter (`IdentifySampleFilter`) is unchanged -- it creates an `IdentifySample` instance and calls `operator()()` as before. The dispatch is transparent.

### File Organization

```
Filters/Algorithms/
    IdentifySample.hpp        # InputValues struct + dispatcher class declaration
    IdentifySample.cpp        # Thin dispatcher using DispatchAlgorithm<BFS, CCL>
    IdentifySampleBFS.hpp     # BFS algorithm class declaration
    IdentifySampleBFS.cpp     # BFS flood-fill implementation
    IdentifySampleCCL.hpp     # CCL algorithm class declaration
    IdentifySampleCCL.cpp     # Chunk-sequential CCL implementation
    IdentifySampleCommon.hpp  # Shared code (VectorUnionFind, slice-by-slice functor)
```

## How to Add Algorithm Dispatch to a Filter

1. **Identify the bottleneck**: Profile the filter in OOC mode. If it is orders of magnitude slower than in-core, random access patterns are likely the cause.

2. **Design the OOC algorithm**: Replace random access with chunk-sequential processing. Common techniques:
   - Scanline CCL with union-find instead of BFS flood fill
   - Worklists sorted by chunk instead of global queues
   - Multi-pass algorithms where each pass processes chunks sequentially

3. **Create separate algorithm classes**: Both must share the same `InputValues` struct and constructor signature. Follow the existing pattern:
   ```
   FilterNameBFS.hpp/cpp     # In-core algorithm
   FilterNameCCL.hpp/cpp     # OOC algorithm (or other descriptive suffix)
   FilterName.hpp/cpp        # Dispatcher
   FilterNameCommon.hpp      # Shared code (if any)
   ```

4. **Make the dispatcher**: In the original algorithm's `operator()()`, use `DispatchAlgorithm`. Pass all input and output arrays so the dispatch triggers OOC mode if any of them are chunked:
   ```cpp
   return DispatchAlgorithm<FilterNameBFS, FilterNameCCL>(
       {inputArrayA, inputArrayB, outputArray},
       dataStructure, messageHandler, shouldCancel, inputValues);
   ```

5. **Register new files**: Add the new algorithm names to the plugin's `AlgorithmList` in `CMakeLists.txt`.

6. **Test both paths**: Use `ForceOocAlgorithmGuard` with Catch2 `GENERATE` to exercise both algorithm paths in every build configuration (see below).

## Testing Both Algorithm Paths

CI typically only builds the in-core configuration, so the OOC algorithm path would never be exercised without an explicit override. The `ForceOocAlgorithm()` flag and `ForceOocAlgorithmGuard` RAII class solve this.

### ForceOocAlgorithm

```cpp
bool& ForceOocAlgorithm();
```

Returns a reference to a static bool. When set to `true`, `DispatchAlgorithm` always selects the OOC algorithm class regardless of whether the data arrays use chunked storage. Defaults to `false`.

### ForceOocAlgorithmGuard

```cpp
class ForceOocAlgorithmGuard
{
public:
  ForceOocAlgorithmGuard(bool force);
  ~ForceOocAlgorithmGuard();
  // non-copyable, non-movable
};
```

RAII guard that sets `ForceOocAlgorithm()` on construction and restores the previous value on destruction. Use with Catch2 `GENERATE` to run each test case twice — once with the in-core algorithm and once with the OOC algorithm:

```cpp
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

TEST_CASE("MyFilter", "[Plugin][MyFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  // ... test body runs identically for both algorithm paths ...
}
```

This ensures both code paths are tested in every build configuration (in-core and OOC). The OOC algorithm class works correctly on in-core `DataStore` data because the chunk API methods are no-ops for `DataStore` (1 chunk spanning the full volume).

## Performance Expectations

- **In-core**: The dispatched in-core algorithm should match or exceed the original single-algorithm performance.
- **OOC**: The OOC algorithm should be dramatically faster than the original (often 100x+), though it may still be slower than in-core due to inherent disk I/O costs.
- **Small datasets**: On small datasets (< 1000 voxels), the dispatch overhead is negligible and both algorithms complete in milliseconds regardless.
