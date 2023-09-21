================
Write ASCII Data
================


Group (Subgroup)
================

IO (Output) (Write) (Export) (Text) (CSV) (ASCII)

Description
===========

This **Filter** accepts DataArray(s) as input, extracts the data, creates the file(s), and writes it out according to
parameter choices

Parameters
==========

======================= ====================== ============================================================
Name                    Type                   Decision
======================= ====================== ============================================================
Output Type             OutputSytle enum class Whether data is printed to one file or multiple
Output Path             Filesystem::path       Directory to store printed array files
File Extension          string                 The file extension used for generated files
Maximum Tuples Per Line int32                  Number of tuples printed before newline character is printed
Delimiter               Delimiter enum class   ASCII character used to seperate values
======================= ====================== ============================================================

Required Geometry
=================

None

Required Objects
================

========= ========================== ========= ========= ====================================================
Kind      Default Name               Type      Comp Dims Description
========= ========================== ========= ========= ====================================================
DataArray Attribute Arrays to Export DataArray any       Specifies **DataArray** to have their values printed
========= ========================== ========= ========= ====================================================

Created Objects
===============

None

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
