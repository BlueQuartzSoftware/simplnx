# Documentation & Code Fixes TODO

## SimplnxCore Plugin (Complete)

### Code Bugs
- [x] Fix missing comma in CombineAttributeArraysFilter defaultTags()
- [x] Fix missing comma in CreateImageGeometryFilter defaultTags()
- [x] Fix #-prefixed tags in ReadBinaryCTNorthstarFilter defaultTags()
- [x] Fix #-prefixed tags in ReadVolumeGraphicsFileFilter defaultTags()

### Human Name Mismatches (doc title vs code humanName())
- [x] CombineTransformationMatricesFilter - remove "Filter" suffix from doc
- [x] ComputeCoordinatesImageGeomFilter - "Image Geometry" → "Image Geom"
- [x] ComputeKMedoidsFilter - add "Compute" prefix in doc
- [x] ComputeVectorColorsFilter - "Compute" → "Generate"
- [x] CreatePythonSkeletonFilter - "Generate" → "Create"
- [x] ExtractFeatureBoundaries2DFilter - "Extract" → "Create"
- [x] PadImageGeometryFilter - "Pad Geometry (Image)" → "Pad Image Geometry"
- [x] RandomizeFeatureIdsFilter - "Randomize Features Filter" → "Randomize Feature Ids"
- [x] RequireMinimumSizeFeaturesFilter - "Require" → "Remove"
- [x] SharedFeatureFaceFilter - "Generate" → "Compute"
- [x] TriangleCentroidFilter - "Calculate" → "Compute"
- [x] TriangleNormalFilter - "Calculate" → "Compute"
- [x] WriteDREAM3DFilter - "DREAM3D" → "DREAM3D-NX"
- [x] WriteNodesAndElementsFilesFilter - lowercase "and" → "And"
- [x] WriteSPParksSitesFilter - typo "FIle" → "File"
- [x] ReadZeissTxmFileFilter - fix "&" vs "/" and singular/plural
- [x] CreateAMScanPathsFilter - remove "Filter" suffix from doc

### Major Description Inaccuracies
- [x] AlignGeometriesFilter - doc lists 8 alignment methods, code only has 2
- [x] ArrayCalculatorFilter - doc says output always "double", code allows selection
- [x] ComputeArrayStatisticsFilter - doc says "double", code uses float32
- [x] ComputeBoundingBoxStatsFilter - doc includes "Histogram" row not in code
- [x] ComputeKMeansFilter - doc says only Euclidean, code supports 6 metrics
- [x] CreateAMScanPathsFilter - doc references nonexistent parameters/outputs
- [x] CreateDataArrayFilter - doc claims random initialization not in code
- [x] ErodeDilateMaskFilter - doc describes erosion behavior backwards
- [x] InitializeImageGeomCellDataFilter - doc says only zeros, code supports more
- [x] WriteBinaryDataFilter - doc says "single file", code writes multiple
- [x] WriteNodesAndElementsFilesFilter - doc says hex has 6 nodes, should be 8

### Medium Severity Description Issues
- [x] CropVertexGeometryFilter - doc claims auto Feature/Ensemble copying
- [x] NearestPointFuseRegularGridsFilter - doc claims Feature/Ensemble AM copying
- [x] DBSCANFilter - doc omits "Seeded Random" parse order option
- [x] ComputeFeatureCentroidsFilter - undocumented "Is Periodic" parameter
- [x] SetImageGeomOriginScalingFilter - doc omits "Center Origin" parameter
- [x] MoveDataFilter - legacy SIMPL terminology
- [x] RemoveFlaggedTrianglesFilter - "edge data" should be "face/triangle data"
- [x] SliceTriangleGeometryFilter - outdated help footer

### Data Type Errors
- [x] CreateGeometryFilter - "signed 64-bit integers" should be uint64
- [x] Fix signed/unsigned 8-bit range errors in CreateDataArrayFilter.md
- [x] Fix signed/unsigned 8-bit range errors in ConditionalSetValueFilter.md
- [x] Fix signed/unsigned 8-bit range errors in CreateDataArrayAdvancedFilter.md

### Orphan Files
- [x] Remove UncertainRegularGridSampleSurfaceMesh.md duplicate

### Build & Test
- [x] Verify code compiles (ninja build: 760/760 succeeded)
- [x] Verify unit tests pass (all tests for modified C++ files pass; 52 pre-existing failures unrelated to changes)

---

## OrientationAnalysis Plugin (Complete)

### Code Bugs
- [x] Fix #-prefixed tags in AlignSectionsMutualInformationFilter defaultTags()
- [x] Fix typo "Phae" → "Phase" in CreateEnsembleInfoFilter defaultTags()

### Human Name Mismatches (doc title vs code humanName())
- [x] ComputeFZQuaternionsFilter - "Compute Reduction Orientations to Fundamental Zone" → "Compute Fundamental Zone Orientations"
- [x] ComputeMisorientationsFilter - "Compute Misorientations" (plural) → "Compute Misorientation" (singular)
- [x] ComputeQuaternionConjugateFilter - "Generate" → "Compute"
- [x] ComputeShapesFilter - "Compute Feature Shapes" → "Compute Feature Shapes (Image Geometry)"
- [x] ComputeShapesTriangleGeomFilter - "from Triangle Geometry" → "(Triangle Geometry)"
- [x] ComputeTwinBoundariesFilter - "Find" → "Compute"
- [x] ConvertOrientationsToVertexGeometryFilter - "to Rodrigues Fundamental Zone Geometry" → "To Rodrigues Geometry"
- [x] ReadChannel5DataFilter - "REad Oxford Channel 5 Data File" → "Read Oxford Instr. Channel 5"

### Doc Group/Subgroup Mismatches
- [x] 15 files: "Crystallographic" → "Crystallography" to match defaultTags()
- [x] BadDataNeighborOrientationCheckFilter - "Orientation Analysis" → "Processing"
- [x] ComputeFZQuaternionsFilter - "OrientationAnalysis (OrientationAnalysis)" → "Processing (OrientationAnalysis)"
- [x] ComputeQuaternionConjugateFilter - "Processing (OrientationAnalysis)" → "Processing (Crystallography)"
- [x] ConvertHexGridToSquareGridFilter - non-standard flat list → "Processing (Conversion)"
- [x] ConvertOrientationsFilter - "Orientation Analysis" → "Processing"
- [x] ConvertQuaternionFilter - "OrientationAnalysis (Conversions)" → "Processing (Conversion)"
- [x] ReadH5EspritDataFilter - "Import/Export (Import)" → "IO (Input)"
- [x] ReadH5OimDataFilter - "Import/Export (Import)" → "IO (Input)"
- [x] ReadH5OinaDataFilter - "Import/Export (Import)" → "IO (Input)"
- [x] RodriguesConvertorFilter - "OrientationAnalysis (Processing)" → "Processing (Crystallography)"
- [x] RotateEulerRefFrameFilter - "Orientation Analysis (Conversion)" → "Processing (Conversion)"

### Orphan Files
- [x] Remove GeneratePoleFigureFilter.md (no matching .cpp; WritePoleFigureFilter is the replacement)

### Build & Test
- [x] Verify code compiles
- [x] Verify unit tests pass
