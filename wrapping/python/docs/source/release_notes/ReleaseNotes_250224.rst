Release Notes 25.02.24
======================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 25.02.24
-----------------


API Changes & Additions 25.02.24
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Filters now have a version number that will be written to the pipeline file.

Change Log 25.02.24
^^^^^^^^^^^^^^^^^^^^

- BUG FIX: CSV Reader now reads UTF-8 text files correctly. (#1205) [2025-02-24]
- ENH: Excessive Warning Cleanup (#1194) [2025-02-19]
- BUG: Fixes bug where CreateDataArrayAdvanced initializes the arrays twice (#1202) [2025-02-19]
- ENH: Updates array names to have padding digits for SplitAtributeArray. (#1204) [2025-02-18]
- BUG: Disables data transfer from voxels to triangles for SurfaceNets (#1200) [2025-02-14]
- BUG FIX: Read H5Ebsd File now reports image geometry bounds correctly. (#1201) [2025-02-13]
- ENH: Crop Image Geometry now crops by physical units by default. (#1196) [2025-02-11]
- BUG: Fixes Python plugin code generation and creates stub doc file(s) (#1195) [2025-02-10]
- BUG: Fix voxel center assumption for ComputeShapes (#1124) [2025-02-07]
- Updated vcpkg baseline for reproc 14.2.5 (#1192) [2025-02-05]
- Fixed some compiler warnings in simplnx (#1189) [2025-02-04]
- ENH: Geometry Constants Cleanup & GeomTypeToString Method (#1187) [2025-01-31]
- ENH: Fixes algorithm to compute shape factors from Triangle geometry (#1157) [2025-01-28]
- ENH: Add multi component support to MultiThresholdObjectsFilter (#1184) [2025-01-24]
- BUG: DataArray::getComponent should return a value not a reference (#1183) [2025-01-21]
- Fixed formatting (#1182) [2025-01-21]
- BUG: Fix format_pr workflow (#1180) [2025-01-21]
- FILTER: ITK Projection Filters (#1162) [2025-01-16]
- ENH: Improve performance of the Surface Mesh to Regular Grid. (#1146) [2025-01-16]
- ENH: Add slice-by-slice processing option to Isolate Largest Feature. (#1169) [2025-01-14]
- ENH: Update all Alignment Filter documentation to include output file format and explanation (#1170) [2025-01-13]
- BUG: Defends against malformed preferences JSON file or Json content within the file (#1171) [2025-01-10]
- BUG FIX: MeshIO writer code now writes cell data properly. (#1166) [2025-01-06]
- ENH: Use GetAllGeomTypes() method in PartitionGeometryFilter. (#1167) [2025-01-06]
- FILTER: Add WriteNodesAndElementsFiles filter (#1168) [2025-01-03]
- ENH: ReadCSVFile can now read data into existing data groups. (#1164) [2024-12-20]
- ENH: Use more specific parameters instead of generic parameters. (#1165) [2024-12-20]
- ENH: Add support for .ply files and vertex geometries to MeshIO python filters (#1163) [2024-12-19]
- FILTER: Crop Edge Geometry (#1153) [2024-12-11]
- ENH: Fixes high memory consumption in SurfaceNets Filter (#1156) [2024-12-10]
- ENH: Filters use consistent ordering for Spacing and Origin for Image Geometries (#1152) [2024-12-06]
- ENH: Added __iter__() and keys() to BaseGroup python binding (#1159) [2024-12-06]
