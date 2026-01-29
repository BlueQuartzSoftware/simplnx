# Project Guidelines for Claude

## Project Overview
simplnx - A C++ project with a plugin-based architecture.

## Directory Structure
- `src/simplnx/` - Core library (Common, Core, DataStructure, Filter, Parameters, Pipeline, Plugin, Utilities)
- `src/Plugins/` - Plugin modules
- `src/nxrunner/` - CLI runner
- `test/` - Test files
- `cmake/` - CMake configuration

## Directories to Ignore
- `wrapping/` - Python wrapping code
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
- CMake-based build system
- vcpkg for dependency management

## Testing
- Unit tests use the Catch2 framework.
- Each `TEST_CASE` should include `UnitTest::CheckArraysInheritTupleDims(dataStructure);` near the end of the test to ensure all created data arrays have correct tuple dimensions inherited from their parent groups.

### Running Unit Tests
- Always use `ctest` to run unit tests, NOT the test binary directly
- The `ctest` command handles test data extraction and cleanup automatically
- Use the `-R` flag to run specific tests by name pattern

Example - Running a specific test:
```bash
cd /path/to/build/directory
ctest -R "SimplnxCore::FillBadData" --verbose
```

Example - Running all SimplnxCore tests:
```bash
cd /path/to/build/directory
ctest -R "SimplnxCore::" --verbose
```

### Printing debug statements in unit tests

Example - Correct

```cpp
auto executeResult = filter.execute(dataStructure, args, nullptr, IFilter::MessageHandler{[](const IFilter::Message& message){ fmt::print("{}\n", message.message); }});
```

Example - Normal Test 

```cpp
auto executeResult = filter.execute(dataStructure, args);
```

## Additional Notes
<!-- Add any other project-specific rules here -->
