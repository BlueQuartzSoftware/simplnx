Release Notes 1.2.7
===================

The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found the necessitate the changing of the API.

Version 1.2.7
-------------

- Documentation has been updated
- Examples updated to use new API


API Changes & Additions 1.2.7
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- The ColorTableParameter API has changed. Please see either the developer or user documentation for more details.
- A few filters have changed their name
- DataPath has had more API take from parts of PathLib. See the documentation for the new API additions
- Node based geometries allow the resizing of their internal data structures using the `resize_*` methods.

Change Log 1.2.7
^^^^^^^^^^^^^^^^^^^^

- ENH: Update error messages from DataArrayIO and StringArrayIO (#901) [d769fce3]
- DOC: Add example for GeneratedFileListParameter in Python (#902) [7021f182]
- BUG: Fixed issue with failing print causing filter to fail (#899) [5da9e786]
- DOC: Update Python documentation with improved navigation (#895) [c59e375c]
- ENH: GeneratePythonPlugin can generate batch/shell init file. (#896) [8f1a45df]
- ENH: Misc doc and filter updates for ITK, ExtractInternalSurfaces and SurfaceNets (#898) [d43cc50b]
- FILTER : ExtractPipelineToFile added (#897) [8b748077]
- FILTER: Add H5OINAReader (#700) [a51dd5f3]
- BUG: Small bugs, doc fixes, StopWatch class added (#894) [327e4b65]
- BUG: StringUtilities::split-Add Additional Edge Case Checks(#893) [3a9c87d8]
- BUG: Rename GenerateFaceMisorientationColors to GenerateFeatureFaceMisorientations (#889) [5ed1cd93]
- BUG: Return invalid Result<> if size of directory is requested. (#891) [219471be]
- BUG/ENH: StringUtilities Split Function Updates (#892) [381aecc4]
- ENH: Report total installed memory on some memory allocation errors (#887) [0343a96c]
- ENH: Add support for getting the pipeline json object from a dream3d file (#890) [6a80a1be]
- ENH: Add checks to ensure vertex cropping bounds are not the same. (#888) [e9f0542a]
- ENH: Forward error messages when creating Node geometries. (#885) [eed0c9b4]
- FILT: ITKImageProcessing - Add new batch of filters (#884) [daea37d6]
- BUG: Fix python plugin generation codes (#881) [60a81213]
- ENH: Initialize Data - Increased Variance in Random Generation for Floats (#880) [023c9896]
- BUG: Fix resetting of the DataStructure::Id when importing from a file. (#879) [8e4e8f98]
- ENH: GeneratePythonFilter-Crashing fixed, Generation Fixed, headers split (#877) [fa92713c]
- BUG: Find Array Statistics (#878) [1645a22f]
- ENH: ReadMHAFile-Allow user to transpose stored matrix and other UI improvements (#871) [f9034641]
- BUG: Fix accepted input file extension types for ConvertHexGridToSquarGridFilter (#875) [eb588250]
- ENH: Update Generate Color Table Filter and Parameter (#866) [d4264ca1]
- ENH: CombineSTLFiles-Add option to label the faces and vertices based on a file index (#873) [93ca6a33]
- PY: Use consistent simplnx alias, tweak parameter declaration sections. (#872) [5340383d]
- BUG: Fix issue where spacing would be set to 1,1,1 (#870) [c8f5f0cf]
- FILTER: Rename 'Contouring' to 'FlyingEdges3D' (#869) [b6a50195]
- PY: DataPath API additions, Example OpenCV filter included. (#868) [1b1d2d8d]
- PYTHON FILTER: Contour Statistics (#865) [da8428bd]
- BUG: ReadH5Ebsd-Fix issue creating proper sized Attribute Matrix for Ensemble Data (#863) [c089bc23]
- DOCS: Implement python developer documentation including filter examples and a tutorial (#837) [2a7b94cd]
- PY: Add support for Python 3.12 with HDF5 1.14. (#861) [5b24757e]