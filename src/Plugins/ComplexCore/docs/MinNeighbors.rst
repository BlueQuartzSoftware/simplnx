===========================
Minimum Number of Neighbors
===========================


Group (Subgroup)
================

Processing (Cleanup)

Description
===========

This **Filter** sets the minimum number of contiguous neighboring **Features** a **Feature** must have to remain in the
structure. Entering zero results in nothing changing. Entering a number larger than the maximum number of neighbors of
any **Feature** generates an *error* (since all **Features** would be removed). The user needs to proceed conservatively
here when choosing the value for the minimum to avoid accidentally exceeding the maximum. After **Features** are removed
for not having enough neighbors, the remaining **Features** are *coarsened* iteratively, one **Cell** per iteration,
until the gaps left by the removed **Features** are filled. Effectively, this is an isotropic **Feature** growth in the
regions around removed **Features**.

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
| Minimum Number Neighbors  | int32_t                   | Number of neighbors a **Feature** must have to remain as a  |
|                           |                           | \**Feature                                                  |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Apply to Single Phase     | bool                      | Whether to apply minimum to single ensemble or all          |
|                           |                           | ensembles                                                   |
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
| Feature Attribute Array     | NumNeighbors | int32_t  | (1)        | Number of contiguous neighboring **Features**   |
|                             |              |          |            | for each \**Feature                             |
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
