Release Notes 25.10.27
======================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 25.10.27
-----------------


API Changes & Additions 25.10.27
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

NONE

Change Log 25.10.27
^^^^^^^^^^^^^^^^^^^^

- BUG: Add missing spreadsheet macros and auto dock spreadsheet dock widget when loading old state files (#997) [2025-10-23]
- STY: Update and Add Scatter Chart Icons (#973) [2025-10-22]
- API: Update to latest EbsdLib 1.0.40 API and Update Reference Frame Docs (#999) [2025-10-22]
- BUG: Fix finalize method of caption annotation to remove actor when destructing (#996) [2025-10-20]
- ENH: Crop Geometry Parameter (#983) [2025-10-16]
- ENH: Move visualization tree check state indicator in place of decoration icon (#994) [2025-10-13]
- BUG: Fix build plate glow pass highlighter to not be visible when highlighted actor is not visible (#995) [2025-10-13]
- BUG: Fix changes to camera presets to update in all render windows (#993) [2025-10-13]
- BUG: Fix spreadsheet load state & reload data bugs (#991) [2025-10-10]
- ENH: Alphabetize window layout presets (#992) [2025-10-10]
- ENH: Add Quadric Clustering LOD Filter (#961) [2025-10-07]
- BUG: Add missing tool tips to spreadsheet color buttons and fix extra spacer in basic spreadsheet options (#989) [2025-10-07]
- ENH: Increment preference file version and auto dock new spreadsheet dock widget (#988) [2025-10-06]
- ENH: Implement basic and multi-dimensional spreadsheet module (#924) [2025-10-06]
- BUG: Fix size policy and initialization for the data structure treeview  (#984) [2025-10-04]
- ENH: ShapeType Centralization and Multidimensional NeighborList/StringArray (#975) [2025-10-03]
- BUG: Fix loading state files with visualization split views (#985) [2025-10-03]
- BUG: Fixes HDF5 memory leak in NXImportHDF5DatasetWidget.cpp (#982) [2025-09-30]

- ENH: Microtexture related filter cleanup (#1438) [2025-10-25]
- VERS: Update EbsdLib to 1.0.40 (#1458) [2025-10-21]
- ENH: Add option to use 26 neighbor kernel for SegmentFeatures class. (#1373) [2025-10-21]
- ENH: Crop Geometry Parameter add to ITKImageImport (#1449) [2025-10-16]
- BUG: Fixes Calculator filter array indexing int32 overflow. (#1456) [2025-10-08]
- ENH: Removed unnecessary use of 'typename' keyword (#1453) [2025-10-07]
- BUG: Compute Feature Phases Warnings Consolidated (#1455) [2025-10-07]
- BUG: Use a Static Boolean to Register ITK Factories Once (#1451) [2025-10-05]
- ENH: Modernize ComputeFZQuaternions (#1452) [2025-10-05]
- CI: Update macOS x86_64 to macOS 14 (#1450) [2025-10-04]
- ENH/API: Multi-Dimensional Tuple Support for StringArray and NeighborList (#1439) [2025-10-03]
- DOC: Update Build Documentation (#1448) [2025-10-01]
- BUG: Modernize Segment Features Feedback (#1444) [2025-10-01]
- BUG: Fix HDF5 memory leaks. (#1446) [2025-09-29]

* API: Update to latest EbsdLib API changes

* BUG: CAxisSegmentFeatures - Ensure all voxels that are segmented have a hexagonal phase type.
* BUG: ComputeAvgOrientations fix incorrect computation.
* BUG: ConvertOrientations - make sure the quaternion is properly formed before conversion
* BUG: Edax,Bruker,Oxford H5 EBSD Readers should check for null PatternData pointer
* BUG: FindFeatureReferenceCAxisOrientation - Use doubles to accumulate the StdDev values. Output values now agree with DREAM.3D > 6.5.171
* BUG: Fixes ComputeFeatureNeighborCAxisAlignments crash if "Find Avg Misalignments" was not enabled.
* BUG: Fixes incorrect progress message in ComputeNeighborhoods filter
* BUG: ReadH5Ebsd: EulerAngles must be read to perform the Euler Ref frame transform
* BUG: ReadH5OimData: Fix index calculation when copying data into final arrays
* DOC: Add Point Group and Rotation Point Group to the Laue class table
* DOC: Updates CreateEnsembleInfo documentation to add in the Rotation Point Groups
* ENH: Fixes SegmentFeatures algorithm to use a throttled messenger for progress updates
* ENH: Remove warning about hex phases for CAxisSegmentFeatures

