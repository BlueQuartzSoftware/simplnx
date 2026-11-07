# Coding Knowledge Base

This is the rules document for simplnx and its plugins. Each entry is a rule that code in this repository has broken in
many places: a defect that kept coming back, or a project rule that many changes missed. Each entry gives the
rule, a real example of the broken rule, why the mistake fails, what to look for, and the pattern to use. The audience
is developers and the coding agents that work in this repository. Before you write or review code, read the index and
every entry for the area you change, and treat each rule as binding.

These rules are not repeated here:

- **Formatting:** `.clang-format`, which the *clang-format pr* check enforces on every pull request. Line endings come
  from `.gitattributes`.
- **Naming:** `.clang-tidy` and the naming policy of the `bluequartz-skills:fix-naming` skill.
- **General C++** (ownership, `std::array`, the special members, `nullptr`, numeric conversions, threads): the
  `bluequartz-skills:cpp-best-practices` skill.
- **Error and warning text:** the `bluequartz-skills:error-messages` skill.
- **Rules that depend on how the out-of-core stores work inside:** the coding knowledge base of the private SimplnxOoc
  repository.

Add or update an entry as "Where new rules go" in `AGENTS.md` describes. An entry needs a rule that was broken in many
places; a rule broken in one place, or a fix needed in one place only, does not get an entry. A systemic class of
defects that produce wrong data, as the `bluequartz-skills:bq-review-code` skill defines it, always gets one. Search
this file first and extend an existing entry instead of adding a second one. Cite pull requests and issues by number,
but do not say whether they are open, merged or closed: that changes, and the entry would go stale. A function, file or
test that an entry names from a pull request may not be in your checkout yet: check that it exists before you use it.
Refer to code by file and function or test name, not by line number. simplnx is public: an entry can name a private
repository, but contains no code from one.

## Index

- [Public code](#public-code)
  - [Keep private implementations out of simplnx](#keep-private-implementations-out-of-simplnx)
- [Filter inputs and preflight](#filter-inputs-and-preflight)
  - [Run a filter, not its algorithm](#run-a-filter-not-its-algorithm)
  - [Put each check at the earliest point that can see its data](#put-each-check-at-the-earliest-point-that-can-see-its-data)
  - [Declare the component shape the algorithm reads](#declare-the-component-shape-the-algorithm-reads)
  - [Support every type and value that a parameter accepts](#support-every-type-and-value-that-a-parameter-accepts)
  - [Check tuple counts in preflight](#check-tuple-counts-in-preflight)
  - [Range-check every index that comes from data](#range-check-every-index-that-comes-from-data)
  - [Validate every parameter value in preflight](#validate-every-parameter-value-in-preflight)
  - [Never throw in preflight](#never-throw-in-preflight)
  - [Do not allocate or read array data in preflight](#do-not-allocate-or-read-array-data-in-preflight)
  - [Make the same decisions in preflight and execute](#make-the-same-decisions-in-preflight-and-execute)
  - [Check every final output path in preflight](#check-every-final-output-path-in-preflight)
  - [Compare a choice with its named value](#compare-a-choice-with-its-named-value)
  - [Match image-cell tuple shapes to the Cell Attribute Matrix](#match-image-cell-tuple-shapes-to-the-cell-attribute-matrix)
- [Algorithms and results](#algorithms-and-results)
  - [Give the same result from the in-core and out-of-core algorithms](#give-the-same-result-from-the-in-core-and-out-of-core-algorithms)
  - [Build DataPaths from DataPaths](#build-datapaths-from-datapaths)
  - [Never drop a Result](#never-drop-a-result)
  - [Check the file close and the stream state before you report a successful write](#check-the-file-close-and-the-stream-state-before-you-report-a-successful-write)
  - [Check for cancel in every long loop](#check-for-cancel-in-every-long-loop)
  - [Write floating-point data as text that reads back to the same value](#write-floating-point-data-as-text-that-reads-back-to-the-same-value)
  - [Write floating-point output through a saturating cast](#write-floating-point-output-through-a-saturating-cast)
  - [Decide NaN and infinity handling by where the value comes from](#decide-nan-and-infinity-handling-by-where-the-value-comes-from)
  - [Copy every field that sets behavior when you clone an object](#copy-every-field-that-sets-behavior-when-you-clone-an-object)
  - [Determine symmetry support from Crystal Structures, not phase numbers](#determine-symmetry-support-from-crystal-structures-not-phase-numbers)
  - [Read histogram bin ranges as stored lower/upper pairs](#read-histogram-bin-ranges-as-stored-lowerupper-pairs)
- [Feature data](#feature-data)
  - [Do not compute anything for feature 0, and give empty features a defined output](#do-not-compute-anything-for-feature-0-and-give-empty-features-a-defined-output)
  - [Treat Feature data as stale after a filter changes the cells it was computed from](#treat-feature-data-as-stale-after-a-filter-changes-the-cells-it-was-computed-from)
  - [Use the same population in every pass of a statistic](#use-the-same-population-in-every-pass-of-a-statistic)
- [Geometry](#geometry)
  - [Take Image Geometry cell centers from the geometry](#take-image-geometry-cell-centers-from-the-geometry)
  - [Set the length unit of every geometry you create](#set-the-length-unit-of-every-geometry-you-create)
  - [Apply the hexagonal axis correction to both hexagonal Laue classes](#apply-the-hexagonal-axis-correction-to-both-hexagonal-laue-classes)
  - [Delete only geometry support data created by the current invocation](#delete-only-geometry-support-data-created-by-the-current-invocation)
- [Concurrency and storage](#concurrency-and-storage)
  - [Lock one mutex around a filter's static header cache](#lock-one-mutex-around-a-filters-static-header-cache)
  - [Write out-of-core stores from one thread](#write-out-of-core-stores-from-one-thread)
  - [Check shape products before you allocate](#check-shape-products-before-you-allocate)
- [Error codes and messages](#error-codes-and-messages)
  - [Give every condition its own error code](#give-every-condition-its-own-error-code)
  - [Name the right values in each message, and test the mask first](#name-the-right-values-in-each-message-and-test-the-mask-first)
- [Legacy pipeline compatibility](#legacy-pipeline-compatibility)
  - [Convert an optional legacy SIMPL key only when it is present](#convert-an-optional-legacy-simpl-key-only-when-it-is-present)
  - [Keep the legacy conversions when a filter moves or a plugin is retired](#keep-the-legacy-conversions-when-a-filter-moves-or-a-plugin-is-retired)
- [Testing](#testing)
  - [Take expected values from an independent oracle](#take-expected-values-from-an-independent-oracle)
  - [Test every axis and plane](#test-every-axis-and-plane)
  - [Assert a valid Result with SIMPLNX_RESULT_REQUIRE_VALID](#assert-a-valid-result-with-simplnx_result_require_valid)
  - [Test core code in the core test target with the simplnx tag](#test-core-code-in-the-core-test-target-with-the-simplnx-tag)
  - [Name a filter test after its plugin](#name-a-filter-test-after-its-plugin)
  - [Call UnitTest::LoadPlugins() only when the test looks up filters by UUID](#call-unittestloadplugins-only-when-the-test-looks-up-filters-by-uuid)
  - [Give Catch2 a bool when you compare file times](#give-catch2-a-bool-when-you-compare-file-times)
  - [Never put a semicolon in a Catch2 test case name](#never-put-a-semicolon-in-a-catch2-test-case-name)

---

## Public code

### Keep private implementations out of simplnx

**Rule:** simplnx is public. Proprietary code stays in the private repository that owns it (for example SimplnxOoc or
DREAM3D-NX). simplnx holds only public interfaces and integration points, and their names, comments and documentation
do not describe a private implementation. Core code (`src/simplnx/`) describes shared utilities in generic terms; it
selects a storage backend through `DataStorageMode` and a registered format name, never by naming the backend.

**Example of the broken rule:** none is recorded. This is a standing policy rule: a single violation publishes code
that cannot be withdrawn.

**Why:** everything in this repository, including its history, comments and pull-request descriptions, is public.

**What to look for:**

- A comment, name or document in simplnx that explains how a private store, cache or plugin works.
- Code copied from a private repository, or a header of a private repository included from simplnx.

**Pattern to use:** a public interface in simplnx (for example the data-store and IO-manager interfaces and
`docs/AlgorithmDispatch.md`) and the implementation in the private repository.

---

## Filter inputs and preflight

### Run a filter, not its algorithm

**Rule:** a filter checks its inputs in one place: parameter validation and `preflightImpl()`. The algorithm class (the
class that the filter's `executeImpl()` constructs and runs) relies on those checks and does not repeat them, because
`IFilter::execute()` runs them before every `executeImpl()` and a second copy would check the same inputs twice on
every run. An algorithm class is therefore not an entry point for other code. Code that needs a filter's work (another
filter, an algorithm or a test) constructs the filter and runs it: `IFilter::execute()` validates the arguments and
runs `preflightImpl()` before `executeImpl()`, and returns the preflight errors when they fail. `ReadImageStack` runs
`ReadImageFilter` this way for each file of the stack.

- The algorithm still checks what preflight cannot see: array values used as indices, such as feature ids and phases,
  and the result of every read and write (a file can change after preflight).
- A count that an upstream filter computes in execute is only a placeholder in preflight: `QuickSurfaceMeshFilter`
  creates its Triangle Geometry with 0 faces. Do not reject that placeholder in preflight; a check that needs the final
  count (an empty mesh, for example) runs in execute, as `SliceTriangleGeometryFilter` does with warning -62105.
- Do not call `IFilter::preflight()` before `IFilter::execute()`; `execute()` runs preflight itself. Call `preflight()`
  on its own only when the caller needs the preflight result without running the filter.
- Construct an algorithm class directly from other code only in the rare case where its filter cannot do the work.
  That caller then establishes every precondition that the filter's parameters and preflight would have checked, and a
  comment at the call names them.
- Document what the algorithm relies on with `@pre`, written as properties of its inputs
  (`@pre Epsilon and MinPoints are positive.`).
- Test an input check through the filter and assert its error code. Construct the algorithm in a test only when the
  test needs something that the filter hides, and then pass it inputs that preflight accepts.
- A report that an algorithm accepts an input that its filter's preflight rejects, when the algorithm is called
  without the filter, is not a defect. Issue #1887 reported this for the DBSCAN Epsilon; only its NaN case was a
  defect, because the preflight check `Epsilon <= 0` lets a NaN through.

**Example of the broken rule:** an algorithm's `operator()()` called the same static `ValidateInputs()` that its
filter's `preflightImpl()` had already called, so every run checked its inputs twice. During the review of PR #1763, a new empty-mesh check in preflight broke every pipeline that used **Create Surface Mesh
(QuickMesh)**, **Create Surface Mesh (Surface Nets)** or M3C before it ran.

**Why:** `IFilter::execute()` always runs preflight before `executeImpl()`, so an input check in the algorithm never
fails for a caller that runs the filter; it only costs time on every run and adds a second error code and message for
one condition. A caller that bypasses the filter also bypasses parameter validation, which the algorithm cannot
replace.

**What to look for:**

- An algorithm that repeats a parameter or preflight check, including a static `ValidateInputs()` that both
  `preflightImpl()` and `operator()()` call.
- Code outside a filter that constructs the filter's algorithm class instead of running the filter.
- A test that calls an algorithm's `operator()()` to reach an input check.
- A preflight check that rejects a count that an upstream action sets to 0 on purpose.

**Pattern to use:**

```cpp
// Run another filter from inside an algorithm or a filter.
const ReadImageFilter imageReader;
IFilter::ExecuteResult executeResult = imageReader.execute(importedDataStructure, BuildReadImageFilterArgs(subConfig));
if(executeResult.result.invalid())
{
  return executeResult.result;
}
```

### Put each check at the earliest point that can see its data

**Rule:** a filter checks its inputs at three levels. Put each check at the earliest level that has the information:

1. **Parameter:** structure that does not depend on other inputs (`AllowedTypes`, `AllowedComponentShapes`). It runs
   before `preflightImpl()`.
2. **`preflightImpl()`:** parameter values, relationships between inputs (tuple counts, element counts), geometry and
   file metadata. Preflight cannot see array values.
3. **Execute:** checks that need array values, such as feature ids or phases used as indices.

Do not repeat a parameter or preflight check in the filter's `executeImpl()` or in its algorithm class.

**Example of the broken rule:** PR #1535 removed geometry and Attribute Matrix existence checks that
`ComputeFeatureSizesFilter`, `ComputeFeaturePhasesFilter` and `ErodeDilateBadDataFilter` repeated in preflight although
their parameters already made them.

**Why:** a repeated check adds a second error code and message for one condition, and a check placed too late lets the
user run a pipeline that preflight could have rejected.

**What to look for:** preflight code that repeats what a selection parameter guarantees; execute code that first
checks metadata that preflight could have checked.

**Pattern to use:** parameter validation in `IFilter::preflight()`; relational checks in `preflightImpl()`;
value-dependent checks in the algorithm.

### Declare the component shape the algorithm reads

**Rule:** when an algorithm assumes a component count for an input `DataArray`, the parameter that selects that array
declares it with `AllowedComponentShapes`. The match is exact: `{{1}}` rejects `{1, 1}`, and `{{3}}` rejects `{1, 3}`.
When a change adds a shape or type constraint to an existing parameter, its pull request lists the shapes and types that
saved pipelines can no longer use. When the code means "number of tuples", call `getNumberOfTuples()`; `getSize()`
returns tuples times components.

**Example of the broken rule:** **Write STL File** accepted a one-component phase array although it reads two phases per
face (PR #1763). PR #1766 added the missing declarations to eight more filters: ComputeVertexToTriangleDistances,
ComputeShapesTriangleGeom, ComputeShapes, ComputeFeaturePhasesBinary, ComputeKMeans, ComputeKMedoids, ComputeNumFeatures
and DBSCAN.

**Why:** without the declaration, a three-component array with the right number of tuples passes preflight, and the
algorithm reads it with the wrong stride: wrong output, or a read past the end.

**What to look for:** a fixed-stride read (`index * 3`, `[2 * i + 1]`) of an input whose parameter has no
`AllowedComponentShapes`.

**Pattern to use:**

```cpp
params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Cell Feature Ids", "...", DataPath{},
                                                        ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
```

### Support every type and value that a parameter accepts

**Rule:** every `DataType` that a parameter allows is handled by the execute code, and every value that preflight
accepts is usable. Either narrow the parameter or support the type. Do not allow `GetAllDataTypes()` when the algorithm
cannot handle `bool`, and do not allow `int8` when the algorithm stores face indices in the label type.

**Example of the broken rule:** `InterpolatePointCloudToRegularGrid` created outputs of the source type, but execute
required `float64`, and a Boolean input reached a dispatch that did not support it (PR #1763). `ComputeNumFeatures`
accepted types beyond its `Int32Array` implementation (PR #1766). `RegularGridSampleSurfaceMesh` cast its output storage
to the input label type (PR #1765).

**Why:** a type that preflight accepts and execute cannot handle fails only at run time, with a `std::bad_cast` or a
wrong result.

**What to look for:** a broad allowed-type list followed by a fixed typed access (`getDataRefAs<Int32Array>`); an
output-type option that the writing code ignores.

**Pattern to use:** an explicit `int32` selection in `ComputeNumFeaturesFilter`; output-type dispatch in
`RegularGridSampleSurfaceMesh`.

### Check tuple counts in preflight

**Rule:**

- **An element array that the filter only reads** has one tuple per geometry element (cells, faces, edges or
  vertices). It may live in any Attribute Matrix. For Image and RectGrid cells use `ValidateCellTupleCount()`; for node
  geometries compare `getNumberOfTuples()` with `getNumberOfFaces()`, `getNumberOfEdges()` or `getNumberOfVertices()`.
  Get the cell count from the geometry, never from `getCellData()->getNumberOfTuples()`.
- **Sibling inputs** that the algorithm reads with the same index have the same tuple count:
  `CheckArraysHaveSameTupleCount()`. Include an optional input (a mask) only when its option is on.
- **Outputs sized from an Attribute Matrix:** every input that sets the loop count has that matrix's tuple count.
- **A filter that changes Feature Ids, their sibling cell arrays or the Feature Attribute Matrix in place** requires
  Feature Ids in the geometry's own Cell Attribute Matrix. Check membership by object ID, not by path: one DataObject
  can have several DataPaths. Use `ValidateArrayInGeometryCellData()`, `ValidateArraysInGeometryCellData()` and
  `ValidateFeatureAttributeMatrixInGeometry()`, then `MapToGeometryCellDataPath()` before the algorithm compares paths
  or finds sibling arrays. Check the ignored arrays only when the selected operation uses them. A Feature Attribute
  Matrix that a removal uses is a direct child of the selected geometry and differs from its Cell Attribute Matrix.

Every message names both paths and both counts.

**Example of the broken rule:** `AddBadData` did not check its Boundary Euclidean Distances input against the
geometry's cell count, nor did `ComputeFeatureSizes` and `WriteINLFile` (PR #1840). `ErodeDilateBadData`, `FillBadData`
and `RemoveFlaggedFeatures` took the arrays to change from `featureIds.getParent()` without checking that parent, and
compared selected paths as strings (PR #1829).

**Why:** an array with the wrong tuple count is indexed past its end or reads unrelated tuples. A path comparison misses
an array selected through another path to the same Attribute Matrix, and an unrelated parent selects the wrong arrays
to change.

**What to look for:**

- A geometry-indexed input checked only against its siblings, or not at all.
- `featureIdsPath.getParent()` used to find the arrays to change.
- `==` between selected `DataPath`s to decide membership.

**Pattern to use:** `ValidateCellTupleCount()`, `CheckArraysHaveSameTupleCount()` and the membership helpers in
`simplnx/Utilities/DataArrayUtilities.hpp`, as `RemoveFlaggedFeaturesFilter` uses them.

```text
"The selected DataArray 'Mesh/Face Data/Face Labels' has 12 tuples, but the Triangle Geometry 'Mesh' has 24 faces."
```

### Range-check every index that comes from data

**Rule:** a feature id, phase, face label or `NeighborList` entry used as an index into another array is checked before
the access. Preflight cannot see these values, so the check runs at the start of execute, or inside the main pass when
that pass reads each value before it uses it and writes no output until the pass ends. For an unmasked one-component
`Int32Array`, call the `ValidateFeatureIdsToFeatureAttributeMatrixIndexing()` overload that takes `shouldCancel`. For
other inputs, scan the minimum and maximum. A masked filter checks only the ids it uses, with the same mask and sentinel
rules as execute. A negative value is out of range unless the code skips it before it indexes. An index that comes from
a parameter value (for example **Phase Index**) is checked in preflight.

**Example of the broken rule:** `ComputeNumFeatures` indexed its ensemble output with unchecked Feature Phases
(PR #1766). The Direct algorithms of QuickSurfaceMesh and SurfaceNets transferred selected feature data without the
bounds check that their Scanline algorithms had (PR #1765).

**Why:** a value read from data can be anything; an unchecked index writes or reads out of bounds.

**What to look for:** `featureArray[featureIds[i]]`, `phaseArray[phases[i]]` or a neighbor-list entry used as an index
with no prior check of its sign and upper bound.

**Pattern to use:** `ValidateFeatureIdsToFeatureAttributeMatrixIndexing()` in `DataArrayUtilities`; the checked phase
scan in `ComputeNumFeaturesFilter`.

### Validate every parameter value in preflight

**Rule:** most numeric parameters have no built-in range. `preflightImpl()` rejects every value that the algorithm
cannot use, before any cast:

- spacings, steps, scales and distances: finite; greater than 0 only where 0 is not valid.
- counts, dimensions and sizes: whole numbers; 1 or more only where 0 is not valid. A supported no-op value, such as 0
  iterations, stays valid.
- indices: `0 <= index < size`.
- related values: legal combinations; accept `min <= max` when the operation defines equal bounds.
- values cast to an array element type: inside the range of that type.

Write each comparison so that NaN fails it. Reject NaN, and reject infinity unless the filter documentation gives it a
meaning; a value the user asks the filter to store in a floating-point array may be NaN or infinity
([Decide NaN and infinity handling by where the value comes from](#decide-nan-and-infinity-handling-by-where-the-value-comes-from)).
Validate a value only when the algorithm uses it (follow the parameter linking), and always validate a value that it
always reads. A direction or axis parameter does not default to `{0, 0, 0}`, and preflight rejects a zero-length or
non-finite vector. Every parameter reaches the algorithm's input values and changes its behavior, or it is removed. When
a filter stores the random seed it used, the algorithm uses that same seed, so the stored seed reproduces the output.

Validate the derived representation as well as the parameter value. For radii and dimensions, check derived widths,
signed coordinate and loop limits, padded extents, element counts and byte counts before the affected arithmetic or
allocation. An index must fit every array or axis it selects; use `index < count`, without subtracting one from the
count. For integer destinations, do not use a rounded floating-point representation of the maximum as an inclusive upper
bound. When fractional conversion is supported, validate the effective value and check related values in the type the
consumer uses. Preserve supported equality. Validate and normalize direction parameters with arithmetic that remains
safe for finite nonzero directions, then use that normalized value.

**Example of the broken rule:** `CreateDataArrayAdvanced` converted negative or fractional tuple dimensions to `usize`;
`InitializeData` accepted a reversed random range; `SliceTriangleGeometry` accepted an invalid slice spacing (all
PR #1763); SurfaceNets accepted a negative smoothing distance and iteration count (PR #1765). `WhiteTopHatImageFilter`
and `BinaryDilateImageFilter` narrowed unsigned radii before constructing kernels, and the constructor also used
unchecked derived sizes; `BinaryProjectionImageFilter` checked against rounded Float64 integer maxima;
`WriteGBCDGMTFileFilter` checked a phase against GBCD tuples but also used it to index Crystal Structures;
`ComputeGBCDPoleFigureDirect` normalized Float32 axes through a squared magnitude that can overflow for finite
components (issue #1873).

**Why:** a cast of an out-of-range or NaN value is undefined behavior, and `value < 0` alone lets NaN through. A value
can also fit its declared type while its derived size, converted value or contextual index is invalid, and a finite
vector can overflow during normalization; checking only the original value leaves these failures reachable.

**What to look for:** a `static_cast` of a parameter before a domain check; `if(value < 0)` on a floating-point value; a
zero step; an unchecked `min`/`max` pair. Also:

- radius-derived widths, padding, products or signed loop endpoints computed before validation;
- one selector used with several arrays but checked against only one count; an upper bound written as `count - 1`;
- integer-range guards using floating-point `max()` values; relations checked only before conversion;
- a direction check followed by normalization that squares components in a narrow type.

**Pattern to use:**

```cpp
const float64 dim = rowData[idx];
const float64 upperExclusive = std::ldexp(1.0, std::numeric_limits<usize>::digits);
if(!std::isfinite(dim) || !(dim >= 1.0) || !(dim < upperExclusive) || std::floor(dim) != dim)
{
  return MakePreflightErrorResult(k_InvalidTupleDim, fmt::format("Tuple dimension at index {} must be a whole number in [1, {}), but was {}", idx, upperExclusive, dim));
}
tupleDims.push_back(static_cast<usize>(dim));
```

`ValidateKernelRadius()` in `simplnx/Utilities/ImageProcessing/StructuringElement.hpp` and `ValidateMorphologyLayout()`
in `MorphologyEngine.hpp` validate derived sizes without constructing the kernel or allocating image values. Check each
contextual count before access and each relation after safe conversion. For a direction, first reject nonfinite
components and the zero vector, scale by the largest absolute component, and normalize the scaled vector with
sufficiently wide arithmetic.

### Never throw in preflight

**Rule:** `IFilter::preflight()` does not convert exceptions into errors, so an exception in `preflightImpl()` can end
the application. In preflight:

- Use `getDataRefAs<T>()` only when a selection parameter or an earlier check guarantees the object and its type.
  Otherwise use `getDataAs<T>()` and check for `nullptr`. Array kind and element type are different things:
  `ArraySelectionParameter` accepts any `IArray` and applies its numeric-type and component-shape lists only to
  `IDataArray`, so a `StringArray` can pass a numeric selection. `MultiArraySelectionParameter::AllowedTypes` can
  establish the array kind, but its `AllowedDataTypes` list does not establish the element type. A check that needs
  only the tuple count reads the selection as `IArray`.
- Use `getCellData()`, `getVertexAttributeMatrix()`, `getFaceAttributeMatrix()` and similar accessors and check for
  `nullptr`: a geometry can have lost its Attribute Matrix.
- Use non-throwing library calls (`std::filesystem::directory_iterator(path, errorCode)`,
  `nlohmann::json::parse(text, nullptr, false)`), or catch and return an error `Result`.
- Check `Result::invalid()` and `std::optional::has_value()` before `.value()`.
- When code walks every child of an Attribute Matrix, handle each array kind (`IDataArray`, `StringArray`,
  `INeighborList`) or return an error that names the array.

**Example of the broken rule:** selections that the parameters accepted caused `std::bad_cast` in preflight (issues
#1821 and #1833; fixed filter by filter in PRs #1817 and #1763). `ConvertDataFilter::preflightImpl()` still reads its
selection with `getDataRefAs<IDataArray>()`. `CropEdgeGeometry` cast every child of an Attribute Matrix by reference
(PR #1765). QuickSurfaceMesh dereferences a missing Cell Attribute Matrix (issue #1843).

**Why:** `getDataRefAs<T>()` throws `std::out_of_range` for a missing path and `std::bad_cast` for another type.

**What to look for:** `getDataRefAs` in `preflightImpl()` that relies only on the numeric-type list of a selection
parameter; a reference cast in a loop over Attribute Matrix children; an unchecked `getCellData()`.

**Pattern to use:** `getDataAs<Int32Array>()` with a `nullptr` check, as `ComputeSurfaceAreaToVolumeFilter` does; the
mask-size check of `DBSCANFilter`, which reads the selection as `IArray`; the per-kind handling in
`CropEdgeGeometryFilter`.

### Do not allocate or read array data in preflight

**Rule:** preflight runs every time a parameter changes in the GUI, on arrays whose stores are usually placeholders. Its
memory may grow with metadata and short previews, but not with the element data of an array.

- Declare every output through an action. In preflight an action does not allocate element storage:
  `CreateArrayAction` and the array-creation paths of the geometry actions create an `EmptyDataStore`, and
  `CreateNeighborListAction` creates an `EmptyListStore`. Do not create an array with real storage in `preflightImpl()`
  or in a helper it calls.
- `CopyDataObjectAction` and the copy paths of the geometry actions copy the store of their source. In pipeline
  preflight the source is usually a placeholder, so the copy is one too. `CopyArrayInstanceAction` creates a new array
  with the shape and type of its source, a placeholder in preflight.
- Compute output shapes from tuple, element and file-derived counts.
- Metadata may grow with phases, columns, files, tiles, output objects (even one output per input tuple, as in
  `SplitDataArrayByTupleFilter`) and the coordinates along one axis (`RegularizeZSpacingFilter` reads the Z + 1 slice
  bounds). A preview that preflight builds from parameter values holds a few tuples and may grow with the component
  count, as in `InitializeDataFilter`. Apart from these, do not allocate a scratch buffer, vector or set with one entry
  per cell, vertex, face or tuple.
- Do not read element values from a `DataStructure` array; read only its shape, type and counts. An element read from
  an `EmptyDataStore` throws.
- A reader may read past the file header when the format requires it to find or check the output structure
  (`ReadCSVFileFilter` counts the lines of the whole file; `ReadGrainMapper3DFilter` reads whole HDF5 metadata
  datasets). Read only what preflight needs. When a scan passes over array data, hold at most a fixed-size buffer or the
  one record it needs, keep none of the scanned records, check `shouldCancel` during a long scan, and cache the result
  (see "Lock one mutex around a filter's static header cache"). Do not decode image pixels or load an array's data only
  to find metadata.

**Example of the broken rule:** `CreateStringArrayAction::apply()` ignores the mode and allocates every string in
preflight (issue #1892). Geometry import read connectivity data in preflight until PR #1667. NeighborList creation
allocated real lists in preflight until PR #1483.

**Why:** a preflight that allocates or reads array data makes the GUI slow or out of memory for large data, on every
parameter change.

**What to look for:** an action `apply()` that ignores `mode`; populated stores created in preflight; a read of
connectivity or element values in `preflightImpl()`; scratch storage proportional to the element count.

**Pattern to use:** `CreateNeighbors()` in `DataArrayUtilities.hpp`, which selects an `EmptyListStore` in preflight.

### Make the same decisions in preflight and execute

**Rule:** preflight and execute reach the same decision from the same inputs: which arrays are processed, output
names, crop or resample indices, which outputs are enabled. Put the decision in one function and call it from both.

- Check every mode, geometry type and list item (every scan, file and input geometry), not only the first one.
- Execute writes every output that preflight creates, with the shape that preflight gave it.
- Preflight declares every object that execute deletes or changes in place.
- Preflight leaves no temporary objects in the `DataStructure`.

**Example of the broken rule:** `ReadImageStack` computed its physical Z crop indices differently in preflight and
execute, and `ReadImage` clamped a crop below the origin only in one of them (PR #1765).
`InterpolatePointCloudToRegularGrid` declared outputs of one type and wrote another (PR #1763).

**Why:** the pipeline plans memory, names and downstream filters from preflight; an execute that decides differently
writes data that no downstream filter expected.

**What to look for:** coordinate, path or option logic written twice; a function that preflight calls with the input
geometry and execute calls with the output geometry.

**Pattern to use:** `ComputeCroppedZRange()` in `ReadImageStackCropping.hpp`, shared by both phases.

### Check every final output path in preflight

**Rule:** for outputs that must stay distinct from their inputs, preflight resolves each output to its final file path
or `DataPath`, after prefixes and suffixes, and rejects:

- two outputs that resolve to the same path (ignore case for file names);
- an output that equals an input;
- a destination that equals or is inside its source;
- a selection that holds a path twice, or a path and its ancestor.

An intentional in-place update or a deferred replacement follows the filter's documented contract and keeps the input
until the replacement is ready (for example **Convert AttributeArray DataType** with **Perform in Place**).

**Example of the broken rule:** `WriteASCIIData` let two selected arrays resolve to the same case-insensitive file name
(PR #1765). `WriteBinaryData` and `WriteNodesAndElementsFiles` still allow it; with equal node and element paths, the
second write truncates the first (issue #1877).

`UncertainRegularGridSampleSurfaceMeshFilter::preflightImpl()` passed the geometry name as the cell-matrix name to
`CreateImageGeometryAction`, then appended `CreateAttributeMatrixAction` for the selected cell matrix. Equal names
caused duplicate creation; different names left the selected matrix outside the geometry's cell-data association.
Evidence: #1800, #1861 (BUG: C01: Copy-paste index drift in 15 filters), and PR #1919 (BUG: Fix copy-paste index
drift in 15 filters).

**Why:** a later write silently replaces an earlier output, or a copy reads from the place it is writing.

**What to look for:** output names built from base names and checked before the suffix is added; no comparison of
outputs with inputs.

- A composite creation action followed by another action for a child it already creates.
  `CreateImageGeometryAction` creates and associates its named Cell Attribute Matrix: pass the selected matrix
  name to that action and do not create that matrix again. Check children as well as top-level output paths. Apply
  the actions in regression tests with equal and distinct geometry/matrix names; a valid action list alone does
  not prove successful creation or the correct association.

**Pattern to use:** the final file-name collision map in `WriteASCIIDataFilter`.

### Compare a choice with its named value

**Rule:** when the values of an enum are the indices of a `ChoicesParameter`, compare the selected value, and pass
defaults and parameter links, with `to_underlying(Enum::Value)`, never with a raw index such as `== 2`. Keep the order
of the choices and of the enum values stable: a saved pipeline stores the index. When the enum values are not the
indices, map the index through a lookup; never `static_cast` a choice index to the enum.

**Example of the broken rule:** `ReadBinaryCTNorthstar`, `ReadImageFilter` and `ImportFijiMontageFilter` cast a
**Length Unit** choice index straight to `IGeometry::LengthUnit` (issue #1878). `LengthUnit::Unspecified` is 100, but
it is choice index 27, so the cast gives the wrong unit.

**Why:** a raw index breaks when choices are reordered, and a cast is wrong whenever the enum is not dense from 0.

**What to look for:** `static_cast<SomeEnum>(choiceIndex)`; `== 2` on a choice value.

**Pattern to use:** `to_underlying(...)` in `RemoveFlaggedFeaturesFilter`; an explicit lookup table for the length
unit.

### Match image-cell tuple shapes to the Cell Attribute Matrix

**Rule:** Image Geometry dimensions are `{X,Y,Z}`. Its Cell Attribute Matrix and cell arrays use tuple shape
`{Z,Y,X}`. Derive cell-array tuple shapes from that matrix's shape or, when declaring a new geometry, from the
same reversed dimensions as its creation action. Matching tuple products alone is insufficient.

**Example of the broken rule:** `UncertainRegularGridSampleSurfaceMeshFilter::preflightImpl()` passed `{X,Y,Z}` to
the label-array action although `CreateImageGeometryAction` created its cell matrix with `{Z,Y,X}`. Evidence:
#1800, #1861 (BUG: C01: Copy-paste index drift in 15 filters), and PR #1919 (BUG: Fix copy-paste index drift in 15
filters).

**Why:** reversing dimensions preserves the tuple count, so a count check misses incompatible shape metadata even
when the flat X-fastest values are unchanged.

**What to look for:**

- One dimension vector reused for both `CreateImageGeometryAction` and cell `CreateArrayAction`, without reversing
  its tuple axes.

**Pattern to use:** `RegularGridSampleSurfaceMeshFilter::preflightImpl()` uses XYZ for new geometry dimensions and
ZYX for cell tuples; its existing-geometry branch takes the actual cell-matrix shape. Test unequal dimensions such
as `{2,3,4}` and assert geometry `{2,3,4}` and matrix/array tuples `{4,3,2}`.

---

## Algorithms and results

### Give the same result from the in-core and out-of-core algorithms

**Rule:** a filter that dispatches between an in-core (Direct) and an out-of-core (Scanline) algorithm has two copies of
its logic, and both give the same output for the same input. A fix, guard, mask rule or initial value in one path goes
into the other, preferably through a shared helper or a check made before the dispatch. Test both paths: run each test
that reaches the dispatch with `GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()))` and
`UnitTest::AlgorithmTestScope`. When two algorithms would differ only in how they read and write, write one
implementation over bulk transfers, but only when it is not slower in-core (measure in a Release build and record it in
the pull request).

**Example of the broken rule:** QuickSurfaceMesh Direct and SurfaceNets Direct lacked the feature-transfer bounds check
of their Scanline algorithms (PR #1765). DBSCAN handled a multi-component mask differently in its two algorithms
(PR #1766).

**Why:** a user cannot see which branch ran; two branches that disagree give different results for the same pipeline
depending on the data size and the storage preferences.

**What to look for:** a guard or mask rule present in only one of `<Name>Direct.cpp` and `<Name>Scanline.cpp`; a test
that runs only the in-core branch.

**Pattern to use:** validation before the dispatch; checked transfers in `TupleTransfer.hpp`.

### Build DataPaths from DataPaths

**Rule:** build each `DataPath` from the path that the user selected or that preflight created, never from a name
string or with string operations.

- New child: `parentPath.createChildPath(name)`.
- Sibling: `path.replaceName(newName)`.
- Same relative path under another geometry: `childPath.rebase(oldGeometryPath, newGeometryPath)`.
- Copied or moved subtree: rebase each descendant with `DataPath::rebase(oldPrefix, newPrefix)`; the destination can
  have a different depth.

**Example of the broken rule:** `CopyDataObject` replaced every matching substring of the name in descendant paths, so
`Grain/Grain Data` became `New/New Data`. PR #1765 fixed the same pattern in `CropEdgeGeometry`, `RotateSampleRefFrame`,
`RemoveFlaggedVertices`, `PadImageGeometry`, `ImageGeometryCrop` and `ImageGeometryResample`; the copies failed with
error -5102.

**Why:** a string replacement also changes a parent or child whose name contains the same text, and
`DataPath({geometryName, "Cell Data", name})` is wrong when the geometry is inside a `DataGroup`.

**What to look for:** `StringUtilities::replace(path.toString(), ...)`; a `DataPath` built from a vector of names; an
assumption that source and destination have the same depth.

**Pattern to use:** `DataPath::rebase()`, `createChildPath()` and `replaceName()`, as `CopyDataObjectFilter` uses them.

### Never drop a Result

**Rule:** return every `Result` or merge it into the one the function returns. A later warning or error does not
replace an earlier one.

- An error from a sub-call keeps its own code and message; add context, do not replace it with a generic code.
- Every `MakePreflightErrorResult(...)` is returned.
- A parallel task that can fail stores its first failed `Result`, and the caller returns it after the tasks finish.
- In execute, a loop over arrays, objects, tuples or chunks returns the first invalid `Result` with the warnings
  collected so far; it does not collect one error per item. Preflight may merge independent checks.
- When independent calls add errors to one list, collect them in a local `std::vector<Error>` and return them only when
  the list is not empty. Never pass `result.errors()` of a valid `Result`.

**Example of the broken rule:** `ReadImage` discarded the warnings of its crop helper (PR #1765); `ReadImageStack`
dropped the warnings of its substeps, and QuickSurfaceMesh its winding warnings (PR #1767). Issue #1868 lists sub-call
results and warnings discarded in 22 filters.

**Why:** a dropped warning hides a changed result from the user; a dropped error turns a failure into a success.

**What to look for:** a sub-result whose `.value()` is used and whose warnings disappear; `return {}` after a call that
returned a `Result`; a new `Result` that replaces accumulated warnings.

**Pattern to use:**

```cpp
if(suppressedFaceCount == 0)
{
  return MergeResults(std::move(windingResult), MakeNoFacesPrunedWarning(geometryPath));
}
```

### Check the file close and the stream state before you report a successful write

**Rule:** a writer checks that its output is complete before it returns success. An HDF5 writer closes the file
explicitly and checks the result. A text or binary writer checks the stream state (`fail()` or `bad()`) after the last
write and after `flush()`. On failure it returns an error before `AtomicFile::commit()`, so an incomplete file never
replaces the old one.

**Example of the broken rule:** **Write DREAM3D-NX File** returned success while its `FileIO` object was still in
scope, so the destructor closed the file and discarded the result of `H5Fclose()`; an injected close failure still gave
a valid `Result` (issue #1832; PR #1859 adds `FileIO::closeChecked()`). The same gap is in
`DataStructureWriter::WriteFile()`, `DREAM3D::AppendFile()` and `DREAM3D::WriteXdmf()` (issue #1853), and in
`ExtractPipelineToFile`, `WriteGBCDTriangleData`, `WriteNodesAndElementsFiles` and `WriteSPParksSites` (issue #1868).

**Why:** HDF5 writes buffered metadata when it closes the file, and a stream buffers its last bytes until `flush()`:
successful writes before that do not prove that the file is complete.

**What to look for:** a path-based writer that returns while its file object is still in scope; a stream that is
written and closed without a state check; an atomic-file commit that follows either.

**Pattern to use:** merge the write result and a checked close before the commit, as the path-based write of
`DataStructureWriter` does with `closeChecked()` in PR #1859:
`return MergeResults(std::move(writeResult), fileWriter.closeChecked());`.

### Check for cancel in every long loop

**Rule:** every loop that can run for a long time checks `shouldCancel` (or `getCancel()`) and has a progress
guarantee: it counts real changes, caps the iterations or applies a timeout. Check `shouldCancel` also between the steps
before the loop, and again after a step that can take a long time (a write, a complete read). A filter that modifies its
input in place may finish once its first in-place write has happened, when its documentation says so.

When a neighborhood traversal discards positions outside a finite contributing domain, intersect its loop bounds with
that domain before iteration, and keep the contributing positions, their order and their weights. A valid large radius
is not invalid merely because the traversal spends time on discarded positions. Cancellation checks remain required.

**Example of the broken rule:** PR #1582 added missing cancel checks to 28 algorithms, among them ComputeAvgCAxes,
ComputeBoundaryStrengths, ComputeGBCDMetricBased, ComputeSchmids, MergeTwins, AlignGeometries, ComputeFeatureClustering,
MultiThresholdObjects and RobustAutomaticThreshold. `PointSampleTriangleGeometry` still has a masked rejection loop with
no cancel check or progress bound (issue #1876). `AdaptiveHistogramEqualizationEngine.hpp` enumerated the full radius
around each voxel and then discarded out-of-image coordinates (issue #1873); `ComputeNeighborhoods.cpp` similarly
enumerates radius-derived bins and discards absent bins (issue #1908).

**Why:** the user cannot stop a long filter, and a loop without a progress guarantee can run forever on bad input. Work
can also keep growing with a valid parameter after the useful neighborhood already covers the data.

**What to look for:** an inner rejection or convergence loop whose outer cancel check is never reached; an expensive
phase with no check between its steps; a radius-driven coordinate or bin loop followed by an out-of-domain `continue` or
an unsuccessful lookup.

**Pattern to use:** a check at each interruptible work boundary, as PR #1582 added them; the
`bluequartz-skills:progress-messaging` skill. For an image axis, intersect `[-radius, radius]` with
`[-position, dimension - 1 - position]` using signed arithmetic before iteration. Apply this only when omitted positions
contribute nothing; clipping a neighborhood that repeats clamped boundary samples changes its result.

### Write floating-point data as text that reads back to the same value

**Rule:** write floating-point data with fmt `{}` (shortest round-trip text) or `max_digits10` of the type actually
written. Apply this rule to files, text headers inside binary files, generated code and saved state. Never use `%f`,
`std::fixed`, `{:.Nf}`, `std::to_string(float|double)`, `digits10` or default stream precision for this data. A precision
that the user selects or that a file-format specification fixes is an exception; name the applicable contract.

**Example of the broken rule:** these writers lose floating-point data (issue #1927):

- `WriteAvizoUniformCoordinate.cpp`, `WriteAvizoUniformCoordinate::generateHeader()`, writes the `BoundingBox` with
  `%f`, including the text header in binary files. PR #1919 fixes this case.
- `WriteAvizoRectilinearCoordinate.cpp`, `WriteAvizoRectilinearCoordinate::writeData()`, writes ASCII coordinates with
  `%f`. `WriteVtkRectilinearGrid.cpp`, `WriteVtkRectilinearGrid::writeCoords()`, does the same (issue #1908 item L29;
  PR #1915).
- `WriteLAMMPSFile.cpp`, `WriteLAMMPSFile::operator()()`, writes bounds and atom coordinates with `{:f}`.
  `WriteINLFile.cpp`, `WriteINLFile::operator()()`, uses fixed stream formatting for header values and data rows.
- `WriteLosAlamosFFT.cpp`, `WriteChunkedData()` and `WriteDirectData()`, writes Euler angles with `{:.3f}`.
  `WriteGBCDTriangleData.cpp`, `WriteGBCDTriangleData::operator()()`, uses `{:0.4f}` for angles, normals and areas.
- `WriteGBCDGMTFile.cpp`, `WriteGBCDGMTFile::operator()()`, uses `%.1f` for misorientation parameters and `%f` for data
  rows. `WriteStatsGenOdfAngleFile.cpp`, `WriteEulerLine()`, uses fixed stream formatting with precision 8.
- `WriteNodesAndElementsFiles.cpp`, `WriteValue<T>()`, writes floating-point node data with fixed stream formatting
  and precision 4.
- `ConvertHexGridToSquareGrid.cpp`, `Converter::operator()()`, writes `.ang` rows with default stream precision.
  Its `modifyAngHeaderLine()` calls `StringUtilities::number()` with `digits10` for XSTEP and YSTEP.
- `Dream3dIO.cpp`, `WriteGeomXdmf()` for `ImageGeom`, writes XDMF origin and spacing with default stream precision.
  `OStreamUtilities.cpp`, `TupleWriter::write()`, uses 8 digits for float32 and 16 for float64 (issue #1867).
- `WriteAbaqusHexahedron.cpp`, `writeNodes()`, and `WriteAbaqusCrystalPlasticity.cpp`, `WriteNodes()`, format computed
  float64 coordinates with `{:.9g}`. Float32 inputs do not make the double expressions safe to write with 9 digits.
- `wrapping/python/utils/simplnx_utilities.py`, `_encode_float()`, rounds numeric parameters before it generates
  Python code.

**Why:** fixed decimal places turn small nonzero values into zero. Too few significant digits change a value when a
reader parses the text. `digits10` describes the decimal-to-binary guarantee, not the binary-to-text-to-binary
guarantee. Wider arithmetic can require more digits than the input type.

**What to look for:**

- `%f`, `{:f}`, `{:.Nf}`, `std::fixed`, `std::to_string()` or default stream precision on exported floating-point data.
- `digits10`, or a precision selected from the input type before arithmetic produces a wider type.
- Text in a binary header, a saved-state string or generated numeric code that passes through display formatting.
- Tiny positive values that become zero, or nearby values that become the same text.
- A claimed precision exception without a named user setting or file-format requirement.

**Pattern to use:** prefer `fmt::format("{}", value)`, as `WriteVtkStructuredPoints.cpp`,
`WriteVtkStructuredPoints::operator()()`, writes spacing and origin. For streams, use
`std::defaultfloat << std::setprecision(std::numeric_limits<T>::max_digits10)` for the type actually written.
`MhaImageIO.cpp`, `MhaImageIO::writePixelData()`, sets float32 `max_digits10` on a fresh stream with default notation.
Reset a prior fixed notation explicitly with `std::defaultfloat`. For printf, use
`fprintf(file, "%.*g", std::numeric_limits<T>::max_digits10, static_cast<double>(value))` for float32 or float64 `T`.

### Write floating-point output through a saturating cast

**Rule:** a filter whose output type can be an integer type (for example an output type that follows the input type)
writes each computed finite floating-point value through `ImageProcessing::detail::SaturateCastFromDouble<U>()`, which
clamps it to the range of `U`, and reports one warning with the number of clipped values. A NaN or an infinity computed
for an integer output is an error that names the value's index and the output type; it is never written as 0. The
general rule is in the numeric-correctness reference of `cpp-best-practices`.

**Example of the broken rule:** `ExpImageFilter` casts `exp(double(value))` straight to the integer output, and
`LogImageFilter`, `SquareImageFilter` and `TanImageFilter` do the same: `log(0)`, the `uint8` square of 16 and the
`uint8` exponential of 10 are undefined (issue #1874).

**Why:** a `static_cast` of NaN or an out-of-range floating-point value to an integer type is undefined behavior.

**What to look for:** `static_cast<U>(floatingExpression)` where `U` can be integral.

**Pattern to use:** `SaturateCastFromDouble<U>()` in
`simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp` for finite values; the nonfinite integer-output
error and the clipping warning of `SmoothingRecursiveGaussianImageFilter` and `CurvatureFlowImageFilter` (PR #1888).

### Decide NaN and infinity handling by where the value comes from

**Rule:** NaN and positive and negative infinity (nonfinite values) are handled by where the value comes from:

- **Parameter values:** preflight rejects NaN. It also rejects positive and negative infinity unless the filter
  documentation gives infinity a meaning, such as an unbounded side of a range. A value that the user asks the filter
  to store in a floating-point array (a fill value or a replacement value) may be NaN or infinity.
- **Values read from a file:** the reader rejects a nonfinite value where the format requires a finite one, with an
  error that names the file and the field.
- **Input array values** that would change a result other than their own element's output, or would reach a
  comparison, an ordering, a range, a histogram, a threshold, a color map, a search, an index or an integer conversion.
  This covers reductions (sums, means, standard deviations, extrema and data ranges, histograms, medians, modes and
  unique-value counts, per feature or per region), neighborhood operations on floating-point data (smoothing, median and
  gradient filters, morphology) and decisions made from a value (thresholds, segmentation tolerances, bin and index
  selection). The filter has a choice parameter **Nonfinite Input Handling** (key
  `nonfinite_input_handling_index`) with two choices, **Error** (the default) and **Warn and Continue**.
  - **Error** stops at the first nonfinite value, before any output is written. The message names the array and the
    tuple and says that **Warn and Continue** handles these values instead.
  - **Warn and Continue** computes ranges and bounds from finite values only, gives each nonfinite element the result
    that the filter documentation states (for example "first bin" or "Outside Value"), and reports one warning with
    the number of nonfinite values. For reductions and neighborhood operations, exclude nonfinite values from the
    contributing samples and counts.
  - The in-core and out-of-core algorithms use one shared test and rule. The parameter has no effect on integer arrays.
- **Element-wise floating-point math**, where one element in gives the same element out and no other element changes,
  lets the IEEE result pass through. The filter documentation lists what NaN, infinity, zero and out-of-domain inputs
  produce. When integer input can produce a fraction or NaN, the output type is floating point.
- **A computed value written to an integer output:** see
  [Write floating-point output through a saturating cast](#write-floating-point-output-through-a-saturating-cast).
- **A result that is undefined for degenerate input** (a zero-length vector, an all-zero quaternion) is NaN in a
  floating-point output, and the documentation says so. It is not reported as bad input data.

These rules apply to new filters and to filters a change already fixes; changing an existing filter's output type or
failure behavior to meet them is a breaking change that its pull request names.

**Example of the broken rule:** **Compute Bounding Box Stats** gives two different wrong answers for Float32 values
{NaN, 1, 2}: the in-core algorithm treats NaN as equal to every value (`Equivalent()` in
`ComputeBoundingBoxStatsDirect`) and reports one unique value and a NaN median, and the out-of-core algorithm sorts with
`operator<`, which is undefined when a value is NaN (issue #1867). **Compute Array
Histogram** groups values with the same equality test, and the shared statistics helpers in `StatisticsCalculations.hpp`
and `HistogramUtilities.hpp` sort and key floating-point values with `operator<`. `ComputeArrayMinMax` is not NaN-aware
(issue #1874). **Invert Intensity Image** accepts a NaN **Maximum** and writes 0 to every integer pixel (issue #1873).

**Why:** every comparison with NaN is false. `operator<` is then not a strict weak ordering, so `std::sort`,
`std::stable_sort` and `std::map` keys are undefined; a threshold or a bin search puts NaN on one side without saying
so; an equality test written as "neither is less than the other" groups NaN with every value; and a cast of NaN or an
infinity to an integer is undefined behavior. Whether a nonfinite input value means "bad data" or "no measurement"
depends on the user's data, so input array values get a choice; parameters and file values do not.

**What to look for:**

- `std::sort`, `std::stable_sort`, `std::map`, `std::set` or `std::unique` over floating-point input values;
- an equivalence written as `!(a < b) && !(b < a)` on floating-point values;
- a running minimum, maximum or sum over input values with no finiteness test;
- a threshold, tolerance or bin index computed from an input value with no finiteness test;
- a parameter check written as `value < 0` or `value <= 0`, which NaN passes.

**Pattern to use:** **Nonfinite Input Handling** in `ThresholdMaximumConnectedComponentsImageFilter` and
`WriteImageFilter` (PR #1888); the Upper Boundary of `ThresholdMaximumConnectedComponentsImageFilter` for a documented
meaning of infinity (PR #1888).

### Copy every field that sets behavior when you clone an object

**Rule:** the `clone()` of an action or a parameter keeps all of its configuration. When a constructor gets a new
argument, update `clone()` in the same change, and test the clone with a value that is not the default: apply the
original and the clone to separate `DataStructure`s and compare.

**Example of the broken rule:** five geometry-creation actions (`CreateGeometry1DAction`, `CreateGeometry2DAction`,
`CreateGeometry3DAction`, `CreateRectGridGeometryAction`, `CreateVertexGeometryAction`) lost their created geometry path
when cloned (PR #802). PR #1838 added length units to geometry creation, and a cloned `CreateImageGeometryAction`
reverted to Micrometer.

**Why:** the framework copies actions and parameters through `clone()` (see `AnyCloneable`) with no visible call at
the filter. A field that the copy loses changes the output without an error.

**What to look for:** a constructor that gets a new argument while `clone()` is not part of the change.

**Pattern to use:**

```cpp
return std::make_unique<CreateImageGeometryAction>(getCreatedPath(), m_Dims, m_Origin, m_Spacing, m_CellDataName,
                                                   m_Units);
```

### Determine symmetry support from Crystal Structures, not phase numbers

**Rule:** use a validated phase index to read Crystal Structures, then compare that property with the named
supported symmetry. Apply this distinction to computations and support warnings. Keep phase-zero and same-phase
eligibility policies separate from symmetry support.

**Example of the broken rule:** ComputeSlipTransmissionMetrics skipped cubic material at phase 2;
ComputeBoundaryStrengths warned for cubic phase 2 but missed noncubic phase 1. Evidence: #1861 (BUG: C01:
Copy-paste index drift in 15 filters), PR #1919 (BUG: Fix copy-paste index drift in 15 filters), and #1880 finding
#3.

**Why:** a phase number identifies an ensemble tuple; it does not encode that tuple's crystal structure.

**What to look for:**

- `phase == 1` or `phase != 1` deciding cubic support, including warning-only branches. Trace the operand: a
  crystal-structure value and a phase index are different domains.

**Pattern to use:** the Crystal Structures lookup and named `Cubic_High` comparisons in
`ComputeSlipTransmissionMetrics::operator()()`. Test cubic material at phase 2 and noncubic material at phase 1;
relabeling the same material must not change its symmetry eligibility.

### Read histogram bin ranges as stored lower/upper pairs

**Rule:** `FillBinRanges` stores two scalars per bin: lower and upper at offsets `2*b` and `2*b+1`. Do not
interpret that buffer as `N+1` edges or reconstruct counting widths from stored, possibly truncated bounds. Modal
lookup uses the original counting parameters and classification policy, then reads the selected stored pair.

**Example of the broken rule:** ComputeArrayHistogramByFeature used an unscaled bin offset;
ComputeArrayHistogram's Direct and Scanline lookups inferred an edge sequence. For ranges `{0,2,2,4}`, mode 2 must
select `{2,4}`, not `{2,2}` or `{2,3}`. Evidence: #1861 (BUG: C01: Copy-paste index drift in 15 filters) and PR
#1919 (BUG: Fix copy-paste index drift in 15 filters).

**Why:** paired storage repeats interior boundaries, and integer bounds can truncate the counting width. An
edge-based interpretation selects the wrong interval.

**What to look for:**

- A `FillBinRanges` consumer using `ranges[b]` / `ranges[b+1]`, `ranges.size()-1` as the bin count, or a width
  reconstructed from stored bounds.

**Pattern to use:** `StatisticsCalculations::findModalBinRange()` in `StatisticsCalculations.hpp`, with the
caller's existing counting policy and eligibility. Test a nonzero modal bin and assert the stored pair
independently, including integer ranges with fractional counting widths.

---

## Feature data

### Do not compute anything for feature 0, and give empty features a defined output

**Rule:** Feature Id 0 marks cells that belong to no feature (mount material or bad data). Code does not compute
anything for feature 0, does not store results in it, and does not treat it as a neighbor feature, unless the user
explicitly asked for that; the filter's documentation then says so. The feature-0 tuple of a per-feature output, and the
tuple of any feature with no cells or faces, get a defined value (normally the output's default) without a calculation:
never 0/0, and never the value of the previous feature. A review flags every computation for feature 0 and asks the
author what feature 0 means there.

**Example of the broken rule:** `ComputeNeighborhoods` included feature 0 as a search source and candidate (PR #1652).
`ComputeShapes`, `ComputeShapesTriangleGeom` and `ComputeSurfaceAreaToVolume` Direct divide by zero for an empty feature
(issue #1875).

**Why:** feature 0 is not a grain; a value computed for it is meaningless, and a value left over from the previous loop
iteration looks real.

**What to look for:** a feature loop that starts at 0; a division before an empty check; a per-feature output written
only for non-empty features.

**Pattern to use:** the explicit feature-0 and empty-feature handling in `ComputeNeighborhoods`.

### Treat Feature data as stale after a filter changes the cells it was computed from

**Rule:** Feature arrays (`NumElements`, `Volumes`, `EquivalentDiameters`, `Centroids`, `AxisLengths`, `AvgQuats`,
`SurfaceFeatures`, every `NeighborList`, ...) are computed from the cell data: which cells exist, their Feature Ids and
their positions. A filter that crops, resamples, pads, rotates, translates or smooths the geometry, removes or renumbers
features, or writes Feature Ids in place makes them stale. Such a filter never passes the old Feature data on as if it
were valid: it clears the affected Feature data, updates it, or names every stale array in preflight so that the user
computes it again. Code after such a filter never uses a Feature array computed before it. When a filter renumbers
features, it maps every Feature ID stored inside Feature data too (the contents of a `NeighborList`), not only the
tuple order.

**Example of the broken rule:** **Crop Geometry (Image)** copied the whole Feature Attribute Matrix with its old values
and warned only about NeighborLists. Features cut by the crop kept their old sizes and centroids, and features entirely
outside it kept their tuples; a user could take `EquivalentDiameters` from a cropped geometry straight into statistics.
Resample and the feature-removal filters had the same defect (issue #1699; PRs #1748 and #1913). Issue #1918 lists 17
more filters that still do it, among them Replace Element Attributes with Neighbor (Threshold), the Align Sections
filters and the NeighborList contents of Randomize Feature Ids.

**Why:** the stale arrays keep their names and plausible values, so nothing downstream fails: the results are wrong
without any error.

**What to look for:**

- A filter that writes Feature Ids, copies neighbor tuples into all cell arrays (Feature Ids included), shifts sections,
  zero-fills cells, or changes the cell count, origin, spacing or vertex positions, while a Feature Attribute Matrix can
  persist, and that neither clears nor flags it.
- A filter that permutes Feature tuples with `swapTuples()` without mapping the IDs stored in `NeighborList`s.
- An action that copies a Feature Attribute Matrix into a new geometry.

**Pattern to use:** the feature-removal filters (`RequireMinimumSizeFeatures`, `RequireMinNumNeighbors`,
`RemoveFlaggedFeatures`, `KeepRemoveRankedFeatures`) name the stale Feature arrays in preflight and remove the
NeighborLists they cannot keep; the geometry-changing filters clear or flag their Feature Attribute Matrix. Use the
shared helpers in `simplnx/Utilities/FilterUtilities.hpp` rather than a new message of your own, and say in the filter's
documentation which Feature data must be computed again.

### Use the same population in every pass of a statistic

**Rule:** counting, gathering, summing and variance passes use the same eligibility test. Reset the denominator of each
group before the code processes that group.

**Example of the broken rule:** **Compute Feature Average Orientations** counted only cells with a valid phase but
gathered the phase-0 quaternions into its vMF and Watson averages (issue #1659, PR #1664).
`ComputeFeatureNeighborMisorientations` and `ComputeFeatureNeighborCAxisMisalignments` reset their neighbor-average
denominators in the wrong loop (PR #1631).

**Why:** the numerator, the denominator and the deviations must describe the same population.

**What to look for:** a statistic with several passes whose `if` conditions differ; a denominator initialized outside
the loop over the groups it counts.

**Pattern to use:** the matching phase and feature predicates of the counting and gather passes in
`ComputeAvgOrientations`.

---

## Geometry

### Take Image Geometry cell centers from the geometry

**Rule:** the Image Geometry origin is the minimum corner of cell 0, and the center of cell `i` is
`origin + (i + 0.5) * spacing`. Get cell centers from `ImageGeom::getCoordsf()` or `getCoords()` instead of a local
formula. Boundary (corner) coordinates and cell-center coordinates need different formulas; name which one the code
computes.

**Example of the broken rule:** `ComputeShapes` computed its moments from corner coordinates (PR #1124). `FlyingEdges`
`getPosCube()`, the boundary coordinates of `WriteVtkRectilinearGrid` and the odd-size sampling of
`ComputeGBCDPoleFigure` Direct still use a local formula (issue #1878).

**Why:** a local formula that treats the origin as the center of cell 0, or that omits the half cell, shifts every
result by half a cell.

**What to look for:** `origin + index * spacing` used as a cell center; half the spacing subtracted from an origin
that is already a corner; integer half-dimension arithmetic.

**Pattern to use:** `ImageGeom::getCoordsf()`, as the corrected `ComputeShapes::findMoments()` uses it.

### Set the length unit of every geometry you create

**Rule:** a reader that creates a geometry sets its length unit from the file or from its **Length Unit** parameter. A
geometry whose coordinates are not lengths (orientation-space values, projection values, image pixels without a
physical scale) uses `IGeometry::LengthUnit::Unspecified`, and the filter documentation says why. Pass the unit to the
creation action explicitly.

**Example of the broken rule:** the Rodrigues geometry of `ConvertOrientationsToVertexGeometry` reported Meter, and the
pole-figure images of `WritePoleFigure` and `ComputeGBCDPoleFigure` reported Micrometer (issue #1803, PR #1838). The
NRRD path of `ReadImage`, `ReadNIfTIFile` and `ReadVolumeGraphicsFile` still drop the unit that the file or the
parameter gives (issue #1878).

**Why:** the coordinates can be correct while their unit metadata is false, and every later measurement inherits the
wrong unit.

**What to look for:** a geometry-creation action without a unit argument; a parsed unit that is never passed on.

**Pattern to use:** the explicit `LengthUnit::Unspecified` arguments in the three filters of PR #1838.

### Apply the hexagonal axis correction to both hexagonal Laue classes

**Rule:** the Oxford-to-EDAX hexagonal axis correction applies to `Hexagonal_Low` and `Hexagonal_High`. Express the
30-degree offset in the angular unit of the source data.

**Example of the broken rule:** **Read H5OINA Data** added 30 to angles in radians (PR #1728). **Read Oxford Instr.
Channel 5 (.cpr/.crc)** exposed the alignment option but never applied it, and the readers tested only
`Hexagonal_High` (PR #1781, which also fixed `ReadCtfData` and `ReadH5OinaData`).

**Why:** the a/a* axis convention belongs to the hexagonal lattice, not only to its high-symmetry Laue class.

**What to look for:** reader alignment code that tests one symmetry enum value; a literal `30` applied to radian data;
an alignment option that the algorithm never reads.

**Pattern to use:** `EbsdReaderUtilities::IsHexagonalCrystalStructure()` with `k_HexagonalAlignmentDegrees` or
`k_HexagonalAlignmentRadiansD`, as `ReadChannel5Data`, `ReadCtfData` and `ReadH5OinaData` use them.

### Delete only geometry support data created by the current invocation

**Rule:** before a geometry `find*` call, record which support objects already exist. Cleanup removes only
temporary objects created by this invocation, including on applicable error and cancellation exits. Reuse or
recalculation does not transfer ownership. Explicit deletion and documented invalidation contracts remain
separate.

**Example of the broken rule:** LaplacianSmoothing deleted supplied edges; FeatureFaceCurvature deleted supplied
vertex incidence; LabelTriangleGeometry and VerifyTriangleWinding deleted supplied incidence and element
neighbors. Evidence: #1861 (BUG: C01: Copy-paste index drift in 15 filters), PR #1919 (BUG: Fix copy-paste index
drift in 15 filters), and LabelTriangleGeometry's #1917 item M16 / #1908 item L41.

**Why:** `findElementsContainingVert(false)` and `findElementNeighbors(false)` can reuse input objects.
Unconditional cleanup then destroys data that existed before the filter ran.

**What to look for:**

- A geometry `find*` call followed by `delete*`, `removeData()`, or association clearing, without an entry-state
  ownership check. Check each support object separately, including dependencies created by the helper.

**Pattern to use:** the entry-state checks in `LaplacianSmoothing::operator()()`. Preserve supplied object IDs,
associations, and contents; test supplied and absent support separately across success and applicable failure
exits. Do not treat core recalculation-failure invalidation as temporary-object cleanup.

---

## Concurrency and storage

### Lock one mutex around a filter's static header cache

**Rule:** `preflightImpl()` and `executeImpl()` are `const`, so data that preflight reads (a file header) and execute
reuses lives in a file-level static cache keyed by an instance id. `IFilter::execute()` runs preflight on the same
instance just before `executeImpl()`, so a successful preflight leaves every entry that execute needs. The cache is
shared by every instance, and the GUI preflights on one thread while a pipeline executes on another: lock one mutex
around every access, in the constructor, the destructor, `preflightImpl()` and `executeImpl()`.

**Example of the broken rule:** `CropImageGeometry` changes its file-static `s_HeaderCache` without a mutex, and so do
the caches of `ReadBinaryCTNorthstar`, `ReadVolumeGraphicsFile`, `ReadCSVFile` and `SplitDataArrayByTuple` (issue
#1876).

**Why:** two threads that insert into or erase from one `std::map` at the same time corrupt it.

**What to look for:** a `static` map in a filter `.cpp` file that the constructor, destructor, preflight or execute
touch; an atomic instance counter with no lock for the map.

**Pattern to use:** `ReadNIfTIFileFilter`:

```cpp
std::mutex s_HeaderCacheMutex;

MyReaderFilter::MyReaderFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  std::lock_guard<std::mutex> lock(s_HeaderCacheMutex);
  s_HeaderCache[m_InstanceId] = {};
}
```

### Write out-of-core stores from one thread

**Rule:** out-of-core stores do not support concurrent writes. Threads that write different tuples, or even different
chunks, of the same store can still race.

- Parallelize the computation on in-memory buffers, join the workers, then write the results from one thread.
- When parallel writes matter for in-core speed, write separate in-core (parallel) and out-of-core (serial) algorithms
  and select one with the algorithm dispatch (`docs/AlgorithmDispatch.md`).
- A `ParallelDataAlgorithm` or `ParallelTaskAlgorithm` whose workers access a store passes every array they read or
  write to `requireArraysInMemory()` (or `requireStoresInMemory()`). That does not load data: it runs the algorithm
  serially when any listed array is out-of-core. Each call replaces the previous decision, so call it once with all
  arrays, and call `setParallelizationEnabled(false)` after it, not before.

**Example of the broken rule:** `ComputeAvgOrientations::computeVmfWatsonAverage()` ran workers that wrote the
per-feature output stores without a gate (PR #1645 made it serial). `ComputeFeatureFaceMisorientation` was fixed in
PR #1618. `CombineStlFiles` (issue #1876), `ConvertOrientations`, `PointSampleEdgeGeometry` and `FeatureFaceCurvature`
still launch workers that write shared stores without a gate.

**Why:** out-of-core stores do not support concurrent writes, and a race between two writers can lose values without
an error. Distinct tuple ranges do not make the writes safe.

**What to look for:** `ParallelDataAlgorithm`, `ParallelTaskAlgorithm`, `tbb::parallel_for` or `task_group` whose
workers call `setValue()`, `setComponent()`, `copyFromBuffer()` or a writable iterator on a store that no
`requireArraysInMemory()` call covers.

**Pattern to use:** compute in worker-owned buffers, join, then write serially; or gate the whole algorithm with
`requireArraysInMemory()` as `ComputeFZQuaternions` does.

### Check shape products before you allocate

**Rule:** the product of a tuple shape or a component shape can be too large for `usize`. Every store that computes a
count from a shape (`DataStore`, `EmptyDataStore`, `StringStore`, `ListStore`):

- looks for a zero dimension first (a shape with a zero dimension has a count of 0);
- checks each multiplication for overflow, then the element count and the byte count;
- makes all checks before it allocates memory or changes a member.

`resizeTuples()` returns an error and keeps the store unchanged (-6035 for `DataStore`, `StringStore` and `ListStore`,
`EmptyDataStore::k_ResizeError` -10603 for `EmptyDataStore`). A constructor throws. Each overflow message names the
count that does not fit ("tuple count", "element count" or "byte count").

**Example of the broken rule:** `DataStore` resize multiplied shapes without overflow checks; for `{2^63, 2}` the count
wrapped to 0 and the store kept a buffer that did not match its shape (PR #1834). The `DataStore` constructors still
accept an overflowing tuple shape (issue #1883), and `StringStore` and `ListStore` erase their data when the product
overflows (issue #1884).

**Why:** a wrapped count allocates a buffer that is too small, and every later index is out of bounds.

**What to look for:** `std::accumulate(..., std::multiplies<>())` over a shape, or `tuples * components * sizeof(T)`,
before an allocation or a member change.

**Pattern to use:** `EmptyDataStore::CheckedPlaceholderProduct()`; the checked `DataStore::resizeTuples()`.

---

## Error codes and messages

### Give every condition its own error code

**Rule:** each error condition gets its own error code and each warning condition its own warning code; do not copy
the code of the check above. Filter codes are more negative than -10000, and each filter uses one range of 100 codes.
Shared utilities use codes that no filter uses. Declare each code as a named constant and use the same constant in
preflight and execute for the same condition. Before you use a new code, check that it is unique in the repository and
in open pull requests (`git grep -nE -- "-47512([^0-9]|$)"`; do not rely on `\b`).

**Example of the broken rule:** `AppendImageGeometry` uses -8201 for separate Y and Z mismatches, `ColorToGrayScale`
reuses -10704, `CreateAttributeMatrixAction` reuses -5205 and `RenameDataAction` reuses -6601 (issue #1872, which lists
23 filters). PR #1767 corrected colliding codes in QuickSurfaceMesh Scanline.

**Why:** a shared code cannot tell the user, a test or a support engineer which check fired.

**What to look for:** the same numeric literal in two `MakeErrorResult` calls; a code copied with its surrounding
block.

**Pattern to use:** `constexpr int32 k_ZDimMismatch = -47512;` per condition, checked for uniqueness before use.

### Name the right values in each message, and test the mask first

**Rule:** each error and warning message names the correct values in the correct order, fires only on the condition it
describes, and gives advice the user can act on (wording by the `bluequartz-skills:error-messages` skill). A per-cell
loop tests the mask first: cells that the mask excludes are not validated, counted or grown.

**Example of the broken rule:** `ComputeFZQuaternions` counts out-of-range phases also for masked-out cells and returns
error -49008 for cells it never uses, so a valid input fails; `ReadEnsembleInfo` swaps the key and the phase group in
its message (issue
#1880, which lists wrong or misleading diagnostics in 10 filters).

**Why:** a message with swapped values sends the user to fix the wrong thing, and an error about excluded cells
fails a run that has no problem.

**What to look for:** `fmt::format` arguments in a different order from the placeholders' meaning; validation or
counting before the mask check.

**Pattern to use:** check eligibility (mask, phase) first, as the negative-phase branch of `ComputeFZQuaternions` does.

---

## Legacy pipeline compatibility

### Convert an optional legacy SIMPL key only when it is present

**Rule:** in a `FromSIMPLJson()` conversion, keep the default when an optional key that later SIMPL versions added is
absent. When the key is present, convert it and keep its errors.

**Example of the broken rule:** older pipelines without the later ignore-list keys failed to convert in
`ErodeDilateBadData`, `FillBadData`, `RequireMinNumNeighbors` and `NeighborOrientationCorrelation`, and without the
randomization key in `CAxisSegmentFeatures` (PR #1682).

**Why:** `SIMPLConversion::ConvertParameter()` returns error -571 for a missing key. Discarding all errors instead also
hides a malformed value that the file does supply.

**What to look for:** a converter that sets defaults and then converts optional keys unconditionally, or that keeps a
result only when it is valid.

**Pattern to use:** `ErodeDilateBadDataFilter::FromSIMPLJson()`:

```cpp
// IgnoredDataArrayPaths was added to the legacy filter after 6.5.49; older pipelines omit the key.
if(json.contains(SIMPL::k_IgnoredDataArrayPathsKey))
{
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(
      args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));
}
```

### Keep the legacy conversions when a filter moves or a plugin is retired

**Rule:** every legacy SIMPL filter UUID has one conversion owner; several legacy UUIDs can map to the same current
filter. Each mapping has a fixture with non-default values, and its test checks the target filter UUID and the
converted arguments. When a filter or plugin is retired, move its conversions to the replacement first.

**Example of the broken rule:** retiring the ITKImageProcessing plugin dropped its 59 legacy SIMPL filter mappings
(DiscreteGaussian, ReadImage, WriteImage, ReadImageStack, Median, RescaleIntensity, the morphology filters, ...), and no
test failed. PR #1752 restored them in `ImageProcessingLegacyUUIDMapping.hpp` and added the coverage test.

**Why:** a missing mapping stops an old pipeline from converting, and a conversion that produces only the defaults loses
every saved value without an error.

**What to look for:** a change that removes or moves a filter or plugin without moving its conversion map and fixtures.

**Pattern to use:** `src/Plugins/NXImageProcessing/test/SimplConversionCoverageTest.cpp`, which reads the live map,
requires a fixture for each entry, checks the target UUID and rejects a conversion that produces only defaults.

---

## Testing

### Take expected values from an independent oracle

**Rule:** expected values come from a hand calculation, a closed-form result, a reference implementation or verified
DREAM3D 6.5.171 output. Never regenerate an exemplar only to absorb a change in output; show with the oracle that the
new value is correct.

**Example of the broken rule:** symmetric fixtures and exemplars hid wrong surface weights and a wrong sphericity
formula in **Compute Surface Area to Volume** (PR #1817). The exemplars of **Read EDAX EBSD Data (.ang)** and **Read
H5OINA Data** were generated by the code under test and were replaced in PRs #1657 and #1728. The
`ComputeArrayHistogramByFeature` test encodes the same wrong modal bounds as the code (issue #1861).

**Why:** an expected value produced by the implementation passes whatever the implementation does.

**What to look for:** an exemplar updated in the same change as the algorithm; expected values that repeat the code's
own indexing.

**Pattern to use:** the analytical box geometries and scale-invariance cases in
`src/Plugins/SimplnxCore/test/ComputeSurfaceAreaToVolumeTest.cpp`.

### Test every axis and plane

**Rule:** code with one block per axis or plane gets a correctness case for each axis or plane, with unequal dimensions
and spacings along X, Y and Z so that a swapped axis changes the result.

**Example of the broken rule:** symmetric fixtures missed swapped X and Y face weights in **Compute Surface Area to
Volume** (PR #1817, issues #1773 and #1780). `ComputeShapesFilterTest` has no valid YZ or XZ expected output, and issue
#1861 lists copy-paste index drift between axis blocks in 15 filters.

#1861 (BUG: C01: Copy-paste index drift in 15 filters) and PR #1919 (BUG: Fix copy-paste index drift in 15
filters) also document wrong sibling operands outside axis branches:
`RemoveFlaggedTrianglesFilter::preflightImpl()` selected the Edge Attribute Matrix instead of Face Data; `Ray.hpp`
used the wrong ray origin; `GridMontage` confused row, column, and plane operands; `Vec7::convertType()`
constructed a `Vec6`; and `MMCellMap::getEdgeLabels()` selected the wrong second endpoint.

**Why:** code with one block per axis is usually written by copying one block; a block with a wrong index passes a test
of the first axis.

**What to look for:** separate axis or plane branches with correctness fixtures for only one of them; cubic dimensions
or equal spacings in a test of an axis-dependent formula.

- Homologous expressions whose indices, source objects, component counts, or endpoints do not match the branch
  they serve. Apply this recognition check to components, populations, endpoints, and named objects as well as
  axes. Give siblings distinct fixture values or names and check each affected result against an independent
  oracle; equal values can hide the substitution.

**Pattern to use:** the generated pair-axis cases with spacing `{2, 3, 5}` in `ComputeSurfaceAreaToVolumeTest.cpp`.

### Assert a valid Result with SIMPLNX_RESULT_REQUIRE_VALID

**Rule:** assert a valid `Result` with `SIMPLNX_RESULT_REQUIRE_VALID(result)`, and an invalid one with
`SIMPLNX_RESULT_REQUIRE_INVALID`, instead of `REQUIRE(result.valid())`. The macro prints each error code and message
when the check fails.

**Example of the broken rule:** `test/DataArrayTest.cpp`, `test/GeometryTest.cpp` and the NXImageProcessing
`ContourEngineTest.cpp` use `REQUIRE(result.valid())`.

**Why:** a bare `REQUIRE` reports only `false`; the failure log does not say which error the code returned.

**What to look for:** `REQUIRE(...valid())` or `REQUIRE_FALSE(...invalid())`.

**Pattern to use:** `SIMPLNX_RESULT_REQUIRE_VALID(result);`

### Test core code in the core test target with the simplnx tag

**Rule:** a test of code in `src/simplnx/` goes in the core `test/` directory and carries the `[simplnx]` tag. Core code
does not depend on an optional plugin's tests for its only coverage, and each new exported helper gets its own direct
test.

**Example of the broken rule:** `test/StorageFormatPlanTest.cpp`, `test/Dream3dPreflightCacheTest.cpp` and
`test/TemporaryRecordStoreTest.cpp` have no `[simplnx]` tag.

**Why:** a core test without its tag is left out when the core tests are selected by tag, and a helper covered only
through a plugin loses its coverage when that plugin is off.

**What to look for:** a `TEST_CASE` in `test/` whose tags do not include `[simplnx]`; a new exported helper with no test
of its own.

**Pattern to use:** the `CreateRectGridGeometryAction` and "units and cloning" cases in `test/GeometryTest.cpp`.

### Name a filter test after its plugin

**Rule:** a filter test name starts with the plugin name, and its first tag is the plugin:
`TEST_CASE("NXImageProcessing::AbsImageFilter: ...", "[NXImageProcessing][AbsImageFilter]")`. When a filter moves to another
plugin, change both.

**Example of the broken rule:** `ComputeShapesFilterTest.cpp` in OrientationAnalysis still has the first tag
`[SimplnxCore]`; `ComputeBoundaryCellsTest.cpp` has no plugin tag; `FillBadDataTest.cpp` uses `[Core]`.

**Why:** tests are selected and reported by plugin; a test with another plugin's tag runs or is skipped with the wrong
set.

**What to look for:** a first tag that is not the plugin of the test file; a former plugin name in a test name.

**Pattern to use:** `ComputeSurfaceAreaToVolumeTest.cpp` in SimplnxCore.

### Call UnitTest::LoadPlugins() only when the test looks up filters by UUID

**Rule:** a test calls `UnitTest::LoadPlugins();` only when it calls `createFilter()` on the filter list, loads a
pipeline (`Pipeline::FromJson()`, `FromFile()`, `FromSIMPLJson()`, `FromSIMPLFile()`; every SIMPL
backwards-compatibility test does), or runs a filter whose code gets other filters from the filter list (`ReadH5Ebsd`
does). Each test that needs plugin loading calls it itself. CTest runs each visible, individually registered
`TEST_CASE` in its own process. A few hidden groups, such as `[.OocStoreContract]`, run together in one process.
`UnitTest::PreferencesSentinel` needs an existing `Application` instance. If the test only needs that instance,
call `Application::GetOrCreateInstance()` before constructing the sentinel.

**Example of the broken rule:** the synthetic tuple-mismatch cases of `ComputeAvgCAxesTest.cpp` and
`ReplaceElementAttributesWithNeighborValuesTest.cpp` load the plugins although they construct the filter directly, and
`UnitTestCommon`'s `LoadDataStructure()` loads them for every caller.

**Why:** the call loads every plugin library and fails the test when one does not load, so an unneeded call makes a
test fail for a plugin it does not use.

**What to look for:** `UnitTest::LoadPlugins()` in a test with no UUID lookup, pipeline load, registry-dependent
subfilter, or other need for plugin registration. A `PreferencesSentinel` constructed before the `Application`
instance exists dereferences a null pointer (PR #1915).

**Pattern to use:** construct filters directly as in `test/GeometryTest.cpp`. For a test that also uses a sentinel,
initialize the `Application` directly as in the projection warning cases in `MaximumProjectionImageFilterTest.cpp`.

### Give Catch2 a bool when you compare file times

**Rule:** in `CHECK` and `REQUIRE`, put an extra pair of parentheses around a comparison of
`std::filesystem::file_time_type` values or of file-clock durations, so that Catch2 gets one `bool`.

**Example of the broken rule:** `test/SelectiveImportReferencesTest.cpp` and `test/PreferencesSizeGBTest.cpp` did not
compile on the macOS CI jobs (fixed in PR #1764).

**Why:** Catch2 splits `a == b` into its operands and compiles code that converts each to text. For a file-clock value,
libc++ formats a floating-point number, which needs `std::to_chars` from macOS 13.3. The macOS CI builds of DREAM3D-NX
set the deployment target to macOS 11.7, so the test does not compile there, while a local build with a newer target
does.

**What to look for:**

- A `CHECK` or `REQUIRE` whose operand is `fs::last_write_time(...)`, a `file_time_type` or a `clock::now() - ...`
  duration.
- The compile error `'to_chars' is unavailable: introduced in macOS 13.3`.

**Pattern to use:**

```cpp
// Evaluate to bool before Catch2 tries to stream libc++'s file-clock time point.
CHECK((fs::last_write_time(files.input) == modifiedBefore));
```

### Never put a semicolon in a Catch2 test case name

**Rule:** do not use `;` in the name of a Catch2 test case that `catch_discover_tests()` registers with CTest. Use a
comma or a dash. This applies to simplnx, its plugins, SimplnxOoc and DREAM3D-NX.

**Example of the broken rule:** seven NXImageProcessing test cases, in `BinaryMorphologicalClosingImageFilterTest` (two),
`ContourEngineTest`, `GrayscaleMorphologicalOpeningImageFilterTest`, `GrayscaleMorphologicalClosingImageFilterTest`,
`WhiteTopHatImageFilterTest` and `BlackTopHatImageFilterTest`, never ran under CTest, while CTest reported their partial
entries as passed (issue #1898; PR #1889 renamed them).

**Why:** discovery stores the test names in a CMake list, and `;` separates list items. A name with one semicolon
becomes two CTest entries; Catch2 finds no case for either part, prints `No test cases matched` and exits with 0.

**What to look for:**

- A `;` in the first argument of `TEST_CASE`, `SCENARIO` or `TEMPLATE_TEST_CASE`, or in the second argument of
  `TEST_CASE_METHOD`.
- A CTest entry whose name starts or ends in the middle of a phrase, with the output `No test cases matched`.

**Pattern to use:**

```cpp
TEST_CASE("NXImageProcessing::BlackTopHatImageFilter: Computed-expected (safeBorder=false, Annulus + Box zero-radius)",
          "[NXImageProcessing][BlackTopHatImageFilter]")
```

---

## Entry template

Copy this block for a new entry, put it in the section it belongs to, and add it to the index.

```markdown
### <The rule, stated as an instruction>

**Rule:** what to do and what not to do.

**Example of the broken rule:** what broke, where (file and function, filter or test), and the pull requests or
issues. Name the places: an entry needs a rule that was broken in many places.

**Why:** the mechanism that makes the mistake fail.

**What to look for:**

- the code shape, call or message that gives it away.

**Pattern to use:** a short code example, or the file and function that already does it right.
```
