# Project Guidelines for Claude

## Project Overview

This is going to be a large refactoring of the simplnx code base. At a high level I would like the codebase to start using "Interfaces" and "Abstract" classes in places that are not currently being used, like the DataStructure class. I would like to implement more of a "COM" (Component Object Model) style of API for the simplnx library and the plugins. This is a very large undertaking so we should approach this in phases. Remember in C++, an 'Interface' class is a class with all pure virtual methods and a virtual destructor. AbstractBaseClasses have some shared code but are still not meant to be instantiated.

### Phase 1
Review the simplnx code and report which classes already inherit from an interface class. From this we can start formulating a plan to refactor the code. Do not proceed further until this part of the project is reviewed. Save the findings into a folder called "Code_Review".

### Phase 2
- [ ] Extract out an Inteface class from "SegmentFeatures"
- [ ] Make "SegmentFeatures" inherit from "ISegmentFeatures"
- [ ] Rename "SegmentFeatures" into "AbstractSegmentFeatures"
- [ ] All unit tests pass

### Phase 3
- [ ] Rename 'IFilter' to 'AbstractFilter'
- [ ] Create the "IFilter" interface from "AbstractFilter" class
- [ ] Update all code locations
- [ ] All unit tests pass

### Phase 4
- [ ] `AbstractPipelineNode`,  Extract `IPipelineNode`
- [ ] Update all occurances
- [ ] All unit tests pass

### Phase 5
- [ ] `AbstractPlugin` Extract `IPlugin`
- [ ] Update all occurances
- [ ] All unit tests pass

### Phase 6
- [ ] `IGeometry` and subclasses
- [ ] Change names to replace "I" with "Abstract" and extract out each "IGeometry" pure virtual subclass
- [ ] "I" prefix but not pure interfaces
- [ ] Update all occurances
- [ ] All unit tests pass

### Phase 7
- [ ] Create the IDataStructure interface based on the current DataStructure header. Then update DataStructure to inherit from IDataStructure and also update all methods within DataStructure to 'override' the IDataStructure methods.
- [ ] Update all occurances
- [ ] All unit tests pass (except for the one expected failure)
- [ ] Update CLAUDE.md with implemented features
- [ ] Git Commit changes if all tests pass (except for the one expected failure)
- [ ] Git Push changes if all tests pass (except for the one expected failure)

### Phase 8
Rerun the phase 1 review of the source code and create a new document called 'Code_Review/Phase_8_Interface_Review.md' 

### Phase 9
This phase we will clean up the last of the 'near-pure interface classes". Move concrete implementations to the Abstract class implementation.
- [x] Fix IDataStore
- [x] Fix IListStore
- [x] Fix IDataFactory

### Phase 10
- [x] Fix IParameter near-pure-virtual to create IParameter Interface and AbstractParameter
- [x] Unit Tests must pass (except for the one allowed failure)
- [x] Update Phase_8_Interface_Review.md
- [x] Update Phase_8_Class_Hierarchy.dot
- [x] Git commit changes
- [x] Git push changes


### Phase 11
The "I" prefix is now **correctly** used for pure interfaces in most places. The two remaining misnomers are:

| Class | Issue | Recommendation |
|-------|-------|----------------|
| `IArrayThreshold` | Has data members and concrete methods | Rename to `AbstractArrayThreshold`, extract `IArrayThreshold` interface |
| `IJsonPipelineParser` | Has data member and concrete method | Rename to `AbstractJsonPipelineParser`, extract `IJsonPipelineParser` interface |

Additionally, several "I"-prefixed classes in the DataObject hierarchy (`IArray`, `IDataArray`, `INeighborList`) are abstract base classes, not pure interfaces. These follow the older naming pattern and could be candidates for future renaming if full consistency is desired.

- [x] Fix remaining "I"-prefixed classes in the DataObject heirarchy and create the intermediate "Abstract" class where needed.
- [x] Unit Tests must pass (except for the one allowed failure)
- [x] Update Phase_8_Interface_Review.md
- [x] Update Phase_8_Class_Hierarchy.dot
- [x] Git commit changes
- [x] Git push changes

### Phase 12
- [x] Rename `IDataIOManager` → `AbstractDataIOManager` (file rename + class rename across 14 files + CMakeLists.txt)
- [x] Unit Tests must pass (except for the one allowed failure)
- [x] Update Phase_8_Interface_Review.md
- [x] Update Phase_8_Class_Hierarchy.dot
- [x] Git commit changes
- [x] Git push changes

### Phase 13
- [x] The Python wrapping now exposes "AbstractArray", "AbstractDataArray", "AbstractNeighborList" as Python class names — add
  backwards-compatible aliases if Python API stability matters
- [x] Unit Tests must pass (except for the one allowed failure)
- [x] Update Phase_8_Interface_Review.md
- [x] Update Phase_8_Class_Hierarchy.dot
- [x] Git commit changes
- [x] Git push changes

### Phase 14
- [x] Convert the "DataObject" to have the proper Interface-ABC-Concrete class heirarchy.
- [x] Unit Tests must pass (except for the one allowed failure)
- [x] Update Phase_8_Interface_Review.md
- [x] Update Phase_8_Class_Hierarchy.dot
- [x] Git commit changes
- [x] Git push changes

### Phase 15
- [x] Quick naming fixes — Rename the 8 misnamed I-prefixed classes to Abstract* (`IDataCreationAction`, `IDataIO`, `IGeometryIO`, `IGridGeometryIO`, `INodeGeom0dIO`-`INodeGeom3dIO`)
- [x] Rename `AbstractStringStore` → `IStringStore` (already a pure interface — no data members)
- [x] Unit Tests must pass (except for the one allowed failure)
- [x] Update Phase_8_Interface_Review.md
- [x] Update Phase_8_Class_Hierarchy.dot
- [x] Git commit changes
- [x] Git push changes

## Directory Structure
- `src/simplnx/` - Core library (Common, Core, DataStructure, Filter, Parameters, Pipeline, Plugin, Utilities)
- `src/Plugins/` - Plugin modules
- `src/nxrunner/` - CLI runner
- `test/` - Test files
- `cmake/` - CMake configuration
- Additional Plugin at "/Users/mjackson/Workspace5/DREAM3D_Plugins/SimplnxReview"
- Additional Plugin at "/Users/mjackson/Workspace5/DREAM3D_Plugins/FileStore"
- Additional Plugin at "/Users/mjackson/Workspace5/DREAM3D_Plugins/Synthetic"
- DREAM3DNX application located in "/Users/mjackson/Workspace5/DREAM3DNX"

## Directories to Ignore
- `scripts/` - Build/utility scripts
- `conda/` - Conda packaging

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

These conventions improve code clarity and distinguish between geometry objects and their underlying data references.

### File Organization
- When creating a C++ based simplnx filter inside a plugin, the complete filter will have a "NameFilter.hpp" and "NameFilter.cpp" file, an "Algorithm/Name.hpp" and "Algorithm/Name.cpp".
- Filter documentation files are created in Markdown and are in the "docs" subfolder inside the Plugins directory
- Unit tests should be created in the 'test' subfolder and use the 'catch2' unit testing framework.

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

## Build System
- vcpkg for dependency management
- CMake-based build system

Example configuring the project
```bash
cd /Users/mjackson/Workspace5/DREAM3DNX && cmake --preset NX-Com-Qt69-Vtk95-Rel
```

- Build directory is located at "/Users/mjackson/Workspace5/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel"

Example building the project
```bash
cd /Users/mjackson/Workspace5/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target all
```

Ensuring all test data files are downloaded
```bash
cd /Users/mjackson/Workspace5/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel &&  cmake --build . --target Fetch_Remote_Data_Files
```

Ensuring the out-of-core version of simplnx is configured
```bash
cd /Users/mjackson/Workspace5/simplnx && cmake --preset simplnx-ooc-Rel
```

Ensuring the out-of-core version of simplnx is build
```bash
cd /Users/mjackson/Workspace5/DREAM3D-Build/simplnx-ooc-Rel && cmake --build . --target all
```

- Python anaconda environment 'dream3d' can be used if needed

## Testing
- Unit tests use the Catch2 framework.
- Each `TEST_CASE` should include `UnitTest::CheckArraysInheritTupleDims(dataStructure);` near the end of the test to ensure all created data arrays have correct tuple dimensions inherited from their parent groups.
- Use the `ctest` to run unit tests

### Running Unit Tests
- Always use `ctest` to run unit tests, NOT the test binary directly
- The `ctest` command handles test data extraction and cleanup automatically
- Use the `-R` flag to run specific tests by name pattern

Example - Running a specific test:
```bash
cd /Users/mjackson/Workspace5/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && ctest -R "SimplnxCore::FillBadData" --verbose
```

Example - Running all SimplnxCore tests:
```bash
cd /Users/mjackson/Workspace5/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && ctest -R "SimplnxCore::" --verbose
```

### Printing debug statements in unit tests

Example - Correct

auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message){ fmt::print("{}\n", message.message); }});

### Exemplar-Based Testing Pattern
- Many tests use "exemplar" datasets - pre-generated golden reference data stored in `.dream3d` files
- Exemplar datasets are generated by running pipeline files (`.d3dpipeline`) that configure and execute filters

#### Workflow for Creating and Publishing Test Data:
1. **Generate test data locally**: Create pipeline file with filter configurations and `WriteDREAM3DFilter` to save results
2. **Execute pipeline**: Run the pipeline to generate exemplar `.dream3d` file and any input data files
3. **Package as tar.gz**: Compress test data (no `6_6_` prefix needed - that was only for legacy DREAM3D data)
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

#### Test Data Archive Naming and Versioning:
- **Base naming**: Use descriptive names that match the test: `test_name.tar.gz`
- **Version suffixes**: When updating existing test data, append version numbers: `test_name_v2.tar.gz`, `test_name_v3.tar.gz`
- **When to version**:
  - Original archive already exists in GitHub data archive
  - Test requirements changed (new exemplars, different parameters, additional data files)
  - Cannot overwrite original because other code may depend on it
  - CMakeLists.txt may reference both old and new versions for different tests
- **Check before creating**: Browse the [Data_Archive release](https://github.com/BlueQuartzSoftware/simplnx/releases/tag/Data_Archive) to see if your test data name already exists
- **Legacy prefixes**: The `6_6_` and `6_5_` prefixes are for data from legacy DREAM3D/SIMPL versions - do NOT use for new DREAM3DNX test data

#### Test Code Pattern:
```cpp
namespace
{
const std::string k_TestDataDirName = "test_name";
const fs::path k_TestDataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDataDirName;
const fs::path k_ExemplarFile = k_TestDataDir / "test_name.dream3d";
const fs::path k_InputImageFile = k_TestDataDir / "input_file.tif";
}
```

**Example**: If `import_image_stack_test.tar.gz` exists in the archive and you need to upload updated test data with new exemplars, create `import_image_stack_test_v2.tar.gz`. Update CMakeLists.txt to reference the new version, and optionally keep the old version if other tests depend on it.

#### Comparing Test Results Against Exemplars:
- **Load exemplar DataStructure**: Use `UnitTest::LoadDataStructure(exemplarFilePath)` to load the .dream3d file
- **ALWAYS use `REQUIRE_NOTHROW()` before `getDataRefAs<T>()`**: This applies to ALL `getDataRefAs` calls - both generated and exemplar data
- **Get generated data**: Use `getDataRefAs<T>()` wrapped in `REQUIRE_NOTHROW()` since objects were just created by the filter
- **Get exemplar data**: Use `getDataRefAs<T>()` wrapped in `REQUIRE_NOTHROW()` to verify the exemplar exists before accessing
- **Compare geometries**: Use `UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom)` - takes two pointers
- **Compare arrays**: Use `UnitTest::CompareDataArrays<T>(exemplarArray, generatedArray)` - type-specific template
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

**Important**: Use the standardized `UnitTest::` comparison methods directly in test code.

### Test Organization
- Each test should call `UnitTest::LoadPlugins()` before executing filters
- Use `DYNAMIC_SECTION()` for parameterized tests that generate multiple test cases

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

## Additional Notes
<!-- Add any other project-specific rules here -->
