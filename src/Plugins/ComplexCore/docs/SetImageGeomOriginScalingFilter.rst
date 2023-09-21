=====================================
Set Origin & Spacing (Image Geometry)
=====================================


Group (Subgroup)
================

Core (Spatial)

Description
===========

This **Filter** changes the origin and/or the spacing of an **Image Geometry**. For example, if the current origin is at
(0, 0, 0) and the user would like the origin to be (10, 4, 8), then the user should enter the following input values:

-  X Origin: 10
-  Y Origin: 4
-  Z Origin: 8

Parameters
==========

============== ========== ================================================================
Name           Type       Description
============== ========== ================================================================
Origin         float (3x) New origin for the \**Image Geometry
Spacing        float (3x) New spacing for the \**Image Geometry
Change Origin  bool       Whether a new origin should be applied to the \**Image Geometry
Change Spacing bool       Whether a new spacing should be applied to the \**Image Geometry
============== ========== ================================================================

Required Geometry
=================

Image

Required Objects
================

============== ============ ==== ========== ===================================================
Kind           Default Name Type Comp. Dims Description
============== ============ ==== ========== ===================================================
Data Container None         N/A  N/A        Data Container with an **Image Geometry** to modify
============== ============ ==== ========== ===================================================

Created Objects
===============

None

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
