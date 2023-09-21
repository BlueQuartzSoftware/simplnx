===========
Merge Twins
===========


Group (Subgroup)
================

Reconstruction (Grouping)

Description
===========

*THIS FILTER ONLY WORKS ON CUBIC-HIGH (m3m) Laue Classes.*

This **Filter** groups neighboring **Features** that are in a twin relationship with each other (currently only FCC σ =
3 twins). The algorithm for grouping the **Features** is analogous to the algorithm for segmenting the **Features** -
only the average orientation of the **Features** are used instead of the orientations of the individual **Elements**.
The user can specify a tolerance on both the *axis* and the *angle* that defines the twin relationship (i.e., a
tolerance of 1 degree for both tolerances would allow the neighboring **Features** to be grouped if their misorientation
was between 59-61 degrees about an axis within 1 degree of <111>, since the Sigma 3 twin relationship is 60 degrees
about <111>).

Parameters
==========

+------------------------------+------------------------------+--------------------------------------------------------+
| Name                         | Type                         | Description                                            |
+==============================+==============================+========================================================+
| Axis Tolerance (Degrees)     | float                        | Tolerance allowed when comparing the axis part of the  |
|                              |                              | axis-angle representation of the misorientation        |
+------------------------------+------------------------------+--------------------------------------------------------+
| Angle Tolerance (Degrees)    | float                        | Tolerance allowed when comparing the angle part of the |
|                              |                              | axis-angle representation of the misorientation        |
+------------------------------+------------------------------+--------------------------------------------------------+

Required Geometry
=================

Not Applicable

Required Objects
================

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Feature Attribute Array     | NonContigu   | List of  | (1)        | List of non-contiguous neighbors for each       |
|                             | ousNeighbors | int32_t  |            | **Feature**. Only needed if *Use Non-Contiguous |
|                             |              |          |            | Neighbors* is checked                           |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | NeighborList | List of  | (1)        | List of neighbors for each \**Feature           |
|                             |              | int32_t  |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Element Attribute Array     | FeatureIds   | int32_t  | (1)        | Specifies to which **Feature** each **Element** |
|                             |              |          |            | belongs                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | Phases       | int32_t  | (1)        | Specifies to which **Ensemble** each            |
|                             |              |          |            | **Feature** belongs                             |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | AvgQuats     | float    | (4)        | Specifies the average orientation of the        |
|                             |              |          |            | **Feature** in quaternion representation        |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Ensemble Attribute Array    | Cryst        | uint32_t | (1)        | Enumeration representing the crystal structure  |
|                             | alStructures |          |            | for each \**Ensemble                            |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Created Objects
===============

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Element Attribute Array     | ParentIds    | int32_t  | (1)        | Specifies to which *parent* each **Element**    |
|                             |              |          |            | belongs                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Matrix            | Ne           | Feature  | N/A        | Created **Feature Attribute Matrix** name       |
|                             | wFeatureData |          |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | ParentIds    | int32_t  | (1)        | Specifies to which *parent* each **Feature**    |
|                             |              |          |            | belongs                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Feature Attribute Array     | Active       | bool     | (1)        | Specifies if the **Feature** is still in the    |
|                             |              |          |            | sample (*true* if the **Feature** is in the     |
|                             |              |          |            | sample and *false* if it is not). At the end of |
|                             |              |          |            | the **Filter**, all **Features** will be        |
|                             |              |          |            | *Active*                                        |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

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
