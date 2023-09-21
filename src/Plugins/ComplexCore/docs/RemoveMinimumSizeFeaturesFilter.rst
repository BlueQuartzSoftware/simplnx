============
Minimum Size
============


Group (Subgroup)
================

Processing (Cleanup)

Description
===========

This **Filter** removes **Features** that have a total number of **Cells** below the minimum threshold defined by the
user. Entering a number larger than the largest **Feature** generates an *error* (since all **Features** would be
removed). Hence, a choice of threshold should be carefully be chosen if it is not known how many **Cells** are in the
largest **Features**. After removing all the small **Features**, the remaining **Features** are isotropically coarsened
to fill the gaps left by the small **Features**.

The **Filter** can be run in a mode where the minimum number of neighbors is applied to a single **Ensemble**. The user
can select to apply the minimum to one specific **Ensemble**.

Notes
=====

If any features are removed **and** the Cell Feature AttributeMatrix contains any *NeighborList* data arrays those
arrays will be **REMOVED** because those lists are now invalid. Re-run the *Find Neighbors* filter to re-create the
lists.

Parameters
==========

+---------------------------+---------------------------+-------------------------------------------------------------+
| Name                      | Type                      | Description                                                 |
+===========================+===========================+=============================================================+
| Minimum Allowed Feature   | int32_t                   | Number of **Cells** that must be present in the **Feature** |
| Size                      |                           | for it to remain in the sample                              |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Apply to Single Phase     | bool                      | Tells the Filter whether to apply minimum to single         |
| Only                      |                           | ensemble or all ensembles                                   |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Phase Index               | int32_t                   | Which **Ensemble** to apply minimum to. Only needed if      |
|                           |                           | *Apply to Single Phase Only* is checked                     |
+---------------------------+---------------------------+-------------------------------------------------------------+

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
| Feature Attribute Array     | Phases       | int32_t  | (1)        | Specifies to which **Ensemble** each            |
|                             |              |          |            | **Feature** belongs. Only required if *Apply to |
|                             |              |          |            | Single Phase Only* is checked                   |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | NumCells     | int32_t  | (1)        | Specifies the number of **Cells** belonging to  |
|                             |              |          |            | each \**Feature                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Created Objects
===============

None

Example Pipelines
=================

-  

   (10) SmallIN100 Full Reconstruction

-  

   (6) SmallIN100 Postsegmentation Processing

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
