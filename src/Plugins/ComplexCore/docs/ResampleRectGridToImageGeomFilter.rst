==============================
Resample RectGrid To ImageGeom
==============================


Group (Subgroup)
================

Sampling (Sampling)

Description
===========

This **Filter** will resample an existing RectilinearGrid onto a regular grid (Image Geometry) and copy cell data into
the newly created Image Geometry Data Container during the process.

The sampling algorithm is a simple “last one wins” algorithm due to the likely hood that interpolating data is probably
not the correct algorithm to bring in data to the new Image Geometry.

The algorithm logic is thus: If the ImageGeometry cell would contain multiple RectilinearGrid cells, then we select from
the covered cells the cell with the largest X, Y and Z index and copy that data into the Image Geometry Cell Attribute
Matrix.

The user can select which cell attribute matrix data arrays will be copied into the newly created Image Geometry Cell
Attribute Matrix.

Parameters
==========

+---------------------------------------+---------------------------------------+---------------------------------------+
| Name                                  | Type                                  | Description                           |
+=======================================+=======================================+=======================================+
| Input Rectilinear Grid                | DataPath                              | Path to the RectGrid Geometry to be   |
|                                       |                                       | re-sampled                            |
+---------------------------------------+---------------------------------------+---------------------------------------+
| Selected Cell Attribute Arrays to     | Array of (DataPath)                   | Rect Grid Cell Data to possibly copy  |
| Copy                                  |                                       |                                       |
+---------------------------------------+---------------------------------------+---------------------------------------+
| Image Geometry Voxel Dimensions       | 3 x Int32                             | The image geometry voxel dimensions   |
|                                       |                                       | in which to re-sample the rectilinear |
|                                       |                                       | grid geometry                         |
+---------------------------------------+---------------------------------------+---------------------------------------+

Required Geometry
=================

Rectilinear Grid Geometry

Created Objects
===============

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Image Geometry              | Image        | N/A      | N/A        | Path to the created Image Geometry              |
|                             | Geometry     |          |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Matrix            | Cell Data    | Eleme    | N/A        | The name of the cell data Attribute Matrix      |
|                             |              | nt/Featu |            | created with the Image Geometry                 |
|                             |              | re/Ensem |            |                                                 |
|                             |              | ble/etc. |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| El                          | Copied from  | in       | (1         | Cell level arrays copied over from the input to |
| ement/Feature/Ensemble/etc. | the input    | t32_t/fl | )/(3)/etc. | the resampled geometry                          |
| Attribute Array             | Data         | oat/etc. |            |                                                 |
|                             | Container    |          |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this plugin.

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
