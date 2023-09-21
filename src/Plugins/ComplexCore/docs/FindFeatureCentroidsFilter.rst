======================
Find Feature Centroids
======================


Group (Subgroup)
================

Generic (Misc)

Description
===========

This **Filter** calculates the *centroid* of each **Feature** by determining the average X, Y, and Z position of all the
**Cells** belonging to the **Feature**. Note that **Features** that intersect the outer surfaces of the sample will
still have *centroids* calculated, but they will be *centroids* of the truncated part of the **Feature** that lies
inside the sample.

Parameters
==========

None

Required Geometry
=================

Image

Required Objects
================

==================== ============ ======= ========== ====================================================
Kind                 Default Name Type    Comp. Dims Description
==================== ============ ======= ========== ====================================================
Cell Attribute Array FeatureIds   int32_t (1)        Specifies to which **Feature** each **Cell** belongs
==================== ============ ======= ========== ====================================================

Created Objects
===============

======================= ============ ===== ========== =================================================
Kind                    Default Name Type  Comp. Dims Description
======================= ============ ===== ========== =================================================
Feature Attribute Array Centroids    float (3)        X, Y, Z coordinates of **Feature** center of mass
======================= ============ ===== ========== =================================================

Example Pipelines
=================

-  

   (1) SmallIN100 Morphological Statistics

-  InsertTransformationPhase

-  

   (6) SmallIN100 Synthetic

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
