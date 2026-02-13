# Project Guidelines for Claude

## Project Overview
<!-- Brief description of the project, its purpose, and primary language/framework -->
I want to modify 'src/simplnx/Common/Array.hpp' to have the functionality of 'src/simplnx/Common/Matrix3X1.hpp'. Specifically the `Vec3` portion of Array.hpp. At the bottom of the Array.hpp create C++ type alias from Vec3<T> to Matrix3x1 so that current codes will keep working correctly. Ensure that a thorough unit tests exists for the Vec3<T> class by creating the appropriate 'Catch2' unit test in the 'test' directory.

## Directory Structure
<!-- Key directories and what they contain -->
- `src/simplnx/` - Core library (Common, Core, DataStructure, Filter, Parameters, Pipeline, Plugin, Utilities)
- `src/Plugins/` - Plugin modules
- `src/nxrunner/` - CLI runner
- `test/` - Test files
- `cmake/` - CMake configuration

## Directories to Ignore
<!-- Directories Claude should not read or modify -->
- `wrapping/` - Python wrapping code
- `scripts/` - Build/utility scripts
- `conda/` - Conda packaging

## Coding Standards

### C++ Style (from .clang-format)
<!-- Language version, formatting rules, brace style, indentation, line length, etc. -->
<!-- Reference a formatter config if one exists (e.g., .clang-format, .prettierrc) -->
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
<!-- File extensions, class/function/variable naming patterns, prefixes/suffixes -->
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
<!-- How files are structured, where new files should go, header/source pairing conventions -->
- When creating a C++ based simplnx filter inside a plugin, the complete filter will have a "NameFilter.hpp" and "NameFilter.cpp" file, an "Algorithm/Name.hpp" and "Algorithm/Name.cpp".
- Filter documentation files are created in Markdown and are in the "docs" subfolder inside the Plugins directory
- Unit tests should be created in the 'test' subfolder and use the 'catch2' unit testing framework.


## Build System
<!-- Build tool (CMake, Gradle, npm, etc.), dependency management, key build commands -->
- CMake-based build system
- vcpkg for dependency management

Example - Configuring the build system
```bash
cd /path/to/source/directory
cmake --preset simplnx-Rel
```

## Testing
<!-- Test framework, how to run tests, test file naming/location conventions -->
- Unit tests use the Catch2 framework.
- Each `TEST_CASE` should include `UnitTest::CheckArraysInheritTupleDims(dataStructure);` near the end of the test to ensure all created data arrays have correct tuple dimensions inherited from their parent groups.


### Running Tests
<!-- Exact commands to run tests, flags for filtering by name, verbosity options -->
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

### Test Patterns
<!-- Common patterns used in tests (fixtures, mocks, exemplar data, etc.) -->

## Additional Notes
<!-- Any other project-specific rules, preferences, or conventions -->
