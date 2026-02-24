# Filter Documentation Audit - Summary of Changes

## SimplnxCore Plugin

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

---

## OrientationAnalysis Plugin

Audited all 54 OrientationAnalysis filter documentation files against their corresponding C++ implementations. Fixed discrepancies across 2 C++ source files and 32 markdown documentation files, plus removed 1 orphan doc file.

## C++ Code Bug Fixes (2 files)

### Inconsistent Tag Prefixes

1. **AlignSectionsMutualInformationFilter.cpp**: Removed `#` prefix from tags in `defaultTags()` (`"#Reconstruction"` -> `"Reconstruction"`, `"#Alignment"` -> `"Alignment"`). All other filters in the codebase use unprefixed tags.

### Typo in Tags

2. **CreateEnsembleInfoFilter.cpp**: Fixed typo `"Phae"` -> `"Phase"` in `defaultTags()`. The misspelling prevented the filter from being found when searching by the "Phase" tag.

## Documentation Title Fixes (8 files)

Updated doc titles to match `humanName()` in code:

| File | Old Title | New Title |
|------|-----------|-----------|
| ComputeFZQuaternionsFilter.md | "Compute Reduction Orientations to Fundamental Zone" | "Compute Fundamental Zone Orientations" |
| ComputeMisorientationsFilter.md | "Compute Misorientations" (plural) | "Compute Misorientation" (singular) |
| ComputeQuaternionConjugateFilter.md | "Generate Quaternion Conjugate" | "Compute Quaternion Conjugate" |
| ComputeShapesFilter.md | "Compute Feature Shapes" | "Compute Feature Shapes (Image Geometry)" |
| ComputeShapesTriangleGeomFilter.md | "Compute Feature Shapes from Triangle Geometry" | "Compute Feature Shapes (Triangle Geometry)" |
| ComputeTwinBoundariesFilter.md | "Find Twin Boundaries" | "Compute Twin Boundaries" |
| ConvertOrientationsToVertexGeometryFilter.md | "Convert Orientations to Rodrigues Fundamental Zone Geometry" | "Convert Orientations To Rodrigues Geometry" |
| ReadChannel5DataFilter.md | "REad Oxford Channel 5 Data File (.cpr/.crc)" (typo + mismatch) | "Read Oxford Instr. Channel 5 (.cpr/.crc)" |

## Doc Group/Subgroup Fixes (32 files)

### "Crystallographic" -> "Crystallography" (15 files)

Updated doc subgroup text from adjective form to match the noun form used in `defaultTags()`:

- ComputeAvgCAxesFilter.md, ComputeAvgOrientationsFilter.md, ComputeBoundaryStrengthsFilter.md, ComputeCAxisLocationsFilter.md, ComputeFeatureNeighborCAxisMisalignmentsFilter.md, ComputeFeatureNeighborMisorientationsFilter.md, ComputeFeatureReferenceCAxisMisorientationsFilter.md, ComputeFeatureReferenceMisorientationsFilter.md, ComputeGBCDFilter.md, ComputeGBCDMetricBasedFilter.md, ComputeGBPDMetricBasedFilter.md, ComputeKernelAvgMisorientationsFilter.md, ComputeSchmidsFilter.md, ComputeSlipTransmissionMetricsFilter.md, ComputeTwinBoundariesFilter.md

### Group Name Mismatches (9 files)

| File | Old Group (Subgroup) | New Group (Subgroup) |
|------|---------------------|---------------------|
| BadDataNeighborOrientationCheckFilter.md | "Orientation Analysis (Cleanup)" | "Processing (Cleanup)" |
| ComputeFZQuaternionsFilter.md | "OrientationAnalysis (OrientationAnalysis)" | "Processing (OrientationAnalysis)" |
| ComputeQuaternionConjugateFilter.md | "Processing (OrientationAnalysis)" | "Processing (Crystallography)" |
| ConvertHexGridToSquareGridFilter.md | "Conversion, ANG File, EDAX, Hex Grid" | "Processing (Conversion)" |
| ConvertOrientationsFilter.md | "Orientation Analysis (Conversion)" | "Processing (Conversion)" |
| ConvertQuaternionFilter.md | "OrientationAnalysis (Conversions)" | "Processing (Conversion)" |
| RodriguesConvertorFilter.md | "OrientationAnalysis (Processing)" | "Processing (Crystallography)" |
| RotateEulerRefFrameFilter.md | "Orientation Analysis (Conversion)" | "Processing (Conversion)" |

### IO Group Fixes (3 files)

| File | Old Group (Subgroup) | New Group (Subgroup) |
|------|---------------------|---------------------|
| ReadH5EspritDataFilter.md | "Import/Export (Import)" | "IO (Input)" |
| ReadH5OimDataFilter.md | "Import/Export (Import)" | "IO (Input)" |
| ReadH5OinaDataFilter.md | "Import/Export (Import)" | "IO (Input)" |

## Orphan File Removal (1 file)

- **GeneratePoleFigureFilter.md**: Removed orphan doc file with no corresponding C++ filter. `WritePoleFigureFilter` is the replacement and already has its own documentation file (`WritePoleFigureFilter.md`).

## Build & Test Results

- **Build**: Full ninja build completed successfully with no errors.
- **Tests**: All OrientationAnalysis unit tests pass; no new test failures introduced.

## Files Modified

### C++ Source Files (2)
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/AlignSectionsMutualInformationFilter.cpp`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/CreateEnsembleInfoFilter.cpp`

### Documentation Files Modified (32)
- `src/Plugins/OrientationAnalysis/docs/BadDataNeighborOrientationCheckFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeAvgCAxesFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeAvgOrientationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeBoundaryStrengthsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeCAxisLocationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeFZQuaternionsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureNeighborMisorientationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureReferenceCAxisMisorientationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureReferenceMisorientationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeGBCDFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeGBCDMetricBasedFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeGBPDMetricBasedFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeKernelAvgMisorientationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeMisorientationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeQuaternionConjugateFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeSchmidsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeShapesFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeShapesTriangleGeomFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeSlipTransmissionMetricsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ComputeTwinBoundariesFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ConvertHexGridToSquareGridFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ConvertOrientationsFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ConvertOrientationsToVertexGeometryFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ConvertQuaternionFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ReadChannel5DataFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ReadH5EspritDataFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ReadH5OimDataFilter.md`
- `src/Plugins/OrientationAnalysis/docs/ReadH5OinaDataFilter.md`
- `src/Plugins/OrientationAnalysis/docs/RodriguesConvertorFilter.md`
- `src/Plugins/OrientationAnalysis/docs/RotateEulerRefFrameFilter.md`

### Documentation Files Deleted (1)
- `src/Plugins/OrientationAnalysis/docs/GeneratePoleFigureFilter.md`

---

## ITKImageProcessing Plugin

Audited all 88 ITKImageProcessing filter documentation files against their corresponding 88 C++ implementations. Fixed discrepancies across 9 markdown documentation files. No C++ code bugs were found in any `defaultTags()` functions.

## Documentation Title Fixes (5 files)

Removed extraneous parenthetical suffixes from doc titles to match `humanName()` in code:

| File | Old Title | New Title |
|------|-----------|-----------|
| ITKBoundedReciprocalImageFilter.md | "ITK Bounded Reciprocal Image Filter (ITKBoundedReciprocalImage)" | "ITK Bounded Reciprocal Image Filter" |
| ITKMaximumProjectionImageFilter.md | "ITK Maximum Projection Image Filter (ITKMaximumProjectionImage)" | "ITK Maximum Projection Image Filter" |
| ITKSmoothingRecursiveGaussianImageFilter.md | "...Filter (ITKSmoothingRecursiveGaussianImage)" | "ITK Smoothing Recursive Gaussian Image Filter" |
| ITKStandardDeviationProjectionImageFilter.md | "...Filter (ITKStandardDeviationProjectionImage)" | "ITK Standard Deviation Projection Image Filter" |
| ITKSumProjectionImageFilter.md | "...Filter (ITKSumProjectionImage)" | "ITK Sum Projection Image Filter" |

## Description Fixes (3 files)

1. **ITKAsinImageFilter.md**: Changed short description from "Computes the sine of each pixel." to "Computes the inverse sine (arcsine) of each pixel." -- the original incorrectly described the sine function instead of the arcsine function.

2. **ITKBinaryOpeningByReconstructionImageFilter.md**: Changed short description from "binary morphological closing of an image." to "Binary morphological opening by reconstruction of an image." -- copy-paste error from the closing filter.

3. **ITKStandardDeviationProjectionImageFilter.md**: Changed short description from "Mean projection." to "Standard deviation projection." -- copy-paste error from the mean projection filter.

## Doc Group/Subgroup Fixes (2 files)

| File | Old Group (Subgroup) | New Group (Subgroup) |
|------|---------------------|---------------------|
| ITKImportFijiMontageFilter.md | "Import/Export (Import)" | "IO (Input)" |
| ITKMaskImageFilter.md | "ITKImageProcessing (ITKImageProcessing)" | "ITKImageIntensity (ImageIntensity)" |

## Doc Format/Footer Fixes (1 file)

- **ITKBoundedReciprocalImageFilter.md**: Replaced manual parameter table (empty `## Parameters`, `## Required Geometry`, `## Required Objects`, `## Created Objects` sections) with the standard `% Auto generated parameter table will be inserted here` placeholder. Updated footer heading from "DREAM3D Mailing Lists" to "DREAM3D-NX Help".

## Known Issue (Not Fixed)

- **ITKBoundedReciprocalImageFIlter.cpp**: The .cpp filename has a typo -- capital "I" in "FIlter". The corresponding header and doc files are spelled correctly. Not renamed because it would require CMakeLists.txt and header changes, risking build breakage. Flagged for manual attention.

## Build & Test Results

- **Build**: Ninja build (16/16 targets) completed successfully with no errors.
- **Tests**: All 151 ITKImageProcessing unit tests pass (100%). No new test failures introduced.

## Files Modified

### Documentation Files Modified (9)
- `src/Plugins/ITKImageProcessing/docs/ITKAsinImageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKBinaryOpeningByReconstructionImageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKBoundedReciprocalImageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKImportFijiMontageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKMaskImageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKMaximumProjectionImageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKSmoothingRecursiveGaussianImageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKStandardDeviationProjectionImageFilter.md`
- `src/Plugins/ITKImageProcessing/docs/ITKSumProjectionImageFilter.md`
