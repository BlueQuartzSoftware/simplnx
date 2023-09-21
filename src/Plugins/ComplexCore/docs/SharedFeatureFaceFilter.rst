==========================
Generate Triangle Face Ids
==========================


Group (Subgroup)
================

Surface Meshing (Connectivity/Arrangement)

Description
===========

This **Filter** assigns a unique Id to each **Triangle** in a **Triangle Geometry** that represents the *unique
boundary* on which that **Triangle** resides. For example, if there were only two **Features** that shared one boundary,
then the **Triangles** on that boundary would be labeled with a single unique Id. This procedure creates *unique groups*
of **Triangles**, which themselves are a set of **Features**. Thus, this **Filter** also creates a **Feature Attribute
Matrix** for this new set of **Features**, and creates **Attribute Arrays** for their Ids and number of **Triangles**.
This process can be considered a **segmentation** where each unique id is the shared boundary between two features.

Because the algorithm is essentially segmenting the triangles based on the unique combination of face labels, the filter
will also generate a Feature level Attribute Matrix and place two additional *DataArrays* into that feature attribute
matrix that store the following information:

1. The number of triangles in each unique boundary
2. The pair of Face Label values that made up the unique boundary.

Generated Feature Boundaries *with* Randomization
-------------------------------------------------

.. figure:: Images/SharedFeaturFace_1.png
   :alt: Example Surface Mesh Coloring By Feature Face Id

   Example Surface Mesh Coloring By Feature Face Id

Generated Feature Boundaries *without* Randomization
----------------------------------------------------

.. figure:: Images/SharedFeaturFace_2.png
   :alt: Example Surface Mesh Coloring By Feature Face Id

   Example Surface Mesh Coloring By Feature Face Id

--------------

Parameters
==========

+-------------------------+--------+------------------------------------------------------------------------------------+
| Name                    | Type   | Description                                                                        |
+=========================+========+====================================================================================+
| Randomize Feature       | b      | Should the final ‘FeatureFaceId’ Array be randomized. This can aid in              |
| Values                  | oolean | visualization                                                                      |
+-------------------------+--------+------------------------------------------------------------------------------------+

Required Geometry
=================

Triangle

Required Objects
================

+-----------------------+------------+-------+-----------+------------------------------------------------------------+
| Kind                  | Default    | Type  | Comp Dims | Description                                                |
|                       | Name       |       |           |                                                            |
+=======================+============+=======+===========+============================================================+
| Face Attribute Array  | FaceLabels | in    | (2)       | Specifies which **Features** are on either side of each    |
|                       |            | t32_t |           | \**Face                                                    |
+-----------------------+------------+-------+-----------+------------------------------------------------------------+

Created Objects
===============

+--------------------+-----------+---------+--------+---------------------------------------------------------------+
| Kind               | Default   | Type    | Comp   | Description                                                   |
|                    | Name      |         | Dims   |                                                               |
+====================+===========+=========+========+===============================================================+
| Face Attribute     | Feat      | int32_t | (1)    | Specifies to which **Feature** each **Face** belongs          |
| Array*\*           | ureFaceId |         |        |                                                               |
+--------------------+-----------+---------+--------+---------------------------------------------------------------+
| Attribute          | FaceFe    | Face    | N/A    | Created **Feature Attribute Matrix** name                     |
| Matrix*\*          | atureData | Feature |        |                                                               |
+--------------------+-----------+---------+--------+---------------------------------------------------------------+
| Feature Attribute  | F         | int32_t | (2)    | Specifies which *original* **Features** are on either side of |
| Array              | aceLabels |         |        | each *new* \**Feature                                         |
+--------------------+-----------+---------+--------+---------------------------------------------------------------+
| Feature Attribute  | Num       | int32_t | (1)    | Number of **Triangles** in each **Feature**                   |
| Array              | Triangles |         |        |                                                               |
+--------------------+-----------+---------+--------+---------------------------------------------------------------+

Example Pipelines
=================

“(03) Small IN100 Mesh Statistics.d3dpipeline”

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
