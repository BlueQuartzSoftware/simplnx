Release Notes 25.07.23
======================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 25.07.23
-----------------


API Changes & Additions 25.07.23
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Added NeighborList Pythong Bindings
- Add ability to append to a DREAM3D file

Change Log 25.07.23
^^^^^^^^^^^^^^^^^^^^

- BUG: ReadCSVFile filter now properly uses the filter's instance id. (#1367) [2025-07-19]
- BUG: Fix Import DREAM3D Memory Usage (#1341) [2025-07-17]
- ENH: Add 'swap' method to IDataArray and implementations in subclasses. (#1347) [2025-07-15]
- BUG: Set attribute on temp directory to force Dropbox to not sync temp files (#1362) [2025-07-11]
- ENH: Add progress feedback to RotateEulerRefFrame (#1360) [2025-07-10]
- ENH: Add option to keep input geometry origin for RotateSampleRefFrame Filter (#1355) [2025-07-03]
- ENH: Allow existing geometry in RegularGridSampleSampleSurfaceMesh (#1344) [2025-07-03]
- ENH: Add min/max operators to Array Calculator filter. (#1351) [2025-07-02]
- DOC: Add missing documentation for Randomize FeatureIds Filer (#1348) [2025-07-01]
- ENH: Creates an edge geometry that visualize bounding boxes (#1326) [2025-07-01]
- ENH: Add StringArray Support To ReadCSVFile Filter (#1339) [2025-06-30]
- BUG: Fix out-of-bounds array access in RequireMinNumNeighbors Filter (#1343) [2025-06-30]
- ENH: Improved threadsafe messaging (#1340) [2025-06-30]
- ENH: Add Randomize FeatureI ds utility and filter (#1306) [2025-06-24]
- BUG: Fix Hardcoded Element List Name to Match SIMPL (#1336) [2025-06-20]
- BUG: Misc Image Geometry Bug Fixes (#1328) [2025-06-20]
- ENH: Update vcpkg for fmt, spdlog, and ITK (#1333) [2025-06-20]
- BUG: RequireMinNumNeighbors DataArray Update fixes (#1320) [2025-06-19]
- ENH: Allow "Segment Features" to consider periodic microstructures (#1291) [2025-06-18]
- FILT: Compute Coordinate Threshold (#1319) [2025-06-18]
- ENH: Replace C style 2D arrays with Eigen::Matrix (#1324) [2025-06-17]
- FILT: Split Array (By Tuple) (#1327) [2025-06-13]
- FILT: Compute Coordinates Image Geom (#1316) [2025-06-06]
- ENH: ComputeFeatureShapesTriangleGeom algorithm parallelization (#1307) [2025-06-05]
- PY: Added NeighborList python bindings (#1314) [2025-06-02]
- FILT: Compute Feature Bounds Filter (#1303) [2025-05-27]
- BUG: Fix RequireMinNumNeighbors Not Using Ignore Paths (#1310) [2025-05-24]
- ENH: Clarify Kernel Average Misorientation algorithm with proper casting. (#1305) [2025-05-23]
- BUG: Range check Compute Triangle Feature Volumes (#1309) [2025-05-23]
- ENH: Update ValidateFeatureIdsToFeatureAttributeMatrixIndexing API (#1308) [2025-05-23]
- STYLE/PERF: Remove Deprecated Test Files and Remove Dead Code in Test Case (#1302) [2025-05-16]
- ENH: Compute Array Statistics - Range Gating and Histogram Migration (#1288) [2025-05-15]
- ENH: Updates codes to use MakeErrorResult() for better readability (#1297) [2025-05-14]
- ENH: Ensure all filters are checking for cancel and sending progress messages. (#1267) [2025-05-13]
- ENH: Add Fill Functionality to Create Array Action & Utility File Refactoring (#1295) [2025-05-13]
- FILT: Point Sample Edge Geometry (#1286) [2025-05-13]
- ENH: Add function to generate an example file name for the generated file list parameter (#1296) [2025-05-13]
- ENH: Add ability to append to a DREAM3D file (#1294) [2025-05-09]
- COMP: Misc. compiler warning cleanups. (#1249) [2025-05-08]
- ENH: Add functions to support loading indexed color presets (#1289) [2025-05-07]
- ENH: Assign -1.0 to all values if triangle mesh is non-conformant. (#1293) [2025-05-06]
- ENH: Verify Triangle Windings Filter and Meshing Util API Expansion (#1279) [2025-05-02]
- ENH: Update conda and vcpkg to use EbsdLib 1.0.38 (#1290) [2025-05-02]
- ENH: Crop Image Geometry now displays input image info even if there are filter errors. (#1287) [2025-05-01]
- ENH: Add Euler Characteristic computation to Compute Triangle Geometry Shapes (#1280) [2025-04-24]
- BUG: Ensure FeatureId arrays are range checked against the Feature Attribute Matrix. (#1278) [2025-04-24]
- BUG: Fix incorrect logic to validate usage of existing attribute matrix (#1275) [2025-04-22]
- ENH: Updated python filter template to show correct way to cancel (#1268) [2025-04-15]
- ENH: Make Dummy Node Optional in Abaqus Hexahedron Writer (#1265) [2025-04-15]
- ENH: Show EBSD Phase Info in Preflight Updated Values for each EBSD readers (#1263) [2025-04-11]
- COMP: Fix missing include directive for <algorithm> (#1266) [2025-04-11]
- FILT: Add 'Pad Image Geometry' filter to SimplnxCore (#1255) [2025-04-11]
- ENH: Pull out CSV Reader helper methods to FileUtilities::CSV (#1261) [2025-04-11]
- ENH: Remove Matrix3x3 and Matrix3x1 classes. Use Eigen instead. (#1258) [2025-04-10]
- BUG: Fix the embedded names of example pipelines and parameter warnings. (#1256) [2025-04-10]
- ENH: Fixes empty exception messages. (#1246) [2025-04-10]
- BUG: Reads and Writes the length units for geometries. (#1251) [2025-04-10]
- ENH: Add missing documentation comments for preflight and execute methods in filters (#1257) [2025-04-10]
- ENH: Update to EbsdLib 1.0.38 (#1259) [2025-04-10]
- ENH: Merge Initial Out-of-Core infrastructure (#1253) [2025-04-09]
- ENH: Dream3dImportParameter supports additional import options. (#1230) [2025-03-26]

