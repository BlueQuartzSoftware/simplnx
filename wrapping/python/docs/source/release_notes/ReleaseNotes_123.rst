Release Notes 1.2.3
===================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.2.3
-------------

- Documentation has been updated
- Examples updated to use new API


API Additions 1.2.3
^^^^^^^^^^^^^^^^^^^

None this release

Filter Changes 1.2.3
^^^^^^^^^^^^^^^^^^^^
- ENH: ITKImportImageStack - Add option to convert images to grayscale 'on-the-fly'. (#832) [2024-01-29]
- BUG: ITKImportFijiMontage: Fixing incorrect DataPath that was causing a crash. (#833) [2024-01-29]
- BUG: Fix preflight updated values for Create Geometry filters (#825) [2024-01-23]
- BUG: Fix ITKImportImageStack image flip operations. Add unit test for flip operations. (#815) [2024-01-19]
- DynamicTableParameter: Return better error message when validation fails. (#824) [2024-01-17]
- FILTER: Generate Python Plugin/Filter Skeleton Code (#821) [2024-01-17]
- Fixed shallow copy in python bindings function (#822) [2024-01-16]
- Changed StringArray to store its data in a shared_ptr (#823) [2024-01-16]
- ENH: Crop Image Geometry with Physical Coordinate Bounds (#818) [2024-01-16]
- PY: Added Python Code for Most Example Pipelines (#800) [2024-01-14]
- ENH: Fix various anaconda packaging issues (#810) [2024-01-12]
- ENH: Group a copy file target into the proper source group (#817) [2024-01-12]
- DOC: ConvertHexGridToSquareGrid. Update and add image to doc file (#816) [2024-01-12]
- ENH: Write STL File - Option to write out the entire triangle geometry in a single file. (#809) [2024-01-09]
- BUG: Fix determination of default parallelization mode. (#814) [2024-01-05]
- STYLE: Fix all-contributors Dynamic Badge in README (#813) [2024-01-05]
- BUG: Fix missing python dependencies for ASAN build (#812) [2024-01-05]
- ENH/BUG: Additional Origin Centering Options and BoundingBox API Fix (#811) [2024-01-03]
- Expose IDataArray/IDataStore resize_tuples method to Python. (#807) [2023-12-28]
- PY: Fix crash when running nxrunner with python filters. (#808) [2023-12-27]
- PY: Enable testing python example codes (#806) [2023-12-27]
- ENH: Create unit tests for each python example pipeline (#793) [2023-12-26]
- ENH: Add Additional Warnings When Moving/Copying DataObjects with a Tuple Mismatch (#804) [2023-12-26]
- BUG: Repair Append Image Geometry Z Slice Default Pipeline (#805) [2023-12-26]
- ENH: Rename complex to simplnx (#801) [2023-12-22]
- BUG FIX: All IDataCreationAction subclasses now correctly pass the geometry path member variable when being cloned. (#802) [2023-12-21]
- ENH: Write Temp Files for All Writers (#790) [2023-12-19]