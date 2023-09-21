==========================================
Find Feature Neighbor C-Axis Misalignments
==========================================


Group (Subgroup)
================

Statistics (Crystallographic)

Description
===========

This **Filter** determines, for each **Feature**, the C-axis misalignments with the **Features** that are in contact
with it. The C-axis misalignments are stored as a list (for each **Feature**) of angles (in degrees).

Notes
-----

**NOTE:** Only features with identical phase values and a crystal structure of **Hexagonal_High** will be calculated. If
two features have different phase values or a crystal structure that is *not* Hexagonal_High then a value of NaN is set
for the misorientation.

Parameters
==========

+------------------------------+------------------------------+--------------------------------------------------------+
| Name                         | Type                         | Description                                            |
+==============================+==============================+========================================================+
| Find Average Misalignment    | bool                         | Whether the *average* of the C-axis misalignments with |
| Per Feature                  |                              | the neighboring **Features** should be stored for each |
|                              |                              | \**Feature                                             |
+------------------------------+------------------------------+--------------------------------------------------------+

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
| Feature Attribute Array     | CAxisMisa    | List of  | (1)        | List of the C-axis misalignment angles (in      |
|                             | lignmentList | float    |            | degrees) with the contiguous neighboring        |
|                             |              |          |            | **Features** for a given \**Feature             |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | AvgCAxisM    | float    | (1)        | Number weighted average of neighbor C-axis      |
|                             | isalignments |          |            | misalignments. Only created if *Find Average    |
|                             |              |          |            | Misalignment Per Feature* is checked            |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Example Pipelines
=================

Combo-EBSD-osc_r0c0

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
