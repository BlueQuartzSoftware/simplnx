Release Notes 1.1.0
===================

The `complex` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.1.0
-------------

API Additions 1.1.0
^^^^^^^^^^^^^^^^^^^

- DataPath: A :ref:`DataPath<DataPath>` object can now be constructed with a "/" separated string in addition to a list of strings.

  .. code:: python

    some_path = complex.DataPath("DataContainer/AttributeMatrix/DataArray")


Filter Changes 1.1.0
^^^^^^^^^^^^^^^^^^^^

- Filters that used a "Mask Array" or a "Good Voxels" array have all been changed to use following terms:
   
   - OLD: 'use_good_voxels' **NEW:** 'use_mask'
   - OLD: 'good_voxels_array_path'  **NEW:** 'mask_array_path'
   - REASONING: This presents a consistent API to the developer across all filters

- ImportCSVDataFilter:

    This filter has been changed to "ReadCSVFilter". The input parameter has also changed. See the :ref:`ReadCSVDataParameter<ReadCSVDataParameter>` for more information.


Git Log 1.1.0
^^^^^^^^^^^^^

- BUG: Export ASCII Data now exports multiple arrays correctly. (#756) - [2023-10-27]
- ENH: Use "Mask" instead of "GoodVoxels" consistently for human facing strings. (#755) - [2023-10-27]
- PY: Use SPHINX_BUILD_EXECUTABLE instead of SPHINX_EXECUTABLE (#750) - [2023-10-26]
- ENH: Additions to nxrunner for detailed version information. (#752) - [2023-10-26]
- ENH: Add HumanStringToDataType Method in TypesUtilities.h (#754) - [2023-10-26]
- ENH: Create DataModifiedAction that marks DataObjects as being modified by a filter (#735) - [2023-10-25]
- ENH: Added 4 Major Issue Templates (#749) - [2023-10-25]
- API: Added cx.DataPath constructor to take a "/Path/Like/This" string (#751) - [2023-10-24]
- ENH: Standardize and Cache Random Seed values used in filters  (#745) - [2023-10-24]
- ENH: GenerateColorTable allows use of a mask array. (#747) - [2023-10-20]
- ENH: Convert AttributeArray DataType allows user to delete input array. (#746) - [2023-10-19]
- TEST: Add Unit test for empty Help Strings in filters. Clean up empty help strings (#744) - [2023-10-18]
- DOC: Add Yang Liu as a Contributor for Research and Data (#743) - [2023-10-17]
- DOC: Improve explanations for the Find Average C-Axis algorithm. (#741) - [2023-10-17]
- FILTER: EDAX .ang file Hex Grid To Sqaure Grid Converter (#738) - [2023-10-17]
- ENH: Replaced CMAKE_CFG_INTDIR with $<CONFIG> (#687) - [2023-10-15]
- DOC: Update documentation files to allow Sphinx to generate html documentation (#708) - [2023-10-12]
- BUG: Normalize Final Find Avg C Axes Direction Vector (#739) - [2023-10-12]
- ENH: Read CSV File Filter Redesign (#706) - [2023-10-12]
- DOCS: Update to the python docs. (#733) - [2023-10-09]
- ENH: Add proper sample and crystal reference frame rotations to these pipelines (#734) - [2023-10-09]
- REQ: Update EbsdLib Requirement to v1.0.26 (#711) - [2023-10-06]
- DOC: Small documentation fixes. (#732) - [2023-10-06]
- DOC: Add VKUDRI as a Contributor for Code (#727) - [2023-10-05]
- DOC: Add john-stone-ics as a Contributor for Code (#726) - [2023-10-05]
- DOC: Add bpenniebq as a Contributor for Code (#725) - [2023-10-05]
- Docs: Add nyoungbq as a Contributor for Code (#724) - [2023-10-05]
- DOC: Add mmarineBlueQuartz as a Contributor for Code (#723) - [2023-10-05]
- DOC: Add jmarquisbq as a Contributor for Code (#722) - [2023-10-05]
- DOC: Add joeykleingers as a Contributor for Code (#721) - [2023-10-05]
- DOC: Add JDuffeyBQ as a Contributor for Code (#720) - [2023-10-05]
- Docs: Add imikejackson as a Contributor for Code (#718) - [2023-10-05]
- BUG: Update .all-contributorsrc (#713) - [2023-10-05]
- DOC/ENH: Contributor Updates (#712) - [2023-10-05]
- COMP: Fix compiler warnings due to casting. (#709) - [2023-10-04]
- ENH: Remove dependency on EbsdLib for the Parameter. (#702) - [2023-10-03]
- DOC: Fill in missing ITK Filter help text. Improves generated python documentation (#707) - [2023-10-02]
- BUG: ITK Filters should check total number of tuples for input compatibility (#705) - [2023-09-29]
- BUG: STLFileReader/Writer - Fix crash when reading certain kinds of STL Files. Fix output path when writing (#701) - [2023-09-26]
- BUG/PERF: Find Neighborhoods Progress and Parallel Optimizations (#697) - [2023-09-21]
- BUG: Fix human name for FeatureFaceCurvatureFilter (#695) - [2023-09-20]
- BUG: Disable parallel for Find Neighborhoods and Fix Copy Error In Fill Bad Data (#694) - [2023-09-20]
- CONDA: Various fixes to allow packaging of python bindings. (#691) - [2023-09-18]
- ENH: Add More Resampling Modes To Resample Image Geometry (#689) - [2023-09-13]
- Filter: SurfaceNets Surface Meshing Implementation (#619) - [2023-09-13]
- ENH: Improve the Tuple Count validation error reporting (#684) - [2023-09-12]
- ENH: Python binding documentation and autogeneration (#673) - [2023-09-12]
- TEST: Update PartitionGeometry Unit test to use Catch2 GENERATOR sections (#666) - [2023-09-02]
- ENH: Calculate "Modal Histogram Bin Ranges" Array In Find Attribute Array Statistics (#686) - [2023-09-02]
- ENH: Changed STLFileReader filter to allow user setting of FaceNormals. (#680) - [2023-09-02]
- FILTER: Import Fiji Montage (#667) - [2023-09-01]