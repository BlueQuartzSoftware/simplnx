Release Notes 25.09.28
======================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 25.09.28
-----------------


API Changes & Additions 25.09.28
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


Change Log 25.09.28
^^^^^^^^^^^^^^^^^^^^


- REL: Official 7.2.2 release [2025-09-25]
- ENH: Allow user to change background render window color (#978) [2025-09-25]
- BUG: Add temporary fix for bar charts to handle vtk bug with very large numbers on bin ranges for bar charts (#979) [2025-09-25]
- STY: Use proper include syntax for EbsdLib since it is an external library (#974) [2025-09-24]
- ENH: Allow user to double-click select for dropped file dialog. (#977) [2025-09-24]
- BUG: Fix charting to only import the data arrays needed (#970) [2025-09-23]
- CI: Update CI Build Machines to Vtk 9.4.2 and Qt 6.8.3 (#976) [2025-09-23]
- BUG: Fix capitalization on default color table from FAST to Fast (#972) [2025-09-19]
- ENH: Move stylesheet editor to Edit menu (#968) [2025-09-17]
- STY: Adjust contrast of the Radio Buttons for Dark and Light mode (#969) [2025-09-17]
- BUG: Fix restoring pipelines in certain cases (#966) [2025-09-15]
- BUG: Fix importing DREAM3D file via drag & drop to import preflight data structure initially (#965) [2025-09-10]
- BUG: Fix parsing of simplnx number parameter value for comparison purposes (#964) [2025-09-08]
- BUG: Fix build plate tab order (#962) [2025-08-27]
- OPT: Use ColorTableUtilities API updates to move away from parsing JSON (#958) [2025-08-27]
- ENH: Add Linux lower case shell script to launch `dream3dnx` in conda environments (#956) [2025-08-21]
- ENH: Changes the default colormap of the render window to "Fast" (#943) [2025-08-21]
- ENH: Memory and Speed optimizations for Preflight with ReadDREAM3DFile filter and parameter (#957) [2025-08-21]
- ENH: Show Python Filter name in the Filter Parameters view for Anaconda builds (#955) [2025-08-21]
- BUG: Leave output directory the same when user cancels from file dialog (#959) [2025-08-21]
- BUG: Fix creation of vis presets  (#949) [2025-08-18]
- BUG: Handle empty string inputs for build plate annotation grid spacing  (#948) [2025-08-18]
- COMP: Update usage of 'ImportDataStructureFromFile' to better reflect intent (#942) [2025-08-18]
- BUG: Fixed VTK minimum version check to properly fail if not met (#952) [2025-08-18]
- BUG: MacOS default menu now loads states properly. (#945) [2025-08-14]
- ENH: Update various NX packaging and conda build variables (#939) [2025-08-07]
- ENH: Update state files to allow linking of nx and visualization pipelines for reloading (#934) [2025-08-06]
- BUG: Fix indexed LUT for multicomponent arrays (#938) [2025-08-04]
- STYLE: Fix styling for label in clip filter [2025-07-31]
- VERS: Update EbsdLib to v1.0.39 for conda builds (#936) [2025-07-30]
- BUG: 0D, 1D, and 2D geometry now create vtkPolyData instead of vtkUnstructuredGrid (#935) [2025-07-30]
- ENH: ComputeFaceIPFColoring Outputs two RGB Arrays (#1427) [2025-09-10]
- BUG: Laplacian was not releasing internal connectivity arrays. Now smooth edges, Hex and Tet Geometries (#1423) [2025-09-09]
- ENH: Catch exceptions and allow float64 when converting Quaternions (#1425) [2025-09-08]
- FILT: Read Zeiss TXM File filter added (#1424) [2025-09-08]
- FILTER: Combine Transformation Matrices (#1396) [2025-09-05]
- PERF/ENH: DBSCAN Rewrite (#1421) [2025-09-04]
- BUG: Fixes not checking the return from 'validateNumberOfTuples' in preflight in some filters (#1413) [2025-09-04]
- BUG: Fixes reading GrainMapper3D LabDCT dimensions correctly. (#1419) [2025-09-04]
- BUG: Fixes bug when moving data in ExtractVertexGeometry (#1415) [2025-09-04]
- OPT: DataStructure::getTopLevelData() optimizations (#1416) [2025-09-03]
- BUG: Fixes copying the Shared Vertex List in Remove Flagged Vertices (#1414) [2025-09-03]
- ENH: AbstractDataStore now uses proxies instead of references (#1409) [2025-09-03]
- BUG: Fixes ReadGrainMapper to convert IPFColors if requested by user (#1407) [2025-08-26]
- BUG: Fixes bug in BadDataNeighborOrientationCheck when finding the LaueOps instance (#1397) [2025-08-26]
- API: ColorTable API added to convert from JSON to vector of structs (#1400) [2025-08-25]
- COMP: Remove MSVC v142 toolset from CI (#1406) [2025-08-22]
- BUG: Add Type Check to GetAllChildDataPaths() (#1399) [2025-08-22]
- BUG: Fixes crash when Reading Legacy DREAM.3D file containing Stats data (#1391) [2025-08-07]
- ENH: SurfaceNets can now transfer both cell and feature data to the triangle mesh (#1386) [2025-08-07]
- BUG: Fixes ColorToGrayScale not checking for a 3-component input(s) (#1392) [2025-08-05]
- ENH: Allows QuickSurfaceMesh to transfer voxel feature data (#1388) [2025-08-05]
- ENH/BUG: RandomizeFeatureIds Repairs AM, Added Tests, Fixes Last Tuple Being Ignored (#1381) [2025-08-05]
- BUG: Fix failing unit test due to missing directory. (#1390) [2025-08-05]
- ENH: Add Preflight Feedback to ReadH5EBSDFilter for ANG and CTF Files (#1385) [2025-07-31]
- VER: Update EBSDLib to v1.0.39 (#1387) [2025-07-30]
- BUG: ApplyTransformationToGeometry - Include translation to/from global origin when saving transform. (#1384) [2025-07-29]
- BUG: ApplyTransformationToGeometry now calculates correct origin when scaling image geometries. (#1383) [2025-07-28]
- BUG: Fix crash if reading primitive types other than Int32 from HDF5 files (#1382) [2025-07-28]
- ENH: CreateGeometry filter can now convert from a SIMPL pipeline. (#1379) [2025-07-28]
