Release Notes 1.2.4
===================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.2.4
-------------

- Documentation has been updated
- Examples updated to use new API


API Additions 1.2.4
^^^^^^^^^^^^^^^^^^^

None this release

Change Log 1.2.4
^^^^^^^^^^^^^^^^^^^^

- DOC: Created example images for most ITK ImageProcessing Filters (#841) - c16c9d57
- ENH: ReadCSVFilter and TriangleCentroidFilter improvements (#854) - dcdb63e2
- ENH: FeatureFaceCurvature Filter Enhancements (#853) - b568c7bb
- DEV: Enable macOS-ARM, remove extra checkout steps (#848) - f8099861
- BUG: GeneratePoleFigure-Only create RGB when creating the Image Geometry (#827) - 3a1e77e8
- DOC: Link directly to the discussion page on GitHub. (#847) - c83e9f2a
- ENH: Fixed cmake syntax warning (#850) - 4db702a5
- ENH: Replaced deprecated exec_program with execute_process (#851) - 8f1a2919
- FILTERS: Added CalculateHistogram, InterpolateGridData, and CliReader Python filters. (#838) - 4a830573
- COMP: Clang v14 -ffp-contract=off compiler option (#849) - 9d8379b6
- FILTER: Label Triangle Geometry and Remove Flagged Triangles Filters (#842) - d512fa24
- BUG: Fixed example pipeline installation when creating Anaconda package (#846) - c265d601
- ENH: Misc user facing labels and help text updates (#845) - 62e6ecec
- BUG: Fix IParallelAlgorithm not running in parallel (#844) - d1d613b2
- BUG: Fix human facing label in Crop Image Geometry (#843) - 73a21c9e
- BUG: ReadStlFile-Fix error logic when reading an ASCII STL File. (#839) - 77f3909c
- Updated vcpkg baseline for pybind11 2.11.1 (#840) - 0ff43b9d
- PY: Fix GIL crashes, unit test false positive results, misc other test issues. (#831) - c6112835
- ENH: ApplyTransformationToGeometry: Better error messages, Update documentation. (#835) - 6406635b
- ENH: ITKImageReader - Allow user to set/override the origin/spacing (#834) - 18498ae2
- ENH: ITKImportImageStack - Add option to convert images to grayscale 'on-the-fly'. (#832) - 19b009c8
- BUG: ITKImportFijiMontage: Fixing incorrect DataPath that was causing a crash. (#833) - 7c434ae5
- BUG: Fix preflight updated values for Create Geometry filters (#825) - 7060dcaa
- BUG: Fix ITKImportImageStack image flip operations. Add unit test for flip operations. (#815) - 3f70819a
- DynamicTableParameter: Return better error message when validation fails. (#824) - 554be1bd
- FILTER: Generate Python Plugin/Filter Skeleton Code (#821) - f869943b
- Fixed shallow copy in python bindings function (#822) - 3b8230ce
- Changed StringArray to store its data in a shared_ptr (#823) - ce7ae084
- ENH: Crop Image Geometry with Physical Coordinate Bounds (#818) - 3e637324
- PY: Added Python Code for Most Example Pipelines (#800) - f282e771
- ENH: Fix various anaconda packaging issues (#810) - 6eef7638
- ENH: Group a copy file target into the proper source group (#817) - 91964a50
- DOC: ConvertHexGridToSquareGrid. Update and add image to doc file (#816) - 3fa6c4a1
- ENH: Write STL File - Option to write out the entire triangle geometry in a single file. (#809) - 1e7759ae
- BUG: Fix determination of default parallelization mode. (#814) - 8641d3f4
- STYLE: Fix all-contributors Dynamic Badge in README (#813) - f1d7bbed
- BUG: Fix missing python dependencies for ASAN build (#812) - f36fa0cd
- ENH/BUG: Additional Origin Centering Options and BoundingBox API Fix (#811) - 62df7647
- Expose IDataArray/IDataStore resize_tuples method to Python. (#807) - 4994adba
- PY: Fix crash when running nxrunner with python filters. (#808) - 26de9937
- PY: Enable testing python example codes (#806) - cded997f
- ENH: Create unit tests for each python example pipeline (#793) - 0e9ce25e
- ENH: Add Additional Warnings When Moving/Copying DataObjects with a Tuple Mismatch (#804) - 6b7b1f0e
- BUG: Repair Append Image Geometry Z Slice Default Pipeline (#805) - ca4c8189
- ENH: Rename complex to simplnx (#801) - 95265d6c
- BUG FIX: All IDataCreationAction subclasses now correctly pass the geometry path member variable when being cloned. (#802) - f340465e
- ENH: Write Temp Files for All Writers (#790) - 5b9d046b

