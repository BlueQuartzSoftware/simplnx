==========================
Find Feature Phases Binary
==========================


Group (Subgroup)
================

Generic (Misc)

Description
===========

This **Filter** assigns an **Ensemble** Id number to binary data. The *true* **Cells** will be **Ensemble** 1, and
*false* **Cells** will be **Ensemble** 0. This **Filter** is generally useful when the **Cell Ensembles** were not known
ahead of time. For example, if an image is segmented into precipitates and non-precipitates, this **Filter** will assign
the precipitates to **Ensemble** 1, and the non-precipitates to **Ensemble** 0.

Parameters
==========

None

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
| Cell Attribute Array        | Mask         | bool     | (1)        | Specifies if the **Cell** is to be counted in   |
|                             |              |          |            | the algorithm                                   |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Matrix            | Cell Data    | A        | N/A        | The *Cell Data* **Attribute Matrix** of the     |
|                             | Attribute    | ttribute |            | **Image Geometry** where the *Binary Phases     |
|                             | Matrix       | Matrix   |            | Array* will be created                          |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Created Objects
===============

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Feature Attribute Array     | Binary       | int32_t  | (1)        | Specifies to which **Ensemble** each            |
|                             | Feature      |          |            | **Feature** belongs                             |
|                             | Phases Array |          |            |                                                 |
|                             | Name         |          |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
