======================================
Find Feature Reference Misorientations
======================================


Group (Subgroup)
================

Statistics (Crystallographic)

Description
===========

This **Filter** calculates the misorientation angle between each **Cell** within a **Feature** and a *reference
orientation* for that **Feature**. The user can choose the *reference orientation* to be used for the **Features** from
a drop-down menu. The options for the *reference orientation* are the average orientation of the **Feature** or the
orientation of the **Cell** that is furthest from the *boundary* of the **Feature**.

Note: the average orientation of the **Feature** is a typical choice, but if the **Feature** has undergone plastic
deformation and the amount of lattice rotation developed is of interest, then it may be more reasonable to use the
orientation *near the center* of the **Feature** as it may not have rotated and thus serve as a better *reference
orientation*.

Parameters
==========

===================== =========== ===========================================================================
Name                  Type        Description
===================== =========== ===========================================================================
Reference Orientation Enumeration Specifies the *reference orientation* to use when comparing to each \**Cell
===================== =========== ===========================================================================

Required Geometry
=================

Image

Required Objects
================

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Cell Attribute Array        | FeatureIds   | int32_t  | (1)        | Specifies to which **Feature** each **Cell**    |
|                             |              |          |            | belongs                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Cell Attribute Array*\*     | Phases       | int32_t  | (1)        | Specifies to which **Ensemble** each **Cell**   |
|                             |              |          |            | belongs                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Cell Attribute Array        | Quats        | float    | (4)        | Specifies the orientation of the **Cell** in    |
|                             |              |          |            | quaternion representation                       |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | AvgQuats     | float    | (4)        | Specifies the average orientation of the        |
|                             |              |          |            | **Feature** in quaternion representation        |
|                             |              |          |            | (<x,y,z>, w). Only required if the *reference   |
|                             |              |          |            | orientation* is selected to be the average of   |
|                             |              |          |            | the \**Feature                                  |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Cell Attribute Array        | GBEuclid     | float    | (1)        | Distance the **Cells** are from the *boundary*  |
|                             | eanDistances |          |            | of the **Feature** they belong to. Only         |
|                             |              |          |            | required if the *reference orientation* is      |
|                             |              |          |            | selected to be the orientation at the           |
|                             |              |          |            | **Feature** centroid                            |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Ensemble Attribute Array    | Cryst        | uint32_t | (1)        | Enumeration representing the crystal structure  |
|                             | alStructures |          |            | for each \**Ensemble                            |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Created Objects
===============

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Cell Attribute Array        | Feature      | float    | (1)        | Misorientation angle (in degrees) between       |
|                             | ReferenceMis |          |            | **Cell’s** orientation and the reference        |
|                             | orientations |          |            | orientation of the **Feature** that owns that   |
|                             |              |          |            | \**Cell                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | F            | float    | (1)        | Average of the *FeatureReferenceMisorientation* |
|                             | eatureAvgMis |          |            | values for all of the **Cells** that belong to  |
|                             | orientations |          |            | the \**Feature                                  |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Example Pipelines
=================

-  

   (5) SmallIN100 Crystallographic Statistics

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
