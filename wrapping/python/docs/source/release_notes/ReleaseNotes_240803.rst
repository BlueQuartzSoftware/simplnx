Release Notes 24.08.03
======================

With this release we are moving towards a YYMMDD style of versioning. The `simplnx` library is
under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 24.08.03
-----------------


API Changes & Additions 24.08.03
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- No major changes/additions to note

Change Log 24.08.03
^^^^^^^^^^^^^^^^^^^^

- ENH: Fix compile warnings in FilterUtilities. Init array in DataStore when resizing (#1026) [d534b18d]
- ENH: Update NXRunner error messages to provide instructions for .json files (#1032) [5aa8dd5f]
- ENH: Combine and Write STL Files - Allow user to select a Part Number Index to use for numbering. (#1024) [24a870f9]
- CI: Fixed python tests on asan CI (#1031) [8eccc1bf]
- CI: Updated nightly ASAN CI to use clang 14 (#1029) [37c3df57]
- CI: Update GitHub CI to newer GCC & Clang versions. (#1028) [f05b3781]
- BUG FIX: PeregrineHDF5Reader file system path and Array Type Fixes (#1025) [0332e709]
- ENH: Allow Crop Image Geometry to only crop a specific dimension (#1021) [c3be139e]
- BUG FIX: Remove Python plugin import error handling. (#1019) [b9d47447]
- BUG: Fix Reading Shift File in AlignSectionsList (#1018) [ed04c3ac]
- ENH: Various ReadPeregrineHDF5File Improvements (#1016) [8346cd36]
- Fixed C++20 issue with template id in constructor starting in gcc 11 (#1015) [c62fa7f8]
- ENH: Update required standard to C++20 (#1004) [31406a49]
- PERF: Inline Keyword and Anonymous Namespace Cleanup (#1013) [639460e9]
- DOC: ConvertHexGridToSquareGrid Doc Updates (#1014) [37e1159a]
- COMP: Fix compiler warnings in ComputeSurfaceFeaturesTest (#1003) [8bc16916]
- BUG: Fix out of core preferences file save bug (#1012) [5f1e159f]
- BUG: KAM messages should only be printed at 1 a second. (#1008) [a6324cac]
- FILTERS: Adding MeshIO reader and writers. (#1009) [74a06a32]
- Remove submodule for geometry creation helper methods. (#1007) [3af3fd55]
- ENH: Add the ability to resample images as they are read in as an image stack. (#1011) [a2aaa4cb]
- ENH: Renaming DataAnalysisToolkit to NXDataAnalysisToolkit. (#1010) [1d9a483a]
- FILTER: Add RemoveFlaggedEdges filter. Update other RemoveFlaggedXXX filters (#986) [d7190167]
- ENH: Include additional geometry examples and tests. (#1006) [365acfd8]
- FILTER: SliceTriangleGeometry & CreateAMScanPaths (#984) [8f241745]
- ENH: More explicit details when a parameter does not validate. (#1005) [34a1c054]
- ENH: Added a benchmark utility executable (#1002) [90feefba]
- ENH: Adjust warning text for 0 step value in InitializeDataArray (#999) [cec98fca]
- BUG: Update geometry find element functions to use static casting (#1000) [d86dbbdc]
- FILTER/ENH: DBSCAN Filter and Clustering Cleanup (#994) [e0d0cf26]
- ENH: Update Python Plugin Generation File Structure (#989) [40512e34]
- BUG: Update node based geometry 'findElementSizes(bool)' family of functions (#991) [baf97023]
- ENH: Reading OEM EBSD HDF5 Files should have better error messages generated (#996) [95f41e7b]
- ENH: Added DataStructure::getDataAsUnsafe functions (#998) [788e0736]
- Update vcpkg baseline for HDF5 1.14.4.3 (#997) [68d0e63f]
- DOCS: Create Python tutorial covering basic usage and filter writing (#925) [07dab90b]
- ENH: Small fixes for DREAM3D-NX RC-12 release (#988) [5592a973]
- ENH: Small fixes for RC-12 release [b7958b48]
