# Code Style Guide

This is the style guide for the SIMPLNX library. When contributing sources to the simplnx repository we ask that all contributions follow this style guide. These general rules have been developed over the years in an effort to allow efficient coding practices.

This guide holds project conventions. A lesson learned from a specific defect, with its evidence, goes in the
[coding knowledge base](Coding_Knowledge_Base.md) instead.

## Source Code Line Endings

 All source code line endings should be in "Unix" style "\n". Every text editor understands these line endings on every platform **except** the "Notepad" application on Windows.

## Use of Tabs

Spaces should be used instead of hard tabs. This helps file portability across different editors. DREAM3D-NX uses a standard whereby all indents  use two spaces.

## Always Use an Include Guard

Always use

    #pragme once

at the top of *EVERY* header file.

## Special Constants

nullptr should be used in conjunction with only raw pointers and 0 (zero) should be used in conjunction with numeric values:

    int* value = nullptr; // Good
    int* value = 0; // Bad

    int i = 0; // Good
    int i = NULL; // Bad 

## Array Initialization

Prefer the use of C++11 std::array<N, T> instead of "C" style arrays.

    int32_t foo[3]; // BAD.. No initialization of the internal data
    std::array<int32_t, 3> foo = {0,0,0};

## The Dereference operator `*` and the Address-Of Operator `&` Should Be Directly Connected with the Type-Specifier

    int32*   p; // Correct
    int32   *p; // Incorrect
    int32*   p, q; // Probably error

The `int32*  p;` form emphasizes type over syntax while the `int32 *p;` form emphasizes syntax over type. Although both forms are equally valid C++, the heavy emphasis on types in C++ suggests that `int32* p;` is the preferable form.

## Always Implement the "Big Five" in C++ Classes

When writing C++ classes the programmer will always define the "Big Three" which are defined as:

- copy constructor
- copy assignment operator
- move constructor
- move assignment
- destructor

The programmer should **never** allow the compiler to implement these methods. This will happen if they are not explicitly defined in the class declaration. It is up to the develop to decide if the methods
should be marked as deleted or default'ed or manually implemented.

    class Foo
    {
      public:
        Foo();
        ~Foo();

        Foo(const Foo&) = delete;            // Copy Constructor Not Implemented
        Foo(Foo&&) = delete;                 // Move Constructor Not Implemented
        Foo& operator=(const Foo&) = delete; // Copy Assignment Not Implemented
        Foo& operator=(Foo&&) = delete;      // Move Assignment Not Implemented
    };

Note that with C++11, programmers now have the ability to inform the compiler which of these operations can be constructed by default and which can be ignored using the `default` and `delete` keywords.

## Naming conventions

There is an include clang-tidy file that most IDE's can use to assist with some of the major areas. One area is the naming conventions that SIMPLNX uses. Here are the basic rules:

- Class and Structs are `UpperCamelCase`
- Class private member variables are `m_UpperCamelCase`
- Class methods are `lowerCamelCase`
- Method arguments are `lowerCamelCase`
- Normal variables are `lowerCamelCase`
- Constants are `k_UpperCamelCase`
- Global statics are `s_UpperCamelCase`
- Free Functions are `UpperCamelCase`
- Macros are `ALL_UPPER_SNAKE_CASE`

## String Constants

String constants in C++ should be declared as:

    const std::string MyFile("SomeFile.dat");

Like constants should be grouped into a namespace or anonymous namespace if appropriate

    namespace MyConstants {
        const std::string MyFile("SomeFile");
    }

When using ANSI C one should use a char* for constant strings:

    const char* MyFile "SomeFile.dat";
    const char MyFile[5] = { 'a', '.', 'd', 'a', 't'};

Using this type of approach allows for quicker code updates when constant values need to be changed.

## Public Code

simplnx is public. Proprietary code stays in the private repository that owns it (for example SimplnxOoc or DREAM3D-NX). simplnx holds only public interfaces and integration points, and their names, comments and documentation do not describe a private implementation.

## Filters and Algorithms

### Algorithms Are Callable Without Their Filter

An algorithm class (the class a filter's `executeImpl()` constructs and runs) must be safe to call directly, without its filter. Other code may construct and run an algorithm without going through the filter's parameter validation and `preflightImpl()`. Examples include another algorithm, a test, or a future API.

The algorithm therefore checks every input property it relies on, and returns an error `Result` when one does not hold. This applies even when the filter's parameters or preflight already enforce the same rule. Check at least:

- component counts and component shapes
- tuple counts that must match between arrays
- data types
- index ranges that come from data, such as feature ids or phases used to index another array

Filter parameters and preflight give the user early feedback. They do not replace the checks in the algorithm. Do not remove a check from an algorithm because the filter makes it unreachable. Test each of these checks by calling the algorithm directly.

Write the checks that preflight can make once, in a static `ValidateInputs()` function of the algorithm class, and call it from both places. `preflightImpl()` calls it before it builds its output actions, and the algorithm's `operator()()` calls it before its first read. Do not write a second copy of a check in `preflightImpl()`: two copies drift apart in their conditions, error codes and messages. Build the input values in one function of the filter that `preflightImpl()` and `executeImpl()` both call, so that both check the same inputs with the same options.

- `ValidateInputs()` checks every input condition that parameter values and metadata can answer: scalar ranges and combinations of options, the existence and type of each input object, component shapes, tuple and geometry element counts, and the header of an input file. It checks an optional input only when its option is on, as execute does.
- It checks inputs only. Outputs, and other objects that the filter's own actions create, do not exist yet during preflight.
- When the filter's own actions move an input, preflight and the algorithm find it at different paths. `ExtractComponentAsArrayFilter` renames the selected array to a temporary name, and its algorithm reads the array under that name. The caller passes the path where the input is at that point (before the actions in preflight, after them in execute); `ValidateInputs()` does not guess the phase from which path exists.
- A count that an earlier filter computes in execute is only a placeholder during preflight: `QuickSurfaceMeshFilter` creates its Triangle Geometry with 0 faces (see entry 9 of the [coding knowledge base](Coding_Knowledge_Base.md)). `ValidateInputs()` can compare such counts with each other. A check that needs the final count, such as a check for an empty mesh, runs in execute, like the checks of array values (for example the index-range check below).
- A check that a parameter already makes (for example `AllowedComponentShapes`) still belongs in it. Through the filter that check runs twice, but a direct caller of the algorithm needs it.
- Build it from the shared utilities, such as `CheckArraysHaveSameTupleCount()` and `ValidateCellTupleCount()`. A reader checks the file header with the same function in both places; execute still handles a failed read, because the file can change after preflight.
- Checks that only the filter needs stay in `preflightImpl()`, for example checks of output names and of parameters that the algorithm does not read.

When an algorithm dispatches to more than one implementation (for example an in-memory branch and an out-of-core branch), call `ValidateInputs()` before the dispatch, so that every branch is covered. When a filter dispatches straight to two algorithm classes, as `ComputeGBCDPoleFigureFilter` does, one class owns `ValidateInputs()` and both call it first. A check that only one branch needs stays in that branch, before its first read. A filter whose actions do all of its work, such as `DeleteDataFilter`, needs no algorithm class.

Write the algorithm's preconditions as properties of its inputs (`@pre Epsilon and MinPoints are positive.`). Do not write them as checks that a filter made (`@pre Filter preflight has validated Epsilon.`), because a direct caller does not run that filter.

    // Algorithms/ExampleAlgorithm.cpp
    Result<> ExampleAlgorithm::ValidateInputs(const DataStructure& dataStructure, const ExampleAlgorithmInputValues& inputValues)
    {
      const auto* inputArray = dataStructure.getDataAs<IDataArray>(inputValues.InputArrayPath);
      if(inputArray == nullptr)
      {
        return MakeErrorResult(k_InputArrayMissing, fmt::format("The input array '{}' does not exist or is not a DataArray.", inputValues.InputArrayPath.toString()));
      }
      const usize numComponents = inputArray->getNumberOfComponents();
      if(numComponents != 2 && numComponents != 3)
      {
        return MakeErrorResult(k_InputComponentCount, fmt::format("Input array '{}' has {} components, but only 2 or 3 are accepted.", inputValues.InputArrayPath.toString(), numComponents));
      }
      return {};
    }

    Result<> ExampleAlgorithm::operator()()
    {
      Result<> validationResult = ValidateInputs(m_DataStructure, *m_InputValues);
      if(validationResult.invalid())
      {
        return validationResult;
      }
      // ... dispatch and compute, merging each Result into validationResult
      return validationResult;
    }

    // ExampleFilter.cpp, in preflightImpl()
    const ExampleAlgorithmInputValues inputValues = CreateInputValues(filterArgs);
    Result<OutputActions> resultOutputActions = ConvertResultTo<OutputActions>(ExampleAlgorithm::ValidateInputs(dataStructure, inputValues), {});
    if(resultOutputActions.invalid())
    {
      return {std::move(resultOutputActions)};
    }
    // ... append the output actions; keep these warnings when a later check fails

### Put Each Check at the Earliest Point That Can See Its Data

A filter checks its inputs at three levels. Put each check at the earliest level that has the information it needs:

1. **Parameter.** Structure that does not depend on other inputs: the allowed `DataType`s and component shapes (`AllowedTypes`, `AllowedComponentShapes`). Parameter validation runs before `preflightImpl()`.
2. **`preflightImpl()`.** Parameter values, relationships between inputs (tuple counts, geometry element counts), geometry and file metadata. Preflight cannot see array values.
3. **Execute.** Checks that need array values, such as feature ids or phases used as indices.

Do not repeat a parameter or preflight check inside the filter's `executeImpl()`. The algorithm still runs its `ValidateInputs()` (see above); that is the same check, not a second copy.

### Declare the Component Shape the Algorithm Reads

If an algorithm assumes a component count for an input `DataArray`, the parameter that selects that array declares it with `AllowedComponentShapes`. Without it, a 3-component array with the correct number of tuples passes preflight and the algorithm reads it with the wrong stride.

    params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "...", DataPath{},
                                                            ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

The match is exact: `AllowedComponentShapes{{1}}` rejects a `{1, 1}` array, and `AllowedComponentShapes{{3}}` rejects a `{1, 3}` array. When a change adds a component-shape or type constraint to an existing parameter, its pull request lists the shapes and types that saved pipelines can no longer use.

When the code means "number of tuples", call `getNumberOfTuples()`. `getSize()` returns tuples times components.

### Every Accepted Type and Value Must Work in Execute

Every `DataType` that a parameter allows must be handled by the execute code, and every value that preflight accepts must be usable. Do not allow `GetAllDataTypes()` when the algorithm cannot handle `bool`; do not allow `int8` when the algorithm stores face indices in the label type. Either narrow the parameter or support the type.

### Check Tuple Counts in Preflight

Tuple-count rules for input arrays:

- **An element array that the filter only reads** must have one tuple per geometry element (cells, faces, edges or vertices). It may live in any Attribute Matrix: selecting an array with the right tuple count from a different Attribute Matrix is a feature. For Image and RectGrid cells use `ValidateCellTupleCount()`. For node geometries compare `getNumberOfTuples()` with `getNumberOfFaces()`, `getNumberOfEdges()` or `getNumberOfVertices()`.
- **Sibling inputs** that the algorithm reads with the same index must have the same tuple count. Use `CheckArraysHaveSameTupleCount()`. Include an optional input (for example a mask) only when its option is on.
- **Outputs sized from an Attribute Matrix**: every input that sets the loop count must have the same tuple count as that Attribute Matrix.
- **A spatial filter that modifies Feature Ids, its sibling cell arrays, or the Feature Attribute Matrix** requires Feature Ids to be in the geometry's Cell Attribute Matrix. Use `ValidateArrayInGeometryCellData()`, `ValidateArraysInGeometryCellData()` and `ValidateFeatureAttributeMatrixInGeometry()`.

Every message names both paths and both counts, as the `bluequartz-skills:error-messages` skill requires:

    "The selected DataArray 'Mesh/Face Data/Face Labels' has 12 tuples, but the Triangle Geometry 'Mesh' has 24 faces."

### Range-Check Every Index That Comes From Data

A feature id, phase, face label or `NeighborList` entry used as an index into another array must be checked before the access. Preflight cannot see these values, so the check runs at the start of execute. For an unmasked 1-component `Int32Array`, call the overload of `ValidateFeatureIdsToFeatureAttributeMatrixIndexing()` that takes `shouldCancel`. That function checks every tuple and has no mask argument. For other inputs, scan the minimum and maximum and return an error. A filter with a mask checks only the ids that execute uses, with the same mask and sentinel rules as execute. A negative value is out of range unless the code skips negative values before it indexes.

An index that comes from a parameter value (for example **Phase Index**) is checked in preflight, because preflight can see it.

### Validate Every Parameter Value in Preflight

Most numeric parameters have no built-in range. `preflightImpl()` rejects every value the algorithm cannot use, before any cast. The documented domain of the algorithm sets the bounds of each parameter:

- spacings, steps, scales and distances: finite. Require a value greater than 0 only where 0 is not valid.
- counts, dimensions and sizes: a whole number. Require 1 or more only where 0 is not valid. A supported no-op value, such as 0 iterations, stays valid.
- indices: `0 <= index < size`
- related values: legal combinations of options. Accept equal bounds (`min <= max`) when the operation defines them; require `min < max` only when the algorithm needs strict order.
- values that will be cast to an array element type: inside the range of that type

Write each comparison so that NaN fails it:

    const float64 dim = rowData[idx];
    const float64 upperExclusive = std::ldexp(1.0, std::numeric_limits<usize>::digits);
    if(!std::isfinite(dim) || !(dim >= 1.0) || !(dim < upperExclusive) || std::floor(dim) != dim)
    {
      return MakePreflightErrorResult(k_InvalidTupleDim, fmt::format("Tuple dimension at index {} must be a whole number in [1, {}), but was {}", idx, upperExclusive, dim));
    }
    tupleDims.push_back(static_cast<usize>(dim));

This example requires `<cmath>` and `<limits>`.

Validation follows the parameter linking: validate a value only when the algorithm uses it, and always validate a value that the algorithm always reads. Every parameter reaches the algorithm's input values and changes its behavior, or it is removed. When a filter stores the random seed it used, the algorithm uses that same seed, so the stored seed reproduces the output.

A direction or axis parameter must not default to `{0, 0, 0}`, and preflight rejects a zero-length or non-finite vector.

### Preflight Must Not Throw

`IFilter::preflight()` does not convert exceptions into errors, so an exception in `preflightImpl()` can end the application. In preflight:

- Use `getDataRefAs<T>()` only when a selection parameter or an earlier check guarantees that the object exists with that type. The numeric-type list of an `ArraySelectionParameter` does not establish a numeric array kind. `MultiArraySelectionParameter::AllowedTypes` can establish that kind, but its `AllowedDataTypes` list does not establish the element type. See entry 4 of the [coding knowledge base](Coding_Knowledge_Base.md). Do not repeat a check that parameter validation already does. Otherwise use `getDataAs<T>()` and check for `nullptr`. `getDataRefAs<T>()` throws `std::out_of_range` when the path does not resolve and `std::bad_cast` when the object is a different type (for example a `float64` array where the code assumes `float32`, or a `StringArray` inside an Attribute Matrix that the code reads as `IDataArray`).
- Use `getCellData()`, `getVertexAttributeMatrix()`, `getFaceAttributeMatrix()` and similar accessors and check for `nullptr`. A geometry can have lost its Attribute Matrix.
- Use non-throwing library calls (`std::filesystem::directory_iterator(path, errorCode)`, `nlohmann::json::parse(text, nullptr, false)`), or catch the exception and return an error `Result`.
- Check `Result::invalid()` and `std::optional::has_value()` before calling `.value()`.
- When code walks every child of an Attribute Matrix, handle each array kind (`IDataArray`, `StringArray`, `INeighborList`) or return an error that names the array. Write that message by the `bluequartz-skills:error-messages` skill.

### Preflight Does Not Allocate or Read Array Data

Preflight runs every time a parameter changes in the GUI, on arrays whose stores are usually placeholders. It must stay cheap: its memory may grow with metadata and short previews, but not with the element data of an array.

- Declare every output through an action. In preflight an action does not allocate element storage: `CreateArrayAction` and the array-creation paths of the geometry actions create an `EmptyDataStore`, and `CreateNeighborListAction` creates an `EmptyListStore`. Do not create an array with real storage in `preflightImpl()` or in a helper that it calls. `CreateStringArrayAction` allocates every string in preflight and breaks this rule (issue #1892).
- `CopyDataObjectAction` and the copy paths of the geometry actions copy the store of their source. In pipeline preflight the source is usually a placeholder, so the copy is one too. `CopyArrayInstanceAction` creates a new array with the shape and type of its source, a placeholder in preflight.
- Use tuple, element and file-derived counts to compute output shapes. A shape stores dimension counts, not element values.
- Metadata may grow with phases, columns, files, tiles, output objects (even one output per input tuple, as in `SplitDataArrayByTupleFilter`) and the coordinates along one axis (`RegularizeZSpacingFilter` reads the Z + 1 slice bounds). A preview that preflight builds from parameter values holds a few tuples and may grow with the component count, as in `InitializeDataFilter`. Apart from these, do not allocate a scratch buffer, vector or set with one entry per cell, vertex, face or tuple.
- Do not read element values from a `DataStructure` array; read only its shape, type and counts. During preflight its store is usually an `EmptyDataStore`, and an element read from it throws.
- A reader may read past the file header when the format requires it to find or check the output structure. `ReadCSVFileFilter` scans the whole file to count its lines, `ReadDeformKeyFileV12Filter` reads the first record of each array to find its component count, and `ReadGrainMapper3DFilter` reads whole HDF5 metadata datasets. Read only what preflight needs. When a scan passes over array data, hold at most a fixed-size buffer or the one record that it needs, keep none of the scanned records, check `shouldCancel` during a long scan, and cache the result (see "Static Header Caches Need a Mutex"). Do not decode image pixels or load the data of an array only to find metadata.

### Preflight and Execute Make the Same Decisions

Preflight and execute must reach the same decision from the same inputs: which arrays are processed, output names, crop or resample indices, which outputs are enabled. Put that decision in one function and call it from both places. Do not copy the logic.

- Check every mode, every geometry type and every item of a list (every scan, every file, every input geometry), not only the first one.
- Execute writes every output that preflight creates, with the tuple and component shape that preflight gave it.
- Preflight declares every object that execute deletes or changes in place.
- Preflight does not leave temporary objects (for example a root-level temporary mask) in the `DataStructure`.

### In-Core and Out-of-Core Algorithms Give the Same Result

A filter that dispatches between an in-core (Direct) algorithm and an out-of-core (Scanline) algorithm has two copies of its logic. Both must give the same output for the same input. A fix, a guard or an initial value in one path goes into the other path too, preferably through a shared helper. Test both paths (see Testing).

### Build DataPaths From DataPaths

Build each `DataPath` from the path that the user selected or that preflight created. Do not build a path from a name string or with string operations.

- New child: `parentPath.createChildPath(name)`
- Sibling in the same parent: `path.replaceName(newName)`
- Same relative path under a different geometry: `childPath.rebase(oldGeometryPath, newGeometryPath)`
- A copied or moved subtree: rebase each descendant from the selected source to its final destination with `DataPath::rebase(oldPrefix, newPrefix)`, as `CopyDataObjectFilter` does. The destination can have a different path depth.

`DataPath({geometryName, "Cell Data", name})` is wrong when the geometry is inside a `DataGroup`. `StringUtilities::replace(path.toString(), oldName, newName)` changes every occurrence of the name: `Grain/Grain Data` becomes `New/New Data`. A parent or child whose name contains the same text is renamed too; copy and geometry operations that did this failed with error -5102 (pull request #1765).

### Never Drop a Result

Return every `Result` or merge it into the `Result` that the function returns. A later warning or error must not replace an earlier one. The text of each error and warning, and how to read the errors of a `Result`, follow the `bluequartz-skills:error-messages` skill.

    if(suppressedFaceCount == 0)
    {
      return MergeResults(std::move(windingResult), MakeNoFacesPrunedWarning(geometryPath));
    }

- An error from a sub-call keeps its own code and message. Add context to it; do not replace it with a generic code.
- Every `MakePreflightErrorResult(...)` is returned.
- A file writer checks the stream state (`fail()` or `bad()`) after the last write and after `flush()`. On failure it returns an error before `AtomicFile::commit()`, so a truncated file is not committed.
- A parallel task that can fail stores its first failed `Result`, and the caller returns it after the tasks finish.
- In execute, a loop over arrays, objects, tuples or chunks returns the first invalid `Result`, with the warnings collected so far. It does not collect one error for each item: when one cause (a full disk, a missing file) fails every item, hundreds of errors hide that cause. This applies also to a checked flush of a `DataStructure`. Preflight can merge the errors of independent checks. Execute can also merge a primary failure with the failure of the cleanup that follows it.
- When several independent calls add errors to one list, collect them in a local `std::vector<Error>` and return `{nonstd::make_unexpected(std::move(errors))}` only when the list is not empty. Do not pass `result.errors()` of a valid `Result` as that list: a valid `Result` holds no error vector, so the call asserts in a Debug build and is undefined behavior in Release.

### Give Empty Features a Defined Output

A feature with no cells or faces, and the reserved feature 0, get a defined value in every per-feature output. Do not compute 0/0 for them, and do not leave the value of an earlier feature in place.

### Write Floating-Point Output Through a Saturating Cast

A filter whose output type can be an integer type (for example an output type that follows the input type) writes each computed floating-point value through `ImageProcessing::detail::SaturateCastFromDouble<U>()`. For an integral `U`, it maps NaN to 0 and clamps to the range of `U`. For a floating-point `U`, it does an ordinary cast without saturation. A plain `static_cast` of NaN or an out-of-range value to an integer type is undefined behavior.

### Check Every Final Output Path in Preflight

For outputs that must stay distinct from their inputs, preflight resolves each output to its final file path or `DataPath`, after prefixes and suffixes, and rejects:

- two outputs that resolve to the same path (ignore case for file names)
- an output that is equal to an input
- a destination that is equal to or inside its source
- a selection that contains a path twice, or a path and its ancestor

An intentional in-place update, or a deferred replacement of an input, follows the documented contract of the filter and keeps the input until the replacement is ready. For example, with **Perform in Place** on, **Convert AttributeArray DataType** writes a temporary array, then deletes the input and renames the temporary array to the input path in deferred actions.

### Compare a Choice With Its Named Value

When the values of an enum are the indices of a `ChoicesParameter`, compare the selected value, and pass default values and parameter links, with `to_underlying(Enum::Value)`, as `RemoveFlaggedFeaturesFilter` does. Do not compare with a raw index such as `== 2`. Keep the order of the choices and the enum values stable: a saved pipeline stores the index, so reordering the choices changes what an existing pipeline does. When the enum values are not the indices, map the index through a lookup (see Geometry Conventions).

## Concurrency

### Static Header Caches Need a Mutex

`preflightImpl()` and `executeImpl()` are `const` and cannot directly modify non-`mutable` data members. By project convention, data that preflight reads (for example a file header) and execute reuses lives in a file-level static cache, keyed by an instance id. `IFilter::execute()` runs preflight on the same instance just before `executeImpl()`, so a successful preflight must leave every entry that execute needs ready for use. `ReadNIfTIFileFilter` shows the cache and its mutex; it uses the cache only in preflight. That cache is shared by every instance of the filter, and the GUI preflights filters on one thread while a pipeline executes on another. Lock one mutex around every access: in the constructor, the destructor, `preflightImpl()` and `executeImpl()`.

    std::mutex s_HeaderCacheMutex;

    MyReaderFilter::MyReaderFilter()
    : m_InstanceId(s_InstanceId.fetch_add(1))
    {
      std::lock_guard<std::mutex> lock(s_HeaderCacheMutex);
      s_HeaderCache[m_InstanceId] = {};
    }

### Out-of-Core Stores Are Written Serially

Out-of-core stores do not support concurrent writes. Threads that write to different tuples, or even to different chunks, of the same store can still race.

- Parallelize the computation on in-memory buffers, join the workers, then write the results to the store from one thread.
- When parallel writes matter for in-core speed, write separate in-core (parallel) and out-of-core (serial) algorithms and select one with the algorithm dispatch (see `docs/AlgorithmDispatch.md`).
- A `ParallelDataAlgorithm` or `ParallelTaskAlgorithm` whose workers access a store passes every array the workers read or write to `requireArraysInMemory()` (or `requireStoresInMemory()`). This does not load the data into memory: it runs the algorithm serially when any listed array is out-of-core. Each call replaces the previous decision, so call it once with all arrays, and call `setParallelizationEnabled(false)` after it, not before.

### Keep Optional Working-Memory Reservations Short

`CacheMemoryBudgetManager::reserveWorkingMemory()` grants memory from one working allowance that every caller shares (a quarter of the cache memory budget). The grant can be smaller than the request, and it can be 0.

- Use the size that was granted, not the size that was requested. The algorithm must also work with a smaller grant.
- Hold an optional reservation (extra slices for batching, cached coordinates) only while the algorithm uses it, so that it does not block required work of the storage backend.
- A `std::unique_ptr<std::vector<T>>` owner makes two allocations: the vector object and its elements. An exact charge includes `sizeof(std::vector<T>)` for each owner.
- Test with a budget that the request fills exactly.

The SimplnxOoc repository documents the ordering rules of its out-of-core stores.

### Long Loops Check for Cancel

Every loop that can run for a long time checks `shouldCancel` (or `getCancel()`), and has a progress guarantee: it counts real changes, caps the number of iterations, or applies a timeout.

Check `shouldCancel` also between the steps that come before the loop. After a step that can take a long time (a write, a read of a complete array), check it again before the next reservation, allocation or complete read.

## Data Stores

### Check Shape Products Before You Allocate

The product of a tuple shape or a component shape can be too large for `usize`. For `{2^63, 2}` an unchecked `std::accumulate` wraps the count to 0, and the store then has a buffer that does not match its shape. Every store that computes a count from a shape (`DataStore`, `EmptyDataStore`, `StringStore`, `ListStore`):

- looks for a zero dimension first. A shape with a zero dimension has a count of 0 whatever its other dimensions are, so `{SIZE_MAX, 2, 0}` and `{0, SIZE_MAX, 2}` give the same result. `EmptyDataStore::CheckedPlaceholderProduct()` shows this order.
- checks each multiplication for overflow, then the element count (tuples times components) and the byte count (elements times `sizeof(T)`).
- does all checks before it allocates memory or changes a member.

`resizeTuples()` returns an error for an overflow and keeps the store unchanged. `DataStore`, `StringStore` and `ListStore` return error -6035 for a resize failure. `EmptyDataStore` returns `EmptyDataStore::k_ResizeError` (-10603). A constructor throws, because it cannot return a `Result`. Each overflow message names the count that does not fit ("tuple count", "element count" or "byte count"), so that a test can tell the checks apart; the rest of its text follows the `bluequartz-skills:error-messages` skill. Issues #1883 and #1884 describe stores that break this rule.

### Check the Stores Before You Use Resident Pointers or Spans

A path that reads or writes through a raw pointer or a span into store memory checks every store it uses: each store reports `IDataStore::StoreType::InMemory` and casts to the required `DataStore<T>`. An algorithm-selection flag such as `ForceInCoreAlgorithm()` does not show that a store is in memory. When a check fails, use the store API instead (`ZeroCrossingEngine` checks both stores and both casts). Test that fallback with a store that is not a `DataStore<T>`.

## HDF5 Files

### A Dataset From `openDataset()` Is Not Valid Until It Is Used

`GroupIO::openDataset()` checks that the dataset exists and returns a `DatasetIO` that opens it only on first use, so `isValid()` returns false until then. Do not use `isValid()` to test that a dataset exists. Check `isDataset(name)` on the parent group, and check the `Result` of the first read. An empty `getDimensions()` does not mean that the dataset is missing: a scalar dataset has no dimensions.

## Geometry Conventions

- The Image Geometry origin is the minimum corner of cell 0. The center of cell `i` is `origin + (i + 0.5) * spacing`. Use `ImageGeom::getCoordsf()` or `getCoords()` for cell centers instead of a local formula.
- A reader that creates a geometry sets its length unit from the file or from the **Length Unit** parameter.
- Do not `static_cast` a choice index to an enum. The order of the choice list does not have to match the enum values (`IGeometry::LengthUnit::Unspecified` is 100, but it is choice index 27). Map the index through a lookup.

## Error Codes and Messages

Write every error and warning message by the `bluequartz-skills:error-messages` skill: it says which values, paths and corrective actions a message shows and how it formats them. The code-allocation rules below supplement that skill.

### Error Codes Are Unique

- Each condition gets its own error code; each warning condition gets its own warning code. Do not copy the code from the check above.
- Filter error codes are more negative than -10000. Each filter uses one range of 100 codes: if the first code is -47500, all codes of that filter are in -47500..-47599.
- Shared utilities use codes that no filter uses.
- Declare each code as a named constant (`constexpr int32 k_ZDimMismatch = -47512;`) and use the same constant in preflight and execute when they report the same condition.
- Before you use a new code, check that it is unique in the repository and in open pull requests:

        git grep -nE -- "-47512([^0-9]|$)"

    Do not rely on `\b`; it is not portable extended-regex syntax.

### Keep Error Codes Through a Refactoring

A refactoring keeps the error code of every condition whose meaning it does not change, also when the check moves to another function or class. Tests, documentation and users rely on the codes. When a condition must get a new code, the pull request description lists the old and the new code.

### Messages Name the Right Values

- Each message names the correct values in the correct order, and fires only on the condition it describes. Advice in a message must be something the user can do.
- A per-cell loop tests the mask first. Cells that the mask excludes are not validated, counted or grown.

## Testing

- **Expected values come from an independent oracle**: a hand calculation, a closed-form result, a reference implementation or verified DREAM3D 6.5.171 output. Never regenerate an exemplar only to absorb a change in output; show with the oracle that the new value is correct.
- **Generated and expected data come from different `DataStructure`s.** A comparison of an array with itself cannot fail.
- **Test every mode.** A test that only runs a mode is not enough; it compares the output values of that mode with expected values.
- **Test both algorithm paths.** For a filter with in-core and out-of-core algorithms, run each test that reaches the dispatch for both with `GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()))` and `UnitTest::AlgorithmTestScope`.
- **Test every axis and plane** for code that has one block per axis or plane.
- **Test error paths by code.** Assert the exact error code, not only `REQUIRE_NOTHROW` or an invalid result.
- **Test degenerate and invalid inputs**: empty geometries, out-of-range feature ids, NaN and limit values (`2^63`, `2^64`, `-1` and `256` for `uint8`), inputs in a `DataGroup` instead of at the root, and arrays from a different Attribute Matrix.
- **Tell apart conditions that share an error code.** Assert the code of the store that owns the condition: -6035 for a `DataStore`, `StringStore` or `ListStore` resize failure, and -10603 (`EmptyDataStore::k_ResizeError`) for an `EmptyDataStore` resize failure. When a contract gives several conditions the same code, the test also asserts a phrase that only the message of its condition contains.
- **Print the errors of a failed Result.** Use `SIMPLNX_RESULT_REQUIRE_VALID(result)` instead of `REQUIRE(result.valid())`. It prints each error code and message when the check fails.
- **Apply an action in both modes.** An `IDataAction` creates different objects in preflight and in execute. Test it with `GENERATE(IDataAction::Mode::Preflight, IDataAction::Mode::Execute)` and an `INFO` that names the mode.
- **Test a size failure without the allocator.** The nightly `asan` job runs with the default AddressSanitizer options. A request above the AddressSanitizer limit (1 TiB on 64-bit platforms) stops the test process instead of throwing `std::bad_alloc` (#1885). Use a size that the code rejects before it allocates, such as a shape product that overflows. Test a recoverable allocation failure through a deterministic failure seam. Test an intentional allocator abort in a separate process, with a timeout and a check of its exit result. An AddressSanitizer build alone does not make a real allocation failure recoverable.
- **Test core code in the core test target.** A test of code in `src/simplnx/` goes in the core `test/` directory and carries the `[simplnx]` tag. Core code must not depend on an optional plugin's tests for its only coverage. Each new exported helper gets its own direct test, also when a filter test already runs it.
- **Pass with optional plugins turned off.** Every test passes when an optional plugin is not built. A test that needs filters from an optional plugin skips them when the filter list does not have them, as `test/PrebuiltPipelineTest.cpp` does for shipped pipelines.
- **Name a filter test after its plugin.** The test name starts with the plugin name and its first tag is the plugin: `TEST_CASE("ImageProcessing::AbsImageFilter: ...", "[ImageProcessing][AbsImageFilter]")`. When a filter moves to another plugin, change both.
- **Hide a test of a known defect.** A test that shows an open defect asserts the correct result, carries the hidden tags `[.][BlockedDefect]` and names the issue in a comment. The change that fixes the defect removes the hidden tags.
- **Use unequal dimensions and spacings** in tests of an algorithm that depends on the geometry: different cell counts and spacings along X, Y and Z, so that a swapped axis changes the result (knowledge base entry 23).
- **No network access in CTest.** A test that needs a network service fails when the service is slow or not available. Do not register it with CTest. Keep the script so that it can be run by hand.
- **Call `UnitTest::LoadPlugins();` only when the test looks up filters by UUID.** Loading the plugins fills the `Application`'s filter list. A test needs it when it calls `createFilter()` on the filter list, loads a pipeline with `Pipeline::FromJson()`, `FromFile()`, `FromSIMPLJson()` or `FromSIMPLFile()` (every SIMPL backwards-compatibility test does), or runs a filter whose code gets other filters from the filter list (`ReadH5Ebsd` does). A test that constructs a filter or algorithm class directly does not call it, whether it builds its data in memory or reads it with `UnitTest::LoadDataStructure()`. The call loads every plugin library and fails the test when one does not load, so an unneeded call makes the test fail for a plugin it does not use. A test that needs the plugins calls `LoadPlugins()` itself: ctest runs each `TEST_CASE` in its own process, so no other test has loaded them.
- **Test an algorithm's own checks by calling the algorithm directly.** Test each failure of `ValidateInputs()` through `operator()()` and assert its error code; a test of `ValidateInputs()` alone does not show that the algorithm calls it. Such a failure returns before the dispatch, so its test does not use `UnitTest::AlgorithmTestScope`, which requires a branch to run. Test a check that is inside a dispatch branch on that branch. To reach an out-of-core branch in the in-core build, use a store whose `getStoreType()` returns `IDataStore::StoreType::OutOfCore` (the ImageProcessing tests do this). When that branch needs external sort or temporary record stores, register a test `IDataIOManager` that provides them, as described in the next item.
- **Restore the global I/O state.** A test that registers an `IDataIOManager` changes the `DataIOCollection` for the rest of the process. `DataIOCollection` has no remove call, so replace an existing manager under its format name and re-add the saved original in a destructor, which also runs when an assertion fails (see `ScopedImportSelectionManager` in `test/Dream3dLoadingApiTest.cpp`). Do not rely on ctest running each `TEST_CASE` in its own process: the test executable run directly runs many cases in one process, and a leaked manager makes later tests fail or skip their checks.

## Build Configuration

### The Python Interpreter Comes From the CMake Preset

Set the interpreter paths in the selected project or user preset. With `SIMPLNX_BUILD_PYTHON=ON`, `AddPythonTest()` gives `PYTHON_EXECUTABLE` to its test driver. Otherwise, it runs `Python3_EXECUTABLE` directly. Set the variable that the selected path uses before tests are registered. Keep machine-specific paths in `CMakeUserPresets.json`.

## Clang Formatting

`clang-format` is a tool to automatically format C++ code. You should run 'clang-format' on your code before opening a Pull Request to make sure it passes the *clang-format pr* check. You can install `clang-format` and `git-clang-format` with `npm install -g clang-format`. To automatically format a file according to a project's C++ code style, run `clang-format -i path/to/simplnx/file`, which is supported on macOS/Linux/Windows. If you want to run `clang-format` on all the changed code on your latest git commit (HEAD), you can run `git-clang-format HEAD~1`. Run `git-clang-format -h`  for extra help.
