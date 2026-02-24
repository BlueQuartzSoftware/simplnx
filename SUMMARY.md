# SimplnxCore Filter Documentation Audit - Summary of Changes

## Overview

Audited all ~152 SimplnxCore filter documentation files against their corresponding C++ implementations. Fixed discrepancies across 4 C++ source files and ~30 markdown documentation files.

## C++ Code Bug Fixes (4 files)

### Missing Commas in `defaultTags()` (String Concatenation Bugs)

1. **CombineAttributeArraysFilter.cpp**: Added missing comma between `"Memory Management"` and `"Combine"` string literals. Without the comma, C++ string literal concatenation produced `"Memory ManagementCombine"` as a single tag instead of two separate tags.

2. **CreateImageGeometryFilter.cpp**: Added missing comma between `"ImageGeometry"` and `"Create Geometry"` string literals. Same string concatenation bug producing `"ImageGeometryCreate Geometry"` as a single tag.

### Inconsistent Tag Prefixes

3. **ReadBinaryCTNorthstarFilter.cpp**: Removed `#` prefix from all tags in `defaultTags()` (e.g., `"#IO"` -> `"IO"`). All other filters in the codebase use unprefixed tags.

4. **ReadVolumeGraphicsFileFilter.cpp**: Same `#` prefix removal as above.

## Documentation Title Fixes (17 files)

Updated doc titles to match `humanName()` in code:

| File | Old Title | New Title |
|------|-----------|-----------|
| CombineTransformationMatricesFilter.md | "...Matrices Filter" | "...Matrices" |
| ComputeCoordinatesImageGeomFilter.md | "...Image Geometry" | "...Image Geom" |
| ComputeKMedoidsFilter.md | "K Medoids" | "Compute K Medoids" |
| ComputeVectorColorsFilter.md | "Compute Vector Colors" | "Generate Vector Colors" |
| CreatePythonSkeletonFilter.md | "Generate Python..." | "Create Python..." |
| ExtractFeatureBoundaries2DFilter.md | "Extract Feature Boundaries 2D" | "Create Feature Boundaries (2D)" |
| PadImageGeometryFilter.md | "Pad Geometry (Image) #" | "Pad Image Geometry" |
| RandomizeFeatureIdsFilter.md | "Randomize Features Filter" | "Randomize Feature Ids" |
| RequireMinimumSizeFeaturesFilter.md | "Require Minimum..." | "Remove Minimum..." |
| SharedFeatureFaceFilter.md | "Generate Triangle Face Ids" | "Compute Triangle Face Ids" |
| TriangleCentroidFilter.md | "Calculate Triangle Centroids" | "Compute Triangle Centroids" |
| TriangleNormalFilter.md | "Calculate Triangle Normals" | "Compute Triangle Normals" |
| WriteDREAM3DFilter.md | "Write DREAM3D NX File" | "Write DREAM3D-NX File" |
| WriteNodesAndElementsFilesFilter.md | "...and..." | "...And..." |
| WriteSPParksSitesFilter.md | "...Sites FIle" (typo) | "...Sites File" |
| ReadZeissTxmFileFilter.md | "...TXM & TXRM File" | "...TXM/TXRM Files" |
| CreateAMScanPathsFilter.md | "...Paths Filter" | "...Paths" |

## Major Description Fixes (11 files)

1. **AlignGeometriesFilter.md**: Removed 6 nonexistent alignment methods (XY/XZ/YZ Min/Max Planes). Code only supports Origin and Centroid.

2. **ArrayCalculatorFilter.md**: Corrected output type description from "always double" to "user-selected numeric type (default: double)". Removed advice to use type conversion filter.

3. **ComputeArrayStatisticsFilter.md**: Changed statistics type from "double" to "32-bit float" for Mean, Median, Standard Deviation, Summation, and Standardized arrays.

4. **ComputeBoundingBoxStatsFilter.md**: Removed nonexistent "Histogram" row from output table. Changed "double" to "32-bit float" for Mean, Median, Standard Deviation, and Summation.

5. **ComputeKMeansFilter.md**: Updated distance metric description from "only Euclidean" to list all 6 supported metrics: Euclidean, Squared Euclidean, Manhattan, Cosine, Pearson, and Squared Pearson.

6. **CreateAMScanPathsFilter.md**: Removed references to nonexistent parameters (laser power, scan speed) and nonexistent outputs (times, powers). Updated to reflect actual parameters (hatch spacing, hatch length, rotation angle).

7. **CreateDataArrayFilter.md**: Removed false claim of random initialization. Code only supports user-defined initialization value.

8. **ErodeDilateMaskFilter.md**: Fixed backwards erosion description. Previously said false cells become true; corrected to true cells with false neighbors become false.

9. **InitializeImageGeomCellDataFilter.md**: Updated from "zeros only" to document all three initialization modes: Manual, Random, and Random With Range.

10. **WriteBinaryDataFilter.md**: Changed "single file" to "separate binary file per array in the output directory."

11. **WriteNodesAndElementsFilesFilter.md**: Fixed Hexahedral node count from 6 to 8.

## Medium Severity Fixes (8 files)

1. **CropVertexGeometryFilter.md**: Removed false claims about automatic Feature/Ensemble Attribute Matrix copying. Updated legacy "Data Container" terminology.

2. **NearestPointFuseRegularGridsFilter.md**: Removed false claim about Feature and Ensemble Attribute Matrix copying.

3. **DBSCANFilter.md**: Documented the third "Seeded Random" parse order option with description of all three modes.

4. **ComputeFeatureCentroidsFilter.md**: Documented the "Is Periodic" parameter and its effect on centroid computation for boundary features.

5. **SetImageGeomOriginScalingFilter.md**: Documented the "Put Input Origin at the Center of Geometry" parameter.

6. **MoveDataFilter.md**: Replaced legacy SIMPL terminology ("Attribute Array", "Data Container") with current simplnx terminology ("Data Objects", "Group").

7. **RemoveFlaggedTrianglesFilter.md**: Changed "edge data" to "triangle (face) data" in the Data Handling section.

8. **SliceTriangleGeometryFilter.md**: Replaced outdated DREAM3D Google Groups mailing list footer with current DREAM3D-NX GitHub Issues footer.

## Data Type Fixes (4 files)

1. **CreateDataArrayFilter.md**: Swapped Signed/Unsigned 8-bit integer ranges (were reversed).
2. **ConditionalSetValueFilter.md**: Same Signed/Unsigned 8-bit range swap fix.
3. **CreateDataArrayAdvancedFilter.md**: Same Signed/Unsigned 8-bit range swap fix.
4. **CreateGeometryFilter.md**: Changed "signed 64-bit integers" to "unsigned 64-bit integers" for element connectivity arrays.

## Orphan File Removal (1 file)

- **UncertainRegularGridSampleSurfaceMesh.md**: Removed duplicate orphan doc file (without "Filter" suffix). The proper `UncertainRegularGridSampleSurfaceMeshFilter.md` remains.

## Build & Test Results

- **Build**: Full ninja build (760/760 targets) completed successfully with no errors.
- **Tests**: All unit tests for modified C++ files pass (CombineAttributeArrays, CreateImageGeometry, ReadVolumeGraphicsFile, ReadBinaryCTNorthstar). 52 pre-existing test failures in the full suite are unrelated to these changes.

## Files Modified

### C++ Source Files (4)
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/CombineAttributeArraysFilter.cpp`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/CreateImageGeometryFilter.cpp`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadBinaryCTNorthstarFilter.cpp`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadVolumeGraphicsFileFilter.cpp`

### Documentation Files Modified (30)
- `src/Plugins/SimplnxCore/docs/AlignGeometriesFilter.md`
- `src/Plugins/SimplnxCore/docs/ArrayCalculatorFilter.md`
- `src/Plugins/SimplnxCore/docs/CombineTransformationMatricesFilter.md`
- `src/Plugins/SimplnxCore/docs/ComputeArrayStatisticsFilter.md`
- `src/Plugins/SimplnxCore/docs/ComputeBoundingBoxStatsFilter.md`
- `src/Plugins/SimplnxCore/docs/ComputeCoordinatesImageGeomFilter.md`
- `src/Plugins/SimplnxCore/docs/ComputeFeatureCentroidsFilter.md`
- `src/Plugins/SimplnxCore/docs/ComputeKMeansFilter.md`
- `src/Plugins/SimplnxCore/docs/ComputeKMedoidsFilter.md`
- `src/Plugins/SimplnxCore/docs/ComputeVectorColorsFilter.md`
- `src/Plugins/SimplnxCore/docs/ConditionalSetValueFilter.md`
- `src/Plugins/SimplnxCore/docs/CreateAMScanPathsFilter.md`
- `src/Plugins/SimplnxCore/docs/CreateDataArrayAdvancedFilter.md`
- `src/Plugins/SimplnxCore/docs/CreateDataArrayFilter.md`
- `src/Plugins/SimplnxCore/docs/CreateGeometryFilter.md`
- `src/Plugins/SimplnxCore/docs/CreatePythonSkeletonFilter.md`
- `src/Plugins/SimplnxCore/docs/CropVertexGeometryFilter.md`
- `src/Plugins/SimplnxCore/docs/DBSCANFilter.md`
- `src/Plugins/SimplnxCore/docs/ErodeDilateMaskFilter.md`
- `src/Plugins/SimplnxCore/docs/ExtractFeatureBoundaries2DFilter.md`
- `src/Plugins/SimplnxCore/docs/InitializeImageGeomCellDataFilter.md`
- `src/Plugins/SimplnxCore/docs/MoveDataFilter.md`
- `src/Plugins/SimplnxCore/docs/NearestPointFuseRegularGridsFilter.md`
- `src/Plugins/SimplnxCore/docs/PadImageGeometryFilter.md`
- `src/Plugins/SimplnxCore/docs/RandomizeFeatureIdsFilter.md`
- `src/Plugins/SimplnxCore/docs/ReadZeissTxmFileFilter.md`
- `src/Plugins/SimplnxCore/docs/RemoveFlaggedTrianglesFilter.md`
- `src/Plugins/SimplnxCore/docs/RequireMinimumSizeFeaturesFilter.md`
- `src/Plugins/SimplnxCore/docs/SetImageGeomOriginScalingFilter.md`
- `src/Plugins/SimplnxCore/docs/SharedFeatureFaceFilter.md`
- `src/Plugins/SimplnxCore/docs/SliceTriangleGeometryFilter.md`
- `src/Plugins/SimplnxCore/docs/TriangleCentroidFilter.md`
- `src/Plugins/SimplnxCore/docs/TriangleNormalFilter.md`
- `src/Plugins/SimplnxCore/docs/WriteBinaryDataFilter.md`
- `src/Plugins/SimplnxCore/docs/WriteDREAM3DFilter.md`
- `src/Plugins/SimplnxCore/docs/WriteNodesAndElementsFilesFilter.md`
- `src/Plugins/SimplnxCore/docs/WriteSPParksSitesFilter.md`

### Documentation Files Deleted (1)
- `src/Plugins/SimplnxCore/docs/UncertainRegularGridSampleSurfaceMesh.md`
