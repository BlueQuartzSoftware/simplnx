=====================================
Find Feature Neighbor Misorientations
=====================================


Group (Subgroup)
================

Statistics (Crystallographic)

Description
===========

This **Filter** determines, for each **Feature**, the misorientations with each of the **Features** that are in contact
with it. The misorientations are stored as a list (for each **Feature**) of angles (in degrees). The axis of the
misorientation is not stored by this **Filter**.

The user can also calculate the average misorientation between the feature and all contacting features.

Notes
-----

**NOTE:** Only features with identical crystal structures will be calculated. If two features have different crystal
structures then a value of NaN is set for the misorientation.

Parameters
==========

+---------------------------+---------------------------+-------------------------------------------------------------+
| Name                      | Type                      | Description                                                 |
+===========================+===========================+=============================================================+
| Find Average              | bool                      | Specifies if the *average* of the misorienations with the   |
| Misorientation Per        |                           | neighboring **Features** should be stored for each          |
| Feature                   |                           | \**Feature                                                  |
+---------------------------+---------------------------+-------------------------------------------------------------+

Required Geometry
=================

Not Applicable

Required Objects
================

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Feature Attribute Array     | N            | List of  | (1)        | List of the contiguous neighboring **Features** |
|                             | eighborLists | int32_t  |            | for a given \**Feature                          |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | AvgQuats     | float    | (4)        | Defines the average orientation of the          |
|                             |              |          |            | **Feature** in quaternion representation        |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | Phases       | int32_t  | (1)        | Specifies to which **Ensemble** each            |
|                             |              |          |            | **Feature** belongs                             |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Ensemble Attribute Array    | Cryst        | uint32_t | (1)        | Enumeration representing the crystal structure  |
|                             | alStructures |          |            | for each \**Ensemble                            |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Created Objects
===============

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Feature Attribute Array     | Misorie      | List of  | (1)        | List of the misorientation angles with the      |
|                             | ntationLists | float    |            | contiguous neighboring **Features** for a given |
|                             |              |          |            | \**Feature                                      |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | AvgMi        | float    | (1)        | Number weighted average of neighbor             |
|                             | sorientation |          |            | misorientations. Only created if *Find Average  |
|                             |              |          |            | Misorientation Per Feature* is checked          |
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
