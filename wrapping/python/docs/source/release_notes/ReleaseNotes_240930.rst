Release Notes 24.09.30
======================

With this release we are moving towards a YYMMDD style of versioning. The `simplnx` library is
under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 24.09.30
-----------------


API Changes & Additions 24.09.30
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- No major changes/additions to note

Change Log 24.09.30
^^^^^^^^^^^^^^^^^^^^

- ENH: Updated NXSeparatorParameter to new SIMPLNX API(#736) [820c3f76]
- BUG: Fix Visualization tree indentation (#735) [694b4332]
- PY: Update conda build specs for EbsdLib 1.0.33 [2d227f46]
- BUG: Turn off auto scrolling on data structure treeview (#733) [f930221e]
- BUG: Visualization 2D Image/Slice Data View Reload Crash Fix (#732) [be2c675d]
- ENH: Added ability to reload the input file to Read CSV Filter. (#729) [7b970583]
- BUILD: Update various CMake Options to better reflect what is actually being built (#730) [f0824129]
- ENH: disable mouse wheel events  (#728) [eb3033d3]
- BUG: Fix resetting of NXIconPushButton hover/normal icon state (#727) [f949ef62]
- PY: Fixed bug when reloading python plugins with the same name (#725) [c2bee63e]
- PY: Switched to conda packages for ITK and VTK (#726) [a116a839]
- ENH: Add help label to visualization render properties treeview when empty (#719) [ca88cfd0]
- BUG FIX: Adding empty map check in VXOutlineRep. (#723) [ed2d17ea]
- Stop moving data store from DataStructure in CV::Array (#722) [8150fbef]
- ENH: Add build plate default settings option (#721) [89786b09]
- Fixed compile issues when DREAM3DNX_COMMERCIAL is off (#718) [65d865d4]
- COMP: Update QtADS to version 4.3.1 [b8df4396]
- PY: Conda 24.08.10 Release [585ed8ea]
- BUG: Fix crash on spreadsheet view close (#715) [8312a458]
- BUG: Add representation after threshold algorithm is applied (#716) [b52bacad]
- ENH: Update the output from histogram ranges to be a 2 component array (#1087) [cafdcf81a]
- Added versioning to filter parameters and json (#1088) [9bb1c8959]
- ENH: Flip the incoming data across the X axis in PeregrineHDF5Reader. (#1086) [f25133d2f]
- ENH: Improvements and Bug Fixes to Orientation Analysis filters. (#1081) [803702574]
- FILTER: Concatenate Data Arrays (#1072) [e6148a2d6]
- Better preflight values for CreateDataArrayAdvanced & InitializeData. (#1084) [af4f5c14d]
- BUG/API: Histogram Sync (#1073) [7b7312847]
- SIMPLConversion header optimization (#1082) [99a8fe14]
- FILT: CreateDataArray Advanced (Create Data Array + Initialize Data Array) (#1066) [8f8be484]
- ENH: Update Read CSV Filter caching to handle modified files. (#1078) [92d6cd64]
- ENH: Los Alamos Writer Progress Updates (#1080) [9eeeacb9]
- ENH: Add support for poly lines to CLIReader. (#1067) [17241eb8]
- BUG: SliceTriangleGeometry - fix crash when RegionIds are not used. (#1065) [b4e856ca]
- COMP: Update Ebsdlib to 1.0.32 (#1056) [22f88bf7]
- ENH: Clear method of ManualImportFinder exposed to Python (#1060) [a613be9a]
- COMP: ITKThresholdMaximumConnectedComponentsImageFilter Fix value changed warning (#1043) [75fa9ad2]
- CI: Added more leak suppressions for python code in ASAN CI (#1058) [0846bc1e]
- Removed -no-pie from ITKImageProcessing as it is not an executable (#1059) [65ccd634]
- ENH: Add the ability to read from multiple cameras to PeregrineHDF5Reader filter (#1055) [4c82103f]
- COMP: Switched to __cpp_lib_bit_cast to detect std::bit_cast availability (#1057) [fb0dc0e6]
- ENH: AppendImageGeometry now has option to mirror across the chosen axis. (#1052) [532ad792]
- BUG: Fixed LD_LIBRARY_PATH and file path typo for ASAN CI (#1053) [7e684986]
- CI: Added option to download data before compile (#1054) [609b0707]
- ENH/BUG: Data Array to Store, Speed Optimizations, Code Cleanup (SimplnxCore) (#1017) [7fb7ffe9]
- BUG: Fixed missing include on latest msvc (#1049) [46a0a0c0]
- PYTHON DOCS: Use proper filter and parameter names in Tutorial 1. (#1048) [a0f0ac5e]
- BUG FIX: Crop Image Geometry now retains the proper geometry length units (#1046) [5391ed6a]
- ENH: AppendImageGeometryZSliceFilter -> AppendImageGeometryFilter (#1041) [6a4de836]
- PY: Conda 24.08.10 Release [e473c9a0]
- ENH: Peregrine HDF5 Reader Layer Thickness Override & Bug Fixes (#1045) [493c94de]
- ENH: SampleSurfaceMesh and GeometryMath Speed Optimizations (#1020) [e1f45937]
- ENH: ITKImageReader can change image data type (#1036) [a44f4914]
- DOC: Updates to python docs (#1039) [6137b7b7]
- BUG: Fixed python version check for existence of importlib.metadata (#1040) [acaa8f88]
- BUG: Fixed ASAN CI test discovery and leak detection (#1037) [91139664]