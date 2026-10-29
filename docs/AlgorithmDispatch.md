# Algorithm Dispatch for Out-of-Core Optimization

## Overview

Some filter algorithms that perform well on in-memory data become extremely slow when data is stored in disk-backed chunks (out-of-core / OOC mode). This document explains why this happens, when a filter needs separate algorithm implementations, and how to use the `DispatchAlgorithm` utility to select between them at runtime.

## Why Out-of-Core Needs Different Algorithms

### In-Core Storage (DataStore)

In-core arrays store data in a single contiguous memory buffer. Any element can be accessed in O(1) time via pointer arithmetic. Algorithms that use random access patterns (BFS flood fill, graph traversal, etc.) work efficiently because every `operator[]` call is a simple memory read.

### Out-of-Core Storage

Out-of-core arrays keep their data on disk in compressed chunks and hold a bounded number of decoded chunks in memory. When code reads or writes one element whose chunk is not in memory, the store loads and decodes that whole chunk, and it may first have to evict another chunk, writing it back when it was changed.

This means algorithms with random access patterns cause **chunk thrashing**: each random jump may load a new chunk and evict another, turning O(1) memory reads into O(disk) operations. A BFS flood fill that visits neighbors across chunk boundaries can trigger thousands of chunk load/evict cycles, making a sub-second algorithm take 10+ minutes.

### The Solution: Sequential Bulk Transfers

Algorithms designed for OOC data read and write contiguous ranges in order, through the bulk transfers of `AbstractDataStore` (`copyIntoBuffer()`, `copyFromBuffer()`, `readExtent()`), and keep only a bounded window in local buffers. A common form reads one XY slice at a time:

```cpp
std::vector<int32> slice(dimX * dimY);
for(usize z = 0; z < dimZ; z++)
{
  Result<> readResult = featureIdsStore.copyIntoBuffer(z * slice.size(), nonstd::span<int32>(slice.data(), slice.size()));
  if(readResult.invalid())
  {
    return readResult;
  }
  // ... process the slice from the local buffer ...
}
```

This pattern gives sequential disk access and a bounded working set. However, such algorithms often require different data structures (e.g., union-find instead of BFS queues) and may use more auxiliary memory.

## When to Use Algorithm Dispatch

Use the dispatch pattern when:

- The in-core algorithm uses random access (BFS, graph traversal, hash-map lookups by voxel index)
- The OOC-optimized algorithm uses fundamentally different data structures or traversal order
- The two approaches have different memory/performance trade-offs that make one unsuitable for the other's use case

Do **not** use dispatch when:

- The algorithm already uses sequential access patterns (a single algorithm with chunk-aware loops suffices)
- The only difference is minor (e.g., reading in larger blocks) rather than a fundamentally different approach
- The data is small enough that OOC overhead is negligible

## The DispatchAlgorithm Utility

**Header:** `simplnx/Utilities/AlgorithmDispatch.hpp`

### IsOutOfCore

```cpp
bool IsOutOfCore(const IDataArray& array);
bool IsOutOfCore(const IArray& array);
```

`IDataArray` uses its `IDataStore::StoreType`; `INeighborList` uses its
storage-neutral `IListStore::isOutOfCore()` query. Other `IArray` types, such
as `StringArray`, are conservatively in-memory until they expose a residency
contract.

### AnyOutOfCore

```cpp
bool AnyOutOfCore(std::initializer_list<const IDataArray*> arrays);
bool AnyOutOfCore(const AlgorithmArrayTargets& targets);
```

Returns `true` if **any** of the given arrays uses OOC storage. Null pointers in the list are skipped. Use this when a filter operates on multiple input/output arrays and any one of them being OOC should trigger the out-of-core algorithm path.

The original `IDataArray` initializer-list overload is retained for source
compatibility, including named `std::initializer_list<const IDataArray*>`
variables and empty `{}` calls. `AlgorithmArrayTargets` owns a copied pointer
list for NeighborList or other mixed targets, so it may safely be constructed
in one statement and passed to dispatch later. It does not own the pointed-to
arrays, which must remain alive for the call.

### DispatchAlgorithm

```cpp
template <typename InCoreAlgo, typename OocAlgo, typename... ArgsT>
Result<> DispatchAlgorithm(std::initializer_list<const IDataArray*> arrays, ArgsT&&... args);

template <typename InCoreAlgo, typename OocAlgo, typename... ArgsT>
Result<> DispatchAlgorithm(const AlgorithmArrayTargets& targets, ArgsT&&... args);
```

When the global `ForceInCoreAlgorithm()` flag is set, constructs `InCoreAlgo(args...)` and calls `operator()()`. Otherwise, when any array in `arrays` uses OOC storage or the global `ForceOocAlgorithm()` flag is set, constructs `OocAlgo(args...)` and calls `operator()()`; when neither is true, it constructs `InCoreAlgo(args...)`. See "ForceOocAlgorithm and ForceInCoreAlgorithm" below.

For existing data-array-only callers, continue using the familiar form:

```cpp
DispatchAlgorithm<Direct, Scanline>({featureIds, output}, args...);
```

For a modal NeighborList or a mixed target set, use the same braced form. It
selects the `AlgorithmArrayTargets` overload automatically:

```cpp
DispatchAlgorithm<Direct, Scanline>({featureIds, modalNeighborList}, args...);
```

**Requirements for algorithm classes:**

1. Both must be constructible from the same argument types (`ArgsT...`)
2. Both must provide `Result<> operator()()` as their execution entry point
3. Both must produce identical results for the same input data

## Example: IdentifySample

The IdentifySample filter identifies the largest contiguous region of "good" voxels in a 3D volume. It has two algorithm implementations:

### IdentifySampleBFS (In-Core)

Uses BFS flood fill with `std::vector<bool>` tracking arrays:
- Memory: 2 bits per voxel (checked + sample flags), plus an int64 queue that grows with the largest component
- Access pattern: Random (BFS visits neighbors in all 6 directions)
- In-core performance: Fast (random access is O(1) in memory)
- OOC performance: Extremely slow (random neighbor access thrashes chunk cache)

### IdentifySampleCCL (Out-of-Core)

Uses scanline Connected Component Labeling with union-find:
- Memory: two rolling slices of int64 labels plus the equivalence storage; a replay pass derives the labels again instead of storing one label per voxel
- Access pattern: Sequential (reads XY slices in Z order with `copyIntoBuffer()`, and checks only the x-1, y-1 and z-1 neighbors)
- In-core performance: Good, at the cost of an extra sequential read for the replay
- OOC performance: Fast (sequential slice reads instead of random neighbor access; the cost of the equivalence lookups depends on the input)

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
    IdentifySampleCCL.cpp     # Slice-sequential CCL implementation
    IdentifySampleCommon.hpp  # Shared code (VectorUnionFind, slice-by-slice functor)
```

## How to Add Algorithm Dispatch to a Filter

1. **Identify the bottleneck**: Profile the filter in OOC mode. If it is orders of magnitude slower than in-core, random access patterns are likely the cause.

2. **Design the OOC algorithm**: Replace random access with sequential bulk transfers. Common techniques:
   - Scanline CCL with union-find instead of BFS flood fill
   - Worklists sorted by position instead of global queues
   - Multi-pass algorithms where each pass reads the data in order

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

6. **Test both paths**: Use `UnitTest::AlgorithmTestScope` with Catch2 `GENERATE` to exercise both algorithm paths in every build configuration (see below).

## Testing Both Algorithm Paths

CI typically only builds the in-core configuration, so the OOC algorithm path would never be exercised without an explicit override. The `ForceOocAlgorithm()` and `ForceInCoreAlgorithm()` flags provide that override, and filter tests set them through `UnitTest::AlgorithmTestScope`.

### ForceOocAlgorithm and ForceInCoreAlgorithm

```cpp
bool& ForceOocAlgorithm();
bool& ForceInCoreAlgorithm();
```

Each returns a reference to a static bool that defaults to `false`. When `ForceOocAlgorithm()` is `true`, `DispatchAlgorithm` selects the OOC algorithm class even when no array uses chunked storage. `ForceInCoreAlgorithm()` takes precedence over both the storage check and `ForceOocAlgorithm()`.

### AlgorithmTestScope

`UnitTest::AlgorithmTestScope` (`simplnx/UnitTest/AlgorithmTestScope.hpp`) runs a test on one algorithm path with in-memory stores. `UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()` returns the paths that the build's `SIMPLNX_TEST_ALGORITHM_PATH` setting allows, so `GENERATE` runs the test once for each:

```cpp
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"

TEST_CASE("MyFilter", "[Plugin][MyFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // ... build the DataStructure and the Arguments ...

  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // ... compare the outputs with the expected values ...
}
```

`scope.executeFilter()` and `scope.execute()` set the force flags only for the call they wrap, so the filters that build the test data run normally. After the call, the scope requires that the selected algorithm path ran and the other path did not. The OOC algorithm class also works on in-core `DataStore` data, because the bulk transfers work on allocated in-memory stores as well as on out-of-core stores. A placeholder store (`EmptyDataStore`) holds no values and rejects them.

## Performance Expectations

- **In-core**: The dispatched in-core algorithm should match or exceed the original single-algorithm performance.
- **OOC**: The OOC algorithm should be dramatically faster than the original (often 100x+), though it may still be slower than in-core due to inherent disk I/O costs.
- **Small datasets**: On small datasets (< 1000 voxels), the dispatch overhead is negligible and both algorithms complete in milliseconds regardless.
