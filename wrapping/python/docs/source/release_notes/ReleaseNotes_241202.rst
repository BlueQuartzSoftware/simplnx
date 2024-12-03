Release Notes 24.12.02
======================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 24.12.02
-----------------


API Changes & Additions 24.12.02
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Filters now have a version number that will be written to the pipeline file.

Change Log 24.12.02
^^^^^^^^^^^^^^^^^^^^

- ENH: Refactors the calculation of triangle normals and areas into utility functions (#1142) [093c79f53]
- BUG: Fixes issue where any component shape was allowed for ComputeTriangleGeometrySizes (#1134) [f36ca6c75]
- PYTHON: Expose methods to convert numeric types. (#1133) [b35840327]
- BUG: Various small bug fixes and corrections (#1131) [bae524443]
- BUG: Updated vcpkg baseline to include ITK windows compile fix (#1140) [ace11c0f8]
- BUG: XDMF-Ensure there is something to write before getting references. (#1129) [66f754236]
- REL: Update version to 1.5.0 (#1128) [2024-11-12]
