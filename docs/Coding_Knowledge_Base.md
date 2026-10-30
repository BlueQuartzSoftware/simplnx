# Coding Knowledge Base

This file collects lessons learned from real defects in simplnx and its plugins. Each entry records a mistake that was
made, why it happened, how it was found, and the rule that prevents it. The audience is developers and the coding
agents that work in this repository. Read the rules before you write or review code in the area they cover.

Add or update an entry when a defect taught a rule, as "Where new rules go" in `AGENTS.md` describes. Keep the entries
short. State the rule first, then the evidence. Cite pull requests and issues by number, but do not say whether they are
open, merged or closed: that changes, and the entry would go stale. Refer to code by file and function or test name, not
by line number, because line numbers change with every edit; a line number belongs only in a link that is pinned to a
commit. A project convention that does not come from a defect belongs in [the code style guide](Code_Style_Guide.md)
instead.

## Index

1. [Record an object in the writer ID map only when the written DataStructure owns it][kb-1]
2. [Check the HDF5 file close before you report a successful save][kb-2]
3. [Copy every field that sets behavior when you clone an object][kb-3]
4. [Do not treat the type list of a selection parameter as proof of a numeric DataArray][kb-4]
5. [Return null from a geometry pointer getter when the geometry is detached][kb-5]
6. [Do not mix a circular mean with an unwrapped arithmetic centroid][kb-6]
7. [Use the same population in every pass of a statistic](#7-use-the-same-population-in-every-pass-of-a-statistic)
8. [Size storage for sparse IDs from the largest ID](#8-size-storage-for-sparse-ids-from-the-largest-id)
9. [Do not reject a generated mesh in preflight for its placeholder size][kb-9]
10. [Open an HDF5 child by its target name below the parent group][kb-10]
11. [Apply a geometry transformation once when a filter runs a subfilter][kb-11]
12. [Give all ghost cells one label](#12-give-all-ghost-cells-one-label)
13. [Compute a shared intersection point from a canonical endpoint order][kb-13]
14. [Key a clipped vertex by its source vertex and its clip point][kb-14]
15. [Convert an optional legacy SIMPL key only when it is present][kb-15]
16. [Check the binary STL file size before you use attribute byte counts][kb-16]
17. [Give a geometry without physical coordinates an unspecified length unit][kb-17]
18. [Apply the hexagonal axis correction to both hexagonal Laue classes][kb-18]
19. [Use the search grid only to find neighbor candidates](#19-use-the-search-grid-only-to-find-neighbor-candidates)
20. [Validate an in-place array by the object ID of its Attribute Matrix, not by its path][kb-20]
21. [Keep the legacy conversions when a filter moves or a plugin is retired][kb-21]
22. [Set a distinct modification time in a file-change test][kb-22]
23. [Test geometry formulas with unequal spacing along each axis][kb-23]
24. [Test a transfer failure after an earlier batch succeeded][kb-24]
25. [Give Catch2 a bool when you compare file times][kb-25]

[kb-1]: #1-record-an-object-in-the-writer-id-map-only-when-the-written-datastructure-owns-it
[kb-2]: #2-check-the-hdf5-file-close-before-you-report-a-successful-save
[kb-3]: #3-copy-every-field-that-sets-behavior-when-you-clone-an-object
[kb-4]: #4-do-not-treat-the-type-list-of-a-selection-parameter-as-proof-of-a-numeric-dataarray
[kb-5]: #5-return-null-from-a-geometry-pointer-getter-when-the-geometry-is-detached
[kb-6]: #6-do-not-mix-a-circular-mean-with-an-unwrapped-arithmetic-centroid
[kb-9]: #9-do-not-reject-a-generated-mesh-in-preflight-for-its-placeholder-size
[kb-10]: #10-open-an-hdf5-child-by-its-target-name-below-the-parent-group
[kb-11]: #11-apply-a-geometry-transformation-once-when-a-filter-runs-a-subfilter
[kb-13]: #13-compute-a-shared-intersection-point-from-a-canonical-endpoint-order
[kb-14]: #14-key-a-clipped-vertex-by-its-source-vertex-and-its-clip-point
[kb-15]: #15-convert-an-optional-legacy-simpl-key-only-when-it-is-present
[kb-16]: #16-check-the-binary-stl-file-size-before-you-use-attribute-byte-counts
[kb-17]: #17-give-a-geometry-without-physical-coordinates-an-unspecified-length-unit
[kb-18]: #18-apply-the-hexagonal-axis-correction-to-both-hexagonal-laue-classes
[kb-20]: #20-validate-an-in-place-array-by-the-object-id-of-its-attribute-matrix-not-by-its-path
[kb-21]: #21-keep-the-legacy-conversions-when-a-filter-moves-or-a-plugin-is-retired
[kb-22]: #22-set-a-distinct-modification-time-in-a-file-change-test
[kb-23]: #23-test-geometry-formulas-with-unequal-spacing-along-each-axis
[kb-24]: #24-test-a-transfer-failure-after-an-earlier-batch-succeeded
[kb-25]: #25-give-catch2-a-bool-when-you-compare-file-times

---

## 1. Record an object in the writer ID map only when the written DataStructure owns it

**Rule:** `DataStructureWriter` maps each written object ID to its HDF5 path, so that a second location of the same
object becomes a link. Record an object in that map only when it belongs to the `DataStructure` that is being written.
Return an error when an ID is already recorded at a different path; do not overwrite the entry.

**Pattern to use:** `DataStructureWriter::addWriter()` in pull request #1859 ignores objects outside
`m_WrittenStructure`, obtains the object ID and writer path, and inserts them with `m_IdMap.try_emplace()`. If the ID
already maps to a different path, it returns error -6055 instead of replacing that mapping.

**Pattern to avoid:** send the objects of a temporary `DataStructure` through the writer of the main structure, and
record them by their numeric ID.

**Why:** an ID is unique only inside one `DataStructure`. Every `DataStructure` starts its IDs at 1, so a temporary
structure and the written structure can both hold an object with the same ID.

**How it showed up:** the NeighborList writer sends a temporary counts array through the main writer. That array
replaced the map entry of an unrelated object that had two locations, and the second location became a link to the
counts dataset. Issue #1831. Fixed by PR #1859.

**How to recognize it next time:**

- A serializer creates a temporary `DataStructure` and writes its objects through the writer of the main structure.
- An ID map that is filled with `operator[]` or `insert_or_assign()` instead of a checked insert.

---

## 2. Check the HDF5 file close before you report a successful save

**Rule:** a writer that owns an HDF5 file closes it explicitly and checks the result before it returns success. A close
failure is returned before `AtomicFile::commit()` replaces the old file.

**Pattern to use:** `FileIO::closeChecked()` in pull request #1859. For example, the path-based write of
`DataStructureWriter` ends with `return MergeResults(std::move(writeResult), fileWriter.closeChecked());`.

**Pattern to avoid:** return success while the `FileIO` object is still in scope, so that its destructor closes the
file and discards the result of `H5Fclose()`.

**Why:** HDF5 can write buffered metadata when it closes the file. Successful dataset writes do not prove that the file
is complete.

**How it showed up:** an injected close failure still gave a valid `Result`, so **Write DREAM3D-NX File** could replace
the old output file with an incomplete one. Issue #1832. Fixed by PR #1859.

**How to recognize it next time:**

- A path-based writer returns while its file object is still in scope, and an atomic-file commit follows.

---

## 3. Copy every field that sets behavior when you clone an object

**Rule:** the `clone()` of an action or a parameter keeps all of its configuration. When a constructor gets a new
argument, update `clone()` in the same change, and test the clone with a value that is not the default.

**Pattern to use:** `CreateImageGeometryAction::clone()` passes the length unit:

```cpp
return std::make_unique<CreateImageGeometryAction>(getCreatedPath(), m_Dims, m_Origin, m_Spacing, m_CellDataName,
                                                   m_Units);
```

Test a clone by applying the original and the clone to separate `DataStructure` objects and comparing the results.

**Pattern to avoid:** a constructor call in `clone()` that leaves out an argument, so the clone silently takes the
default value.

**Why:** the framework copies actions and parameters through `clone()` (see `AnyCloneable`) without a visible call at
the filter. A field that the copy loses changes the output with no error.

**How it showed up:** PR #1838 added length units to geometry creation, but a cloned image-geometry action reverted to
Micrometer. The review of that pull request asked for a direct clone test.

**How to recognize it next time:**

- A constructor gets a new argument and `clone()` is not part of the change.

---

## 4. Do not treat the type list of a selection parameter as proof of a numeric DataArray

**Rule:** distinguish array kind from numeric element type. `ArraySelectionParameter` accepts any `IArray`, but applies
its numeric-type and component-shape restrictions only to `IDataArray`. `MultiArraySelectionParameter` checks
`AllowedTypes` and required component shapes, but does not check `AllowedDataTypes`. Use `getDataRefAs<T>()` only when
parameter validation or an earlier check establishes the required type. Otherwise use `getDataAs<T>()` and reject
`nullptr`.

**Pattern to use:** `ComputeSurfaceAreaToVolumeFilter::preflightImpl()` gets its inputs with
`getDataAs<Int32Array>()` and checks each pointer. A check that needs only the tuple count, such as a mask-size check,
reads the selection as `IArray` (`DBSCANFilter` does). A `StringArray` that the parameter accepted then gets the
tuple-count error instead of a `std::bad_cast`. Reading it as `IArray` does not make it a numeric array for execute.

**Pattern to avoid:** rely on the numeric-type list of `ArraySelectionParameter` to establish `IDataArray`, or on
`MultiArraySelectionParameter::AllowedDataTypes` to establish a concrete element type.

**Why:** `getDataRefAs<T>()` throws `std::bad_cast` for an object of a different type, and `IFilter::preflight()` does
not convert exceptions into errors.

**How it showed up:** selections that the parameters accepted caused `std::bad_cast` in preflight, or reached a Boolean
type that the algorithm dispatch did not support. Issues #1821 and #1833. PRs #1817 and #1763 fixed the affected
filters. PR #1766 tightens the parameter declarations and preflight checks of individual filters; it does not fix
the shared parameter-validation gaps.

**How to recognize it next time:**

- A typed reference access in preflight that relies only on the type list of a selection parameter.

---

## 5. Return null from a geometry pointer getter when the geometry is detached

**Rule:** a geometry getter that returns a pointer checks the geometry's `DataStructure` pointer before it resolves an
associated object, and returns `nullptr` when the geometry is detached.

**Pattern to use:** `IGridGeometry::getCellData()`:

```cpp
DataStructure* dataStructurePtr = getDataStructure();
if(nullptr == dataStructurePtr)
{
  return nullptr;
}
return dataStructurePtr->getDataAs<AttributeMatrix>(m_CellDataId);
```

**Pattern to avoid:** dereference `getDataStructure()`, or use a throwing reference accessor, inside a pointer getter.

**Why:** an external `shared_ptr` can keep a geometry alive after its `DataStructure` is destroyed. The destructor of
the `DataStructure` detaches the objects that survive it.

**How it showed up:** a geometry getter that ran during application shutdown threw and ended the application. PR #1809.

**How to recognize it next time:**

- A geometry that something outside the `DataStructure` owns while the structure is cleared or destroyed.

---

## 6. Do not mix a circular mean with an unwrapped arithmetic centroid

**Rule:** state which periodic centroid the code computes, and make the code and the filter documentation agree. For an
unwrapped arithmetic centroid, choose the seam (the largest gap), unwrap the positions, keep their weights, average
them, then wrap the result back into the domain.

**Pattern to use:** the file-local `PeriodicMean1D()` in `src/simplnx/Utilities/GeometryHelpers.cpp` unwraps vertex
positions at the largest gap.

**Pattern to avoid:** add half of the domain to an ordinary mean, or compute `atan2(sumSin, sumCos)` and document the
result as an arithmetic average after unwrapping.

**Why:** the circular mean and the unwrapped arithmetic mean are different for an asymmetric population.

**How it showed up:** PR #1670 (issue #1665) removed constant-offset corrections from the triangle and feature centroid
filters. The arithmetic centroid for cells in `ComputeFeatureCentroids` is fixed by PR #1764.

**How to recognize it next time:**

- Periodic centroid code with a constant offset or `atan2(sumSin, sumCos)`, while the documentation promises an
  arithmetic average.

---

## 7. Use the same population in every pass of a statistic

**Rule:** counting, gathering, summing and variance passes use the same eligibility test. Reset the denominator of each
group before the code processes that group.

**Pattern to use:** the counting and gather passes in `ComputeAvgOrientations` both require a positive phase smaller
than `numEnsembles`. The gather pass also requires the voxel's feature ID to match the feature being averaged.

**Pattern to avoid:** exclude phase-0 cells from the count but include their quaternion, or a placeholder value, in
another pass.

**Why:** the numerator, the denominator and the deviations must describe the same population.

**How it showed up:** PR #1664 (issue #1659) stopped phase-0 quaternions from entering the vMF and Watson averages in
**Compute Feature Average Orientations**. PR #1631 made the neighbor-average denominators of
`ComputeFeatureNeighborMisorientations` and `ComputeFeatureNeighborCAxisMisalignments` local to each feature.

**How to recognize it next time:**

- A statistic with several passes whose `if` conditions are different.

---

## 8. Size storage for sparse IDs from the largest ID

**Rule:** when external IDs can have gaps, size the storage for the largest ID, or map the IDs explicitly to dense
indices. Do not size it from the number of distinct IDs.

**Pattern to use:** `ReadAngDataFilter::preflightImpl()` sizes the ensemble Attribute Matrix from the largest phase
index:

```cpp
// Size ensemble arrays from the largest phase index because phase indices can have gaps.
tupleDims = {std::max(maxPhaseIndex, angPhases.size()) + 1};
```

`SilhouetteDirect` maps sparse cluster IDs to dense indices with a `std::unordered_map<int32, usize>`.

**Pattern to avoid:** allocate `numberOfDistinctIds + 1` entries and index them with the unchanged external IDs.

**Why:** two declared phases can have the IDs 1 and 5.

**How it showed up:** PR #1657 fixed out-of-bounds ensemble writes for ANG files with sparse phase IDs. PR #1765 fixed
sparse cluster IDs in **Silhouette**.

**How to recognize it next time:**

- The allocation uses the `size()` of a container, but the writes use IDs read from its elements.

---

## 9. Do not reject a generated mesh in preflight for its placeholder size

**Rule:** when an upstream filter computes the size of a mesh from the data, accept the placeholder size in preflight.
Check for an empty mesh in execute.

**Pattern to use:** `SliceTriangleGeometryFilter` accepts the placeholder in preflight. In `executeImpl()` it adds
warning -62105 when the Triangle Geometry has no triangles.

**Pattern to avoid:** a preflight check that rejects every geometry with zero faces.

**Why:** mesh-producing filters such as `QuickSurfaceMeshFilter` create their geometry in preflight with zero faces,
because they compute the real mesh in execute.

**How it showed up:** the review of PR #1763 found that a new empty-mesh check broke pipelines that use **Create
Surface Mesh (QuickMesh)**, **Create Surface Mesh (Surface Nets)** and M3C before they ran.

**How to recognize it next time:**

- A new preflight check that rejects a count that an upstream action sets to 0 on purpose.

---

## 10. Open an HDF5 child by its target name below the parent group

**Rule:** when a reader receives the parent group of an object, open the object by its target name.

**Pattern to use:** `INodeGeom0dIO` and `RectGridGeomIO`:

```cpp
auto groupReader = dataStructureGroup.openGroup(dataPath.getTargetName());
```

**Pattern to avoid:** open the complete `DataPath` below a group that already is the parent of the object.

**Why:** for an object at the root, the two forms give the same path, so the mistake stays hidden. For a nested object,
the path repeats the parent and does not resolve.

**How it showed up:** importing a geometry inside a `DataGroup` crashed. PR #1676.

**How to recognize it next time:**

- A helper that takes both a parent HDF5 group and a complete `DataPath`.

---

## 11. Apply a geometry transformation once when a filter runs a subfilter

**Rule:** before a filter runs a subfilter, take into account the geometry changes that its own preflight actions have
already applied.

**Pattern to use:** `ReadH5Ebsd` sets `keep_input_geometry_origin` when it runs **Rotate Sample Reference Frame**,
because preflight already assigned the transformed origin through `UpdateImageGeomAction`.

**Pattern to avoid:** transform the origin in an outer action, then let the subfilter transform that origin again.

**Why:** `IFilter::execute()` applies the preflight actions before it calls `executeImpl()`, so the subfilter sees their
result.

**How it showed up:** the automatic sample rotation of **Read H5EBSD File** rotated the origin twice, and the equivalent
manual pipeline gave a different origin. PR #1836, issue #1788.

**How to recognize it next time:**

- An outer preflight writes transformed metadata, and execute runs a filter that transforms the same metadata.

---

## 12. Give all ghost cells one label

**Rule:** give one exterior region one label. Keep metadata about where a boundary cell is in separate storage.

**Pattern to use:** `M3CSurfaceMeshing` writes `k_GhostLabel` to every cell of the ghost shell.

**Pattern to avoid:** different sentinel labels that record which side of the box a ghost cell is on.

**Why:** the mesher treats two different labels as a material interface.

**How it showed up:** six ghost sentinels made false interfaces inside the shell, and M3C made triangles outside the
volume. PR #1718.

**How to recognize it next time:**

- Sentinel values that encode both material identity and location.

---

## 13. Compute a shared intersection point from a canonical endpoint order

**Rule:** every calculation of the same intersection point uses the same endpoint order, and sets the constrained
coordinate exactly.

**Pattern to use:** `IntersectTriangleWithPlane()` in `src/simplnx/Utilities/GeometryUtilities.cpp`:

```cpp
// Interpolate from the lower-z endpoint so shared edges produce identical points.
if(s[2] > en[2])
{
  std::swap(s, en);
  std::swap(d1, d2);
}
const float t = d1 / (d1 - d2);
Point3Df p = s + (en - s) * t;
p[2] = planeZ;
```

**Pattern to avoid:** let the winding of each triangle choose the interpolation order.

**Why:** floating-point calculations that are equal in algebra can give different results. Vertex deduplication with
exact equality then leaves two vertices where there should be one.

**How it showed up:** **Slice Triangle Geometry** made disconnected outlines along edges that two triangles share. PR
#1763.

**How to recognize it next time:**

- Adjacent primitives that compute a shared point independently, followed by a merge with exact equality.

---

## 14. Key a clipped vertex by its source vertex and its clip point

**Rule:** one source vertex can give several output vertices when an operation clips. Include the intersection point
in the key of a clipped vertex, and keep a separate output-to-source map for the copied attributes.

**Pattern to use:** `CropEdgeGeometry` keys its clipped vertices with
`std::map<std::pair<uint64, ClipPoint>, uint64>`, and copies vertex attributes through a separate source map.

**Pattern to avoid:** cache one clip point for each source vertex outside the crop box.

**Why:** two edges from the same outside vertex can cross the crop boundary at different points.

**How it showed up:** in **Crop Geometry (Edge)**, two crossing edges shared one substituted end point, which moved one
edge off its original line. PR #1765.

**How to recognize it next time:**

- A topology-changing operation that assumes one output vertex for each source vertex.

---

## 15. Convert an optional legacy SIMPL key only when it is present

**Rule:** in a `FromSIMPLJson()` conversion, keep the default when an optional key that later SIMPL versions added is
absent. When the key is present, convert it and keep its errors.

**Pattern to use:** `ErodeDilateBadDataFilter::FromSIMPLJson()`:

```cpp
// IgnoredDataArrayPaths was added to the legacy filter after 6.5.49; older pipelines omit the key.
if(json.contains(SIMPL::k_IgnoredDataArrayPathsKey))
{
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(
      args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));
}
```

**Pattern to avoid:** expect a default argument to prevent a missing-key error, or discard every failed conversion
result.

**Why:** `SIMPLConversion::ConvertParameter()` returns error -571 for a missing key. Discarding all errors also hides a
malformed value that the file does supply.

**How it showed up:** older pipelines that did not have the later ignore-list and randomization keys failed to convert.
PR #1682.

**How to recognize it next time:**

- A converter that sets defaults and then converts optional keys unconditionally, or that adds a result only when it is
  valid.

---

## 16. Check the binary STL file size before you use attribute byte counts

**Rule:** before you use the attribute field of a binary STL triangle as a payload length, check that the file has
room for attribute payloads. Keep the overrides for known vendors.

**Pattern to use:** `StlUtilities::SanityCheckFile()` compares the file size with the size of the fixed records and
excludes Magics and VXelements files. `ReadStlFile` checks its result before it seeks.

**Pattern to avoid:** decide the layout of the whole file from triangle 0, or always seek by the attribute field.

**Why:** some writers store color bits or random data in that field without writing payload bytes.

**How it showed up:** an uncolored first triangle hid a Magics color encoding, and later triangles caused invalid seeks
and a corrupt geometry in **Read STL File**. PR #1696.

**How to recognize it next time:**

- A format field that vendors interpret in different ways, and a heuristic that samples only the first record.

---

## 17. Give a geometry without physical coordinates an unspecified length unit

**Rule:** pass `IGeometry::LengthUnit::Unspecified` when the coordinates of a geometry are orientation-space values,
projection values or image pixels without a physical scale.

**Pattern to use:** the geometry actions in `ConvertOrientationsToVertexGeometryFilter`,
`ComputeGBCDPoleFigureFilter` and `WritePoleFigureFilter` pass the unit explicitly.

**Pattern to avoid:** accept the physical default unit of a geometry action for coordinates that are not lengths.

**Why:** the numbers can be correct while their unit metadata is false.

**How it showed up:** the Rodrigues geometry reported Meter, and pole-figure images reported Micrometer. PR #1838,
issue #1803.

**How to recognize it next time:**

- A plotting or orientation filter that creates a geometry without a unit argument.

---

## 18. Apply the hexagonal axis correction to both hexagonal Laue classes

**Rule:** the Oxford-to-EDAX hexagonal axis correction applies to `Hexagonal_Low` and `Hexagonal_High`. Express the 30
degree offset in the angular unit of the source data.

**Pattern to use:** `EbsdReaderUtilities::IsHexagonalCrystalStructure()` with `k_HexagonalAlignmentDegrees` or
`k_HexagonalAlignmentRadiansD`. `ReadChannel5Data`, `ReadCtfData` and `ReadH5OinaData` use them.

**Pattern to avoid:** test only `Hexagonal_High`, or add the literal 30 to angles in radians.

**Why:** the a/a* axis convention belongs to the hexagonal lattice, not only to its high-symmetry Laue class.

**How it showed up:** PR #1728 fixed a correction of 30 radians. PR #1781 added the missing correction to **Read Oxford
Instr. Channel 5 (.cpr/.crc)** and included `Hexagonal_Low`.

**How to recognize it next time:**

- Reader alignment code that selects a coordinate convention by one symmetry enum value.

---

## 19. Use the search grid only to find neighbor candidates

**Rule:** compute acceptance radii from the selected neighborhood definition. In diameter-based mode, use each
feature's own diameter. In fixed-radius mode, use the user-supplied radius for every feature. Spatial bins find
candidates; they do not change the acceptance test.

**Pattern to use:** in diameter-based mode, `ComputeNeighborhoods` uses the average diameter for the bin size and each
feature's diameter for its radius:

```cpp
radii[i] = equivalentDiameters[i] * multiplesOfAverage;
binSize = avgDiameter;
```

**Pattern to avoid:** replace the per-feature radii of the diameter-based mode with a global radius, or make its
directional neighbor relation symmetric.

**Why:** an acceleration structure must not change which neighbors qualify.

**How it showed up:** a performance rewrite used `avgDiameter * multiples / 2` as the radius, and **Compute Feature
Neighborhoods** found far fewer neighbors. PR #1652.

**How to recognize it next time:**

- A performance rewrite that uses the bin width as the scientific acceptance threshold.

---

## 20. Validate an in-place array by the object ID of its Attribute Matrix, not by its path

**Rule:** one DataObject can have several DataPaths. A filter that changes cell arrays in place checks that Feature Ids
and the ignored arrays it uses are children of the geometry's Cell Attribute Matrix by comparing object IDs, and checks
the ignored arrays only when the selected operation uses them. A Feature Attribute Matrix that a removal uses is a
direct child of the selected geometry and differs from its Cell Attribute Matrix. After validation, map the paths onto
the geometry's Cell Attribute Matrix path before the algorithm compares paths or finds sibling arrays.

**Pattern to use:** `RemoveFlaggedFeaturesFilter` checks membership with `ValidateArrayInGeometryCellData()`,
`ValidateArraysInGeometryCellData()` and `ValidateFeatureAttributeMatrixInGeometry()`, then calls
`MapToGeometryCellDataPath()` (all in `simplnx/Utilities/DataArrayUtilities.hpp`).

**Pattern to avoid:** compare the selected DataPaths as strings, or take the arrays to change from
`FeatureIdsArrayPath.getParent()` without checking that parent.

**Why:** a path comparison misses an ignored array that was selected through another path to the same Attribute
Matrix, and an unrelated parent selects the wrong arrays to change.

**How it showed up:** PR #1829 added the membership checks and the path mapping to the filters that change the cell
data of a geometry in place.

**How to recognize it next time:**

- A spatial in-place algorithm that compares selected paths or derives the arrays to change from the parent of the
  selected Feature Ids.

---

## 21. Keep the legacy conversions when a filter moves or a plugin is retired

**Rule:** every legacy SIMPL filter UUID has one conversion owner. Several legacy UUIDs can map to the same current
filter. Each mapping has a fixture with non-default values, and its test checks the target filter UUID and the
converted arguments. When a filter or plugin is retired, move its conversions to the replacement first.

**Pattern to use:** `src/Plugins/ImageProcessing/test/SimplConversionCoverageTest.cpp` reads the live conversion map,
requires a fixture for every entry, checks the target UUID, and rejects a conversion that produces only the default
arguments.

**Pattern to avoid:** a copied table of expected mappings, or a test that checks only that a pipeline loads.

**Why:** a missing mapping stops an old pipeline from converting, and a conversion that produces only the defaults
loses every saved value without an error.

**How it showed up:** removing the retired image-processing plugin dropped 59 SIMPL conversions, and no test failed.
PR #1752 restored the mappings and added the coverage test.

**How to recognize it next time:**

- A change that removes or moves a filter or plugin without moving its conversion map and fixtures.

---

## 22. Set a distinct modification time in a file-change test

**Rule:** when a test rewrites a file and expects a size-and-modification-time check to notice the change, set a
modification time that differs from the cached one.

**Pattern to use:** `test/Dream3dPreflightCacheTest.cpp`:

```cpp
const auto cachedMtime = fs::last_write_time(filePath);
// ... rewrite the file ...
fs::last_write_time(filePath, cachedMtime - std::chrono::seconds(20));
```

**Pattern to avoid:** rewrite the file and assume that its size or time changed.

**Why:** HDF5 can give different contents the same file size, and the timestamp resolution of the file system can hide
a fast rewrite.

**How it showed up:** the stale-file test of the preflight metadata cache needed an explicit timestamp to detect the
rewrite reliably. PR #1668.

**How to recognize it next time:**

- A cache-invalidation test that rewrites a file without setting its modification time.

---

## 23. Test geometry formulas with unequal spacing along each axis

**Rule:** test a formula that depends on axis spacing with unequal spacing, with a case along each axis, and with
expected values derived independently. Check a dimensionless result, such as sphericity, under uniform scaling.

**Pattern to use:** the analytical pair and scale-invariant sphericity cases in
`src/Plugins/SimplnxCore/test/ComputeSurfaceAreaToVolumeTest.cpp`.

**Pattern to avoid:** test only with equal spacing, or use agreement between two implementations as the only check.

**Why:** equal spacing hides a swapped axis, and two implementations can share the same error.

**How it showed up:** **Compute Surface Area to Volume** used the wrong face weights for X- and Y-normal faces, which
is wrong only when the X and Y spacings differ. PR #1817, issues #1773 and #1780.

**How to recognize it next time:**

- A test of an axis-dependent formula whose fixtures all use equal spacing or a symmetric shape.

---

## 24. Test a transfer failure after an earlier batch succeeded

**Rule:** when an algorithm reads or writes in batches, inject a failure after at least one batch has succeeded. Assert
the original error, the values that the earlier batches wrote, and that the regions after the failure stay unwritten.

**Pattern to use:** the "later metric I/O failures preserve prior batches" case in
`src/Plugins/SimplnxCore/test/ComputeSurfaceAreaToVolumeTest.cpp`, which fails each operation of the second batch.

**Pattern to avoid:** test only a failure of the first transfer, or assume that a failure undoes the earlier writes.

**Why:** a later failure leaves the earlier batches written, and only a test of that state shows whether the error and
the partial output are what the caller expects.

**How it showed up:** PR #1817 bounded the transfers of **Compute Surface Area to Volume** into batches and added these
failure cases.

**How to recognize it next time:**

- An algorithm that writes output before its last operation that can fail, with tests that fail only the first one.

---

## 25. Give Catch2 a bool when you compare file times

**Rule:** in `CHECK` and `REQUIRE`, put an extra pair of parentheses around a comparison of
`std::filesystem::file_time_type` values or of file-clock durations, so that Catch2 gets one `bool`.

**Pattern to use:** the legacy-file conversion case in `test/PreferencesSizeGBTest.cpp`:

```cpp
// Evaluate to bool before Catch2 tries to stream libc++'s file-clock time point.
CHECK((fs::last_write_time(files.input) == modifiedBefore));
```

**Pattern to avoid:**

```cpp
CHECK(fs::last_write_time(files.input) == modifiedBefore);
```

**Why:** Catch2 splits `a == b` into its two operands and compiles code that converts each operand to text for the
failure message. For a file-clock value, libc++ formats a floating-point number, which needs `std::to_chars` from
macOS 13.3. The macOS CI builds of DREAM3D-NX set the deployment target to macOS 11.7, so the test does not compile
there. A local build with a newer deployment target compiles it without an error.

**How it showed up:** `test/SelectiveImportReferencesTest.cpp` and `test/PreferencesSizeGBTest.cpp` did not compile on
the macOS CI jobs. PR #1764 fixed both.

**How to recognize it next time:**

- A `CHECK` or `REQUIRE` whose operand is `fs::last_write_time(...)`, a `file_time_type`, or a `clock::now() - ...`
  duration.
- The compile error `'to_chars' is unavailable: introduced in macOS 13.3` in `__format/formatter_floating_point.h`.

---

## Entry template

Copy this block for a new entry, number it, and add it to the index.

```markdown
## N. <The rule, stated as an instruction>

**Rule:** what to do and what not to do, in one or two sentences.

**Pattern to use:** a short code example, or the file and function that already does it right.

**Pattern to avoid:** a short code example of the mistake.

**Why:** the mechanism that makes the mistake fail.

**How it showed up:** the symptom, the filter or test, and the pull request or issue that fixed it.

**How to recognize it next time:**

- the code shape or the error message that gives it away.
```
