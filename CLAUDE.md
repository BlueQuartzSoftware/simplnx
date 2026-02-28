# Project Guidelines for Claude

## Project Overview

simplnx is a C++20 plugin-based data processing library for materials science and engineering. It provides filters, data structures, and pipeline infrastructure used by DREAM3D-NX.

## Directory Structure

- `src/simplnx/` - Core library (Common, Core, DataStructure, Filter, Parameters, Pipeline, Plugin, Utilities)
- `src/Plugins/` - Plugin modules (SimplnxCore, OrientationAnalysis, etc.)
- `src/nxrunner/` - CLI runner
- `test/` - Test files
- `cmake/` - CMake configuration

## Directories to Ignore

- `wrapping/` - Python wrapping code
- `scripts/` - Build/utility scripts
- `conda/` - Conda packaging

## Code Formatting

All C++ code must be clang-formatted before committing. The project has a `.clang-format` file at the repository root. After writing or modifying any `.hpp` or `.cpp` files, run:

```bash
clang-format -i <files...>
```

Do NOT run clang-format on non-C++ files (e.g., `.md`, `.cmake`, `.json`).

## Coding Standards

### C++ Style (from .clang-format)

- C++20 standard
- Allman brace style (braces on new lines for classes, control statements, enums, functions, namespaces, structs, before else)
- 200 column limit
- 2-space indentation, no tabs
- Pointer alignment left (`int* ptr` not `int *ptr`)
- No space before parentheses
- Sort includes alphabetically
- No short functions on single line
- Always break template declarations
- Constructor initializers break before comma

### Naming Conventions (from .clang-tidy)

- C++ header files: `.hpp` extension
- C++ source files: `.cpp` extension
- Namespaces: `lower_case`
- Classes: `CamelCase`
- Structs: `CamelCase`
- Class methods: `camelBack`
- Functions: `camelBack`
- Variables: `camelBack`
- Private members: `m_` prefix + `CamelCase` (e.g., `m_MemberVariable`)
- Global variables: `CamelCase`
- Global constants: `k_` prefix + `CamelCase` (e.g., `k_DefaultValue`)
- Local pointers: `camelBack` + `Ptr` suffix (e.g., `dataPtr`)
- Type aliases: `CamelCase` + `Type` suffix (e.g., `ValueType`)
- Macros: `UPPER_CASE`

### Descriptive Variable Naming

Use suffixes to make variable types and purposes immediately clear:

**Geometry variables use `Geom` suffix:**
- Correct: `const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(path);`
- Incorrect: `const auto& image = dataStructure.getDataRefAs<ImageGeom>(path);`

**DataStore references use `Ref` suffix:**
- Correct: `const auto& verticesRef = vertexGeom.getVertices()->getDataStoreRef();`
- Incorrect: `const auto& vertices = vertexGeom.getVertices()->getDataStoreRef();`

Examples:
```cpp
// Geometry variables
auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imagePath);
const auto& rectGridGeom = dataStructure.getDataRefAs<RectGridGeom>(rectPath);
const auto& edgeGeom = dataStructure.getDataRefAs<EdgeGeom>(edgePath);

// DataStore references
const auto& xBoundsRef = rectGridGeom.getXBounds()->getDataStoreRef();
const auto& yBoundsRef = rectGridGeom.getYBounds()->getDataStoreRef();
const auto& verticesRef = edgeGeom.getVertices()->getDataStoreRef();
```

### File Organization

- When creating a C++ based simplnx filter inside a plugin, the complete filter will have a `NameFilter.hpp` and `NameFilter.cpp` file, an `Algorithm/Name.hpp` and `Algorithm/Name.cpp`.
- Filter documentation files are created in Markdown and are in the `docs` subfolder inside the Plugins directory.
- Unit tests should be created in the `test` subfolder and use the Catch2 unit testing framework.

## Filter Implementation Guidelines

### Parameter Validation

- Selection parameters (GeometrySelectionParameter, ArraySelectionParameter, DataGroupSelectionParameter, etc.) automatically validate that the selected object exists in the DataStructure. Do NOT add null checks for these in preflightImpl() or executeImpl().
- Only add explicit existence checks for objects that are not validated by a selection parameter.

### DataStructure Access

- Use `getDataRefAs<T>()` to get a reference when you know the object exists (e.g., validated by a selection parameter).
- Use `getDataAs<T>()` to get a pointer only when you need to check if an object exists or when the object may not be present.
- **IMPORTANT**: In unit tests, always wrap `getDataRefAs<T>()` calls with `REQUIRE_NOTHROW()` to provide clear test failure messages if the object doesn't exist.

Example - Correct:
```cpp
// Parameter already validated this exists, use reference
const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pInputImageGeometryPathValue);
SizeVec3 dims = imageGeom.getDimensions();
```

Example - Incorrect:
```cpp
// Unnecessary null check - parameter already validated existence
const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(pInputImageGeometryPathValue);
if(imageGeomPtr == nullptr)
{
  return {MakeErrorResult<OutputActions>(-1000, "Could not find geometry")};
}
```

## Thread Safety

### DataArray and DataStore Classes Are NOT Thread-Safe

- `DataArray`, `DataStore`, and `AbstractDataStore` classes are **NOT thread-safe** for concurrent read or write access.
- The subscript operator (`operator[]`) and other access methods may have internal state or go through virtual function calls that are not safe for concurrent access, even when accessing different indices.
- Some `DataStore` subclasses use out-of-core implementations where data may not be resident in memory. Getting raw pointers to the underlying data is dangerous and should be avoided.

### Parallelization Guidelines

- When writing parallel algorithms using `ParallelDataAlgorithm`, be aware that passing `DataArray` or `DataStore` references to worker classes can cause random failures on different platforms.
- If parallel access to data arrays is required, consider:
  1. Disabling parallelization with `parallelAlgorithm.setParallelizationEnabled(false)` for correctness
  2. Using thread-local storage for intermediate results
  3. Structuring the algorithm to avoid concurrent access to the same DataArray
- Do NOT assume that writing to different indices of a DataArray from multiple threads is safe.

Example - Potentially Unsafe:
```cpp
// This pattern can cause random failures even when threads write to different indices
class MyParallelWorker
{
  DataArray<float32>& m_OutputArray;  // NOT thread-safe for concurrent access
  void operator()(const Range& range) const
  {
    for(usize i = range.min(); i < range.max(); i++)
    {
      m_OutputArray[i] = computeValue(i);  // May fail randomly
    }
  }
};
```

## Build & Test Protocol

This project maintains two separate builds from the same source: in-core and out-of-core. After modifying any source code, both must be built and tested to ensure identical results.

### Presets

| Purpose | Configure Preset | Build Preset | Test Preset |
|---------|-----------------|--------------|-------------|
| In-core Release | `simplnx-Rel` | `simplnx-Rel [BUILD]` | `simplnx-Rel-test` |
| Out-of-core Release | `simplnx-ooc-Rel` | `simplnx-ooc-Rel [BUILD]` | `simplnx-ooc-Rel-test` |

### After modifying code

1. Build both configurations (can run in parallel). **Always build ALL targets** (no `--target` flag) to ensure all plugin dependencies (FileStore, SimplnxCore, etc.) are built:
   ```bash
   cmake --build --preset "simplnx-Rel [BUILD]"
   cmake --build --preset "simplnx-ooc-Rel [BUILD]"
   ```

2. Run tests for the modified code on both:
   ```bash
   ctest --preset simplnx-Rel-test -R "FilterName"
   ctest --preset simplnx-ooc-Rel-test -R "FilterName"
   ```

3. Both must pass. If a test fails in one configuration but not the other, investigate before proceeding.

### First-time setup

Configure both builds before building:
```bash
cmake --preset simplnx-Rel
cmake --preset simplnx-ooc-Rel
```

### Key differences between configurations

- **In-core** (`simplnx-Rel`): Standard in-memory data storage. Plugin: `SimplnxReview`.
- **Out-of-core** (`simplnx-ooc-Rel`): `SIMPLNX_FORCE_OUT_OF_CORE_DATA=ON`, enables compressors, uses `FileStore` plugin instead of `SimplnxReview`. Disables visualization and charting.

### Build tips

- Use `-j` to control parallelism if builds are memory-constrained
- To rebuild a single target: `cmake --build --preset "simplnx-Rel [BUILD]" --target TargetName`
- To list available tests: `ctest --preset simplnx-Rel-test -N`
- Always use `ctest` to run unit tests, NOT the test binary directly. `ctest` handles test data extraction and cleanup automatically.

## Testing

- Unit tests use the Catch2 framework.
- Each `TEST_CASE` should include `UnitTest::CheckArraysInheritTupleDims(dataStructure);` near the end of the test to ensure all created data arrays have correct tuple dimensions inherited from their parent groups.
- Each test should call `UnitTest::LoadPlugins()` before executing filters.
- Use `DYNAMIC_SECTION()` for parameterized tests that generate multiple test cases.

### Printing debug statements in unit tests

```cpp
auto executeResult = filter.execute(dataStructure, args, nullptr,
    IFilter::MessageHandler{[](const IFilter::Message& message){ fmt::print("{}\n", message.message); }});
```

### Exemplar-Based Testing Pattern

Many tests use "exemplar" datasets — pre-generated golden reference data stored in `.dream3d` files. Exemplar datasets are generated by running pipeline files (`.d3dpipeline`) that configure and execute filters.

#### Workflow for Creating and Publishing Test Data

1. **Generate test data locally**: Create pipeline file with filter configurations and `WriteDREAM3DFilter` to save results
2. **Execute pipeline**: Run the pipeline to generate exemplar `.dream3d` file and any input data files
3. **Package as tar.gz**: Compress test data (no `6_6_` prefix — that was only for legacy DREAM3D data)
   ```bash
   tar -zvcf test_name.tar.gz test_directory/
   ```
4. **Compute SHA512 hash**:
   ```bash
   shasum -a 512 test_name.tar.gz
   ```
5. **Upload to GitHub**: Upload to the [DREAM3D data archive release](https://github.com/BlueQuartzSoftware/simplnx/releases/tag/Data_Archive)
6. **Update CMakeLists.txt**: Add `download_test_data()` call in `src/Plugins/[PluginName]/test/CMakeLists.txt`:
   ```cmake
   download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
                      ARCHIVE_NAME test_name.tar.gz
                      SHA512 <hash_from_step_4>)
   ```
7. **Test data auto-downloads**: When tests run, the sentinel mechanism automatically downloads and extracts the tar.gz to `unit_test::k_TestFilesDir`

#### Test Data Archive Naming and Versioning

- **Base naming**: Use descriptive names that match the test: `test_name.tar.gz`
- **Version suffixes**: When updating existing test data, append version numbers: `test_name_v2.tar.gz`, `test_name_v3.tar.gz`
- **When to version**: Original archive already exists, test requirements changed, cannot overwrite original because other code may depend on it
- **Check before creating**: Browse the [Data_Archive release](https://github.com/BlueQuartzSoftware/simplnx/releases/tag/Data_Archive) to see if your test data name already exists
- **Legacy prefixes**: `6_6_` and `6_5_` prefixes are for data from legacy DREAM3D/SIMPL versions — do NOT use for new DREAM3DNX test data

#### Test Code Pattern

```cpp
namespace
{
const std::string k_TestDataDirName = "test_name";
const fs::path k_TestDataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDataDirName;
const fs::path k_ExemplarFile = k_TestDataDir / "test_name.dream3d";
const fs::path k_InputImageFile = k_TestDataDir / "input_file.tif";
}
```

#### Comparing Test Results Against Exemplars

- **Load exemplar DataStructure**: Use `UnitTest::LoadDataStructure(exemplarFilePath)` to load the .dream3d file
- **ALWAYS use `REQUIRE_NOTHROW()` before `getDataRefAs<T>()`**: This applies to ALL `getDataRefAs` calls — both generated and exemplar data
- **Compare geometries**: Use `UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom)` — takes two pointers
- **Compare arrays**: Use `UnitTest::CompareDataArrays<T>(exemplarArray, generatedArray)` — type-specific template
- **Switch on data type** when comparing arrays to handle different types (uint8, uint16, uint32, float32, etc.)

Example pattern:
```cpp
// Load exemplar
DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

// Get geometries - ALWAYS wrap getDataRefAs with REQUIRE_NOTHROW
REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(generatedGeomPath));
const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(generatedGeomPath);
REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({exemplarGeomName})));
const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({exemplarGeomName}));

// Compare geometries (dimensions, origin, spacing) - pass pointers
UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

// Get arrays - ALWAYS wrap getDataRefAs with REQUIRE_NOTHROW
REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);

// Compare arrays based on type
switch(generatedArray.getDataType())
{
case DataType::uint8:
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);
  break;
case DataType::uint16:
  UnitTest::CompareDataArrays<uint16>(exemplarArray, generatedArray);
  break;
// ... etc
}
```

## Pipeline Files

- JSON format with `.d3dpipeline` extension
- Contains array of filter configurations with arguments
- Each filter has:
  - `args`: Dictionary of parameter keys and values
  - `comments`: Description of what the filter does
  - `filter`: Name and UUID
  - `isDisabled`: Boolean to skip filter execution
- Common pattern: Multiple filter configurations followed by WriteDREAM3DFilter to save all results to one `.dream3d` file
- Output geometry paths in pipeline must match exemplar names expected by tests

## Filter Algorithm Optimization Workflow

When optimizing a filter's algorithm for out-of-core performance, follow these steps in order:

### 1. Update the unit test with PreferencesSentinel and LoadPlugins

Add `UnitTest::LoadPlugins()` and `const UnitTest::PreferencesSentinel prefsSentinel(...)` to each TEST_CASE in the filter's test file, before the `TestFileSentinel` line. Follow the pattern used in `FillBadDataTest.cpp`:

```cpp
UnitTest::LoadPlugins();
const UnitTest::PreferencesSentinel prefsSentinel("Zarr", <bytes>, true);

const nx::core::UnitTest::TestFileSentinel testDataSentinel(...);
```

### 2. Determine the bytes threshold for each test case

The bytes threshold should be roughly **1 slice of the largest input array** in the dataset. To determine the dataset dimensions:

1. Check the test code for explicit array sizes (e.g., `std::array<uint8, 27>` = 3x3x3).
2. If the size isn't obvious from the code, inspect the `.dream3d` input file (HDF5 format) using `h5py`:
   ```python
   python3 -c "
   import h5py
   with h5py.File('path/to/input.dream3d', 'r') as h:
       def visit(name, obj):
           if isinstance(obj, h5py.Dataset):
               print(f'{name}: shape={obj.shape} dtype={obj.dtype}')
       h.visititems(visit)
   "
   ```
3. Calculate bytes for one 2D slice, accounting for **dtype size and component count**. The HDF5 shape is typically `(X, Y, Z, components)`. For example, a `(5, 5, 5, 3)` float32 dataset has 5x5 = 25 voxels per slice, times 3 components, times 4 bytes = **300 bytes per slice**. Pick a threshold near the largest array's slice size so that at least the largest arrays are forced out-of-core.

### 3. Verify in-core tests pass

```bash
cmake --build --preset "simplnx-Rel [BUILD]"
ctest --preset simplnx-Rel-test -R "FilterName"
```

All in-core tests must pass before proceeding.

### 4. Verify out-of-core tests pass (baseline)

```bash
cmake --build --preset "simplnx-ooc-Rel [BUILD]"
ctest --preset simplnx-ooc-Rel-test -R "FilterName"
```

Double check for "chunk shape:" printouts in verbose OOC test output. If absent, the data is not actually using ZarrStore.

### 5. Optimize the algorithm

Optimize the filter's algorithm implementation. Two approaches:

- **Optimize in-place**: Improve data access patterns, reduce random access, batch operations, etc.
- **New algorithm**: Write a fundamentally different algorithm that achieves the same results faster.

**Critical constraints:**
- Do NOT modify any input files or exemplar/expected data in the unit tests. The algorithm must produce identical results.
- Do NOT significantly degrade in-core performance to gain out-of-core speed. Both configurations must remain fast.
- After each change, rebuild and test BOTH configurations to ensure correctness.

### 6. Validate final results

After optimization, run the full test suite for the filter on both configurations. Both must pass with identical results.

Double check for "chunk shape:" printouts in verbose OOC test output. If absent, the data is not actually using ZarrStore.

### 7. Add a large benchmark test case

The existing unit tests typically use small datasets (3x3x3, 5x5x5) where test overhead dominates, making optimization speedups invisible. Add a benchmark test with a large programmatic dataset to demonstrate real-world performance.

**Key requirements:**

1. **Write to disk, then reload**: `CreateTestDataArray` creates in-memory `DataStore<T>`, not `ZarrStore<T>`. To exercise actual disk-backed OOC chunk access, write the programmatic DataStructure to a `.dream3d` file using `UnitTest::WriteTestDataStructure()`, then reload it with `UnitTest::LoadDataStructure()`. The reload respects `PreferencesSentinel` and creates `ZarrStore`-backed arrays for large data.

2. **Use 3D tuple shapes**: Arrays inside an `AttributeMatrix` must use 3D tuple shapes `{kDimZ, kDimY, kDimX}` matching the AttributeMatrix dimensions, NOT flat `{kTotalVoxels}`. The HDF5 write/read cycle will fail with out-of-bounds errors if shapes don't match.

3. **Bytes threshold**: Use roughly **1 slice of the largest input array**. For example, a 200x200x200 float32 array with 4 components: 200 x 200 x 4 x 4 = 640,000 bytes per Z-slice.

4. **Generate interesting data**: Uniform data (all identical orientations) makes the algorithm trivially fast. Create spatially-varying data that exercises all algorithm phases.

5. **No exemplar check needed**: The existing correctness tests prove the algorithm produces identical results. The benchmark only verifies the filter executes without error (`SIMPLNX_RESULT_REQUIRE_VALID`).

6. **Clean up temp file**: Remove the `.dream3d` file at the end of the test.

**Example benchmark pattern:**
```cpp
TEST_CASE("FilterName: Benchmark 200x200x200", "[Plugin][FilterName][Benchmark]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 640000, true);

  constexpr usize kDimX = 200, kDimY = 200, kDimZ = 200;
  constexpr usize kTotalVoxels = kDimX * kDimY * kDimZ;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/filter_benchmark.dream3d",
                                                   unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    ImageGeom* imageGeom = ImageGeom::Create(buildDS, "Image Geometry");
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    AttributeMatrix* cellAM = AttributeMatrix::Create(buildDS, Constants::k_Cell_Data,
                                                       cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    // ... create and fill arrays with cellTupleShape ...

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  // Stage 2: Reload (arrays become ZarrStore in OOC) and run filter
  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    FilterType filter;
    Arguments args;
    // ... set args ...
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
```

**Required includes for benchmark tests:**
```cpp
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include <cmath>
```

### 8. Performance report

After all tests pass, provide a before/after performance comparison using the benchmark test:

1. **Baseline**: Stash the optimized algorithm file, rebuild both configs, run the benchmark
2. **Optimized**: Pop the stash, rebuild, and time the same benchmark
3. **Verify OOC is real**: Check for "chunk shape:" printouts in verbose OOC test output. If absent, the data is not actually using ZarrStore.
4. **Report**: Present a summary table showing in-core and out-of-core before/after times, speedups, chunk shapes observed, and confirmation that all tests pass

**Interpreting results:**
- In-core speedup reflects pure algorithm improvement (no disk I/O)
- OOC speedup may be modest if disk I/O dominates — this is expected
- Small datasets (< 1000 voxels) will show no measurable difference — always use the large benchmark for timing

## Out-of-Core Architecture Reference

Understanding the OOC data storage system is essential for writing efficient algorithms.

### Storage Layer

When OOC is enabled (`SIMPLNX_FORCE_OUT_OF_CORE_DATA=ON` or arrays exceed `largeDataSize`), arrays use `ZarrStore<T>` (from the FileStore plugin) instead of in-memory `DataStore<T>`. Both inherit from `AbstractDataStore<T>`, so algorithm code uses the same API.

On disk, each array is a directory of Zarr chunk files. Each chunk is a compressed binary file named by its N-dimensional chunk index (e.g., `0.0.0`, `0.0.1`). Compression uses Blosc by default.

### Chunk Shape Calculation

`FileStore::createChunkShape()` determines how the array is partitioned into chunks:

1. Compute budget: `numValues = largeDataSize / sizeof(T) / numComponents`
2. Iterate dimensions from **fastest (X) backward** to slowest (Z)
3. Fill each dimension fully if cumulative product <= budget
4. When a dimension would exceed the budget, halve it repeatedly until it fits
5. Append component dimensions to the chunk shape

Example: A `(100, 200, 300)` float32 array with 4 components and `largeDataSize = 65536`:
- Budget: `65536 / 4 / 4 = 4096` values
- X=300 fits (300 <= 4096), Y=200: 300x200=60000 > 4096, so halve Y repeatedly until it fits
- Chunk shape: `(1, 12, 300, 4)` — full X slabs, partial Y, one Z layer

### Memory and Caching

The Zarr layer maintains a **fixed-size FIFO circular queue of 6 chunks** in memory (`MAX_BLOCK_COUNT = 6` in `IArray.hpp`). Key behaviors:

- **Auto-load**: `operator[]` automatically loads the correct chunk from disk when not cached
- **FIFO eviction**: When 6 chunks are loaded and a 7th is needed, the oldest is evicted (flushed to disk if dirty)
- **`memoryUsage()`** returns `sizeof(T) * chunkSize` — one chunk's memory footprint
- **Thread safety**: Mutex-protected queue and individual chunk access

### AbstractDataStore Chunk API

These methods are defined on `AbstractDataStore<T>` and work for both in-core and OOC:

| Method | In-core (`DataStore`) | OOC (`ZarrStore`) |
|--------|----------------------|-------------------|
| `getNumberOfChunks()` | Returns 1 | Returns actual chunk count |
| `loadChunk(flatIdx)` | No-op | Pre-loads chunk into the 6-slot cache |
| `flush()` | No-op | Writes all dirty cached chunks to disk |
| `getChunkLowerBounds(idx)` | `{0, 0, 0, ...}` | Chunk's lower corner in tuple space |
| `getChunkUpperBounds(idx)` | `{dimZ-1, dimY-1, dimX-1, ...}` | Chunk's upper corner (inclusive) |
| `getChunkShape()` | `nullopt` | Returns the chunk dimensions |

### Optimization Principles for OOC

1. **Sequential chunk iteration**: Process data chunk-by-chunk using `loadChunk()` to pre-load, then iterate within `getChunkLowerBounds()`/`getChunkUpperBounds()`. This ensures the current chunk is always cached and avoids thrashing. Pattern from `FillBadData.cpp`:
   ```cpp
   for(usize chunkIdx = 0; chunkIdx < store.getNumberOfChunks(); chunkIdx++)
   {
     store.loadChunk(chunkIdx);
     auto lower = store.getChunkLowerBounds(chunkIdx); // [z, y, x]
     auto upper = store.getChunkUpperBounds(chunkIdx); // [z, y, x] inclusive
     for(z = lower[0]; z <= upper[0]; z++)
       for(y = lower[1]; y <= upper[1]; y++)
         for(x = lower[2]; x <= upper[2]; x++) { ... }
   }
   ```

2. **Worklists over full rescans**: When iteratively processing a subset of voxels, use a worklist (deque/queue) to track which voxels need processing instead of rescanning the entire volume. This keeps data access localized to a few chunks.

3. **Cross-chunk neighbor access is OK**: The 6-chunk cache means accessing neighbors at chunk boundaries doesn't require explicit handling — adjacent chunks are likely still cached. Just access them normally via `operator[]`.

4. **In-core compatibility**: All chunk API calls are no-ops for `DataStore` (1 chunk spanning the full volume). Code using the chunk pattern runs identically in-core with zero overhead.

5. **Avoid random access patterns**: Random jumps across the volume cause worst-case chunk thrashing (evict -> load -> evict). Linear or spatially-localized access patterns are ideal.

6. **Pre-compute flat indices**: When accessing multi-component arrays, compute `flatIndex = tupleIndex * numComponents + comp` to use `operator[]` directly rather than `getComponentValue()` which adds overhead.
