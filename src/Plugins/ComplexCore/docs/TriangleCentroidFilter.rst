===========================
Generate Triangle Centroids
===========================


Group (Subgroup)
================

Surface Meshing (Misc)

Description
===========

This **Filter** computes the centroid of each **Triangle** in a **Triangle Geometry** by calculating the average
position of all 3 **Vertices** that make up the **Triangle**.

Parameters
==========

None

Required Geometry
=================

Triangle

Required Objects
================

None

Created Objects
===============

==================== ============= ======= ========= ======================================
Kind                 Default Name  Type    Comp Dims Description
==================== ============= ======= ========= ======================================
Face Attribute Array FaceCentroids float32 (1)       Specifies the centroid of each \**Face
==================== ============= ======= ========= ======================================

Example Pipelines
=================

-  Triangle_Face_Data_Demo.d3dpipeline

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
