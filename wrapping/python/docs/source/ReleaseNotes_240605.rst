Release Notes 24.06.05
======================

With this release we are moving towards a YYMMDD style of versioning. The `simplnx` library is under activate development and while we strive to maintain a stable API bugs are
found that necessitate the changing of the API.

Version 24.06.05
-----------------


API Changes & Additions 24.06.05
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Many of the filters found in simplnx had their names changed to end with "Filter"
- All ITKImageProcessing filters had their name changed to end with "Filter"
- Many of the parameter keys had their names changed to bring consistency to the API. This will manifest as the named arguments to XXXXFilter.execute(...) function will have changed. Please check the latest documentation for the proper arguments to use.

Change Log 24.06.05
^^^^^^^^^^^^^^^^^^^^

- ENH: Update docs for filters that change FeatureIds to warn user of invalid feature attribute matrix (#980) [2024-05-28]
- ENH/BUG: Mask Option Type Clean-Up and Out-of-Core Compatibility Update (#978) [2024-05-26]
- ENH: Add Python Plugin Environment File to generated plugin (#970) [2024-05-25]
- ENH: Update docs for Orientation Correlation/Comparison filters. (#979) [2024-05-25]
- BUG: ReadCSVFileFilter-Replace illegal characters in headers. (#977) [2024-05-25]
- ENH: Update ComputeSlipTransmissionMetrics and ComputeBoundaryStrengths docs (#974) [2024-05-24]
- FILTER: Create Peregrine Reader Filter (Python) (#965) [2024-05-24]
- ENH: Consistency in default parameter values for DataPath and DataObjectName (#972) [2024-05-24]
- DOC: Fix capitalization issue in the documentation for the GitHub link. (#975) [2024-05-23]
- DOC: Fix various documentation bugs. (#973) [2024-05-23]
- COMP: Update GitHub CI runner to use macOS 12 for x86 (#971) [2024-05-21]
- ENH: Factor Out EliminateDuplicateNodes and FillDataArray, Add Sub-Volume Reading, and Improve Error Reporting (#964) [2024-05-20]
- ENH: Rename Filters that start with Find/Generate/Calculate to Compute (#956) [2024-05-20]
- BUG: Fix crashes in SIMPL Json to SIMPLNX filter (#959) [2024-05-18]
- ENH: Add Default Extension (.csv) to WriteFeatureDataAsCSVFilter (#963) [2024-05-17]
- DOC: Add Python documentation for all Actions. (#951) [2024-05-17]
- ENH: CliReaderFilter-Add edge-bounding box intersection options (#960) [2024-05-16]
- ENH: Merge Twins Rework and Segment Features Cleanup (#955) [2024-05-14]
- Python feedback changes (#942) [2024-05-13]
- BUG: Fix parameter key for Import HDF5 Parameter (#953) [2024-05-08]
- BUG: Pipeline and Filter human facing label cleanup (#934) [2024-05-06]
- COMP: Turn warnings for inconsistent-missing-override into errors (#950) [2024-05-04]
- ENH: Python CLI Reader Bounding Box (#946) [2024-05-03]
- ENH: Moved Result handling outside of AtomicFile (#941) [2024-05-03]
- ENH: Moved StringUtilities helper functions to detail namespace (#940) [2024-05-03]
- BUG: Fix ITKImageProcessing filter UUIDs. (#945) [2024-05-02]
- ENH: RenameAction and Filter Add Overwrite Option, Result Changes (#912) [2024-05-02]
- BUG: WriteAsciiData-Add preflight checks for empty paths. (#938) [2024-05-01]
- BUG: ReadCTFData-Remove phase=0 value adjustment. (#937) [2024-05-01]
- COMP: ReadVtkStructuredPoints-Fix compiler warning about over flow in memset (#932) [2024-04-30]
- ENH: All filter's class names end with "Filter". (#931) [2024-04-29]
- BUG: Fix STLFileReader crash bug (#930) [2024-04-28]
- BUG: Filters that delete NeighborLists from the DataStructure send strong warning messages. (#926) [2024-04-28]
- API: DataPath::replaceName convenience method. (#928) [2024-04-25]
- BUG: Fix HDF5 implicit copy crashes. (#924) [2024-04-23]
