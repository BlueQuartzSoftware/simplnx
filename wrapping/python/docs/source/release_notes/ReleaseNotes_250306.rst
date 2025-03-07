Release Notes 25.03.06
======================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 25.03.06
-----------------


API Changes & Additions 25.03.06
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- No new changes

Change Log 25.03.06
^^^^^^^^^^^^^^^^^^^^

- BUG: Fixes bugs with Trilinear Interpolation in Apply Transformation to Geometry (#1220) (f052fcf34)
- FILT: ReadStringDataArray implemented (#1211) (b0fb842a0)
- BUG: Compute Twin Boundaries ASAN Cleanup (#1224) (e2258a45d)
- FILTER: Combine Node Based Geometries (#1210) (076136059)
- ENH: Change usages of 'DREAM.3D' to 'DREAM3D-NX' where appropriate (#1218) (0403f41b7)
- BUG: Fixes missing 'MaterialNames' arrays in the Small IN100 pipelines. (#1213) (65e3944a1)
- Removed unused header in Types.hpp (#1219) (4316aeab3)
- CMAKE: Number the file downloads and log download results to a file (#1212) (700306fb7)
- ENH: Allows users to store the transform matrix from the ApplyTransformationToGeometry (#1216) (8c16ef7ed)
- BUG: A NPE crash due to not checking a DataStore<T> pointer. (#1206) (91b62e342)
- BUG: RenameDataObject - Add Missing Overwrite Check (#1209) (a2101456e)
- FILTER: Find Twin Boundaries (#1193) (833d32097)
