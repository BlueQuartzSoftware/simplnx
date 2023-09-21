=============================
Vtk Rectilinear Grid Exporter
=============================


Group (Subgroup)
================

I/O Filters

Description
===========

This Filter reads the **Feature** and phase ids together with image parameters required by Vtk to an output file named
by the user. The file is used to generate the image of the **Features** and phases of the **Features**.

Parameters
==========

===================== ===================
Name                  Type
===================== ===================
Output File           Output File
Write **Feature** Ids Boolean (On or Off)
Write Phase Ids       Boolean (On or Off)
Write Band Contrasts  Boolean (On or Off)
Write KAM Value       Boolean (On or Off)
Write GAM Values      Boolean (On or Off)
Write LMG Values      Boolean (On or Off)
Write Good \**Cells   Boolean (On or Off)
Write IPF Colors      Boolean (On or Off)
Write Binary File     Boolean (On or Off)
===================== ===================

Required DataContainers
=======================

Voxel

Required Objects
================

+--------------+----------------------------------+--------------------------------+---------------------+-----------+
| Type         | Default Name                     | Description                    | Comment             | Filters   |
|              |                                  |                                |                     | Known to  |
|              |                                  |                                |                     | Create    |
|              |                                  |                                |                     | Data      |
+==============+==================================+================================+=====================+===========+
| Cell         | GrainIds                         | Ids (ints) that specify to     | Values should be    | Segment   |
|              |                                  | which **Feature** each         | present from        | Features  |
|              |                                  | **Cell** belongs.              | segmentation of     | (Misori   |
|              |                                  |                                | experimental data   | entation, |
|              |                                  |                                | or synthetic        | C-Axis    |
|              |                                  |                                | generation and      | Misori    |
|              |                                  |                                | cannot be           | entation, |
|              |                                  |                                | determined by this  | Scalar)   |
|              |                                  |                                | filter. Not having  | (Reconst  |
|              |                                  |                                | these values will   | ruction), |
|              |                                  |                                | result in the       | Read Dx   |
|              |                                  |                                | filter to fail/not  | File      |
|              |                                  |                                | execute.            | (IO),     |
|              |                                  |                                |                     | Read Ph   |
|              |                                  |                                |                     | File      |
|              |                                  |                                |                     | (IO),     |
|              |                                  |                                |                     | Pack      |
|              |                                  |                                |                     | Primary   |
|              |                                  |                                |                     | Phases    |
|              |                                  |                                |                     | (S        |
|              |                                  |                                |                     | yntheticB |
|              |                                  |                                |                     | uilding), |
|              |                                  |                                |                     | Insert    |
|              |                                  |                                |                     | Pr        |
|              |                                  |                                |                     | ecipitate |
|              |                                  |                                |                     | Phases    |
|              |                                  |                                |                     | (S        |
|              |                                  |                                |                     | yntheticB |
|              |                                  |                                |                     | uilding), |
|              |                                  |                                |                     | Establish |
|              |                                  |                                |                     | Matrix    |
|              |                                  |                                |                     | Phase     |
|              |                                  |                                |                     | (         |
|              |                                  |                                |                     | Synthetic |
|              |                                  |                                |                     | Building) |
+--------------+----------------------------------+--------------------------------+---------------------+-----------+

Created Objects
===============

None

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
