====================
Compute Feature Rect
====================


Group (Subgroup)
================

Reconstruction (Reconstruction)

Description
===========

This **Filter** computes the XYZ minimum and maximum coordinates for each **Feature** in a segmentation. This data can
be important for finding the smallest encompassing volume. This values are given in **Pixel** coordinates.

== = = = = =
\  0 1 2 3 4
== = = = = =
0  0 0 1 0 0
1  0 0 1 1 0
2  0 1 1 1 1
3  0 0 1 1 0
4  0 0 0 0 0
== = = = = =

If the example matrix above which represents a single feature where the feature ID = 1, the output of the filter would
be:

::

   X Min = 1
   Y Min = 0
   Z Min = 0

   X Max = 4
   Y Max = 3
   Z Max = 0

Parameters
==========

N/A

Required Geometry
=================

N/A

Required Objects
================

+-------------------------+---------------------+-------------------------+------------+-------------------------+
| Kind                    | Default Name        | Type                    | Comp. Dims | Description             |
+=========================+=====================+=========================+============+=========================+
| FeatureIds              | FeatureIdsArrayName | int32_t                 | (1)        |                         |
+-------------------------+---------------------+-------------------------+------------+-------------------------+
| Feature Attribute       | N/A                 | Feature AttributeMatrix | N/A        | The path to the cell    |
| Matrix                  |                     |                         |            | feature \**Attribute    |
|                         |                     |                         |            | Matrix                  |
+-------------------------+---------------------+-------------------------+------------+-------------------------+

Created Objects
===============

======================= ============ ====== ========== ==================================
Kind                    Default Name Type   Comp. Dims Description
======================= ============ ====== ========== ==================================
Feature Attribute Array FeatureRect  uint32 (6)        Xmin, Ymin, Zmin, Xmax, Ymax, Zmax
======================= ============ ====== ========== ==================================

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this plugin.

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
