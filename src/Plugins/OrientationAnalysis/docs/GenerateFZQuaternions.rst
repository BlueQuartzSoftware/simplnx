=====================
GenerateFZQuaternions
=====================


Group (Subgroup)
================

OrientationAnalysis (OrientationAnalysis)

Description
===========

This **Filter** reduces input orientations (Quaternions) into the fundamental zone for the given Laue group.

The following figures represent a BCC Interstitial steel EBSD data set courtesy of
`1 <N.%20Allain-Bonasso,%20F.%20Wagner,%20S.%20Berbenni,%20D.P.%20Field,%20A%20study%20of%20the%20heterogeneity%20of%20plastic%20deformation%20in%20IF%20steel%20by%20EBSD,%20Materials%20Science%20and%20Engineering:%20A,%20Volume%20548,%2030%20June%202012,%20Pages%2056-63,%20ISSN%200921-5093,%20http://dx.doi.org/10.1016/j.msea.2012.03.068.>`__.

+-----------------------------------------------------------+-----------------------------------------------------------+
| |image1|                                                  | |image2|                                                  |
+-----------------------------------------------------------+-----------------------------------------------------------+
| Original IPF Colored dataset                              | IPF Color Legend for m-3m Laue group                      |
+-----------------------------------------------------------+-----------------------------------------------------------+
| |image3|                                                  | |image4|                                                  |
+-----------------------------------------------------------+-----------------------------------------------------------+
| EBSD Dataset **before** the filter. IPF Colors are using  | EBSD Dataset **after** the filter. IPF Colors are using a |
| a reference direction of < 001 >. The data is visualized  | reference direction of < 001 >. The data is visualized in |
| in the 3D stereographic unit spheres with a superimposed  | the 3D stereographic unit spheres with a superimposed     |
| Rodrigues Fundamental Zone.                               | Rodrigues Fundamental Zone.                               |
+-----------------------------------------------------------+-----------------------------------------------------------+

Parameters
==========

+---------------------------------------+---------------------------------------+---------------------------------------+
| Name                                  | Type                                  | Description                           |
+=======================================+=======================================+=======================================+
| Apply to Good Elements Only (Bad      | bool                                  | Whether to assign a black color to    |
| Elements Will Be Black)               |                                       | “bad” \**Elements                     |
+---------------------------------------+---------------------------------------+---------------------------------------+

Required Geometry
=================

Not Applicable

Required Objects
================

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Element Attribute Array     | Quaternions  | float    | (4)        | Quaternions ordered as (< x, y, z >, w)         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Element Attribute Array     | Phases       | int32_t  | (1)        | Phase Id specifying the phase of the \**Element |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Element Attribute Array     | Mask         | bool     | (1)        | Used to define **Elements** as *good* or *bad*. |
|                             |              |          |            | Only required if *Apply to Good Elements Only   |
|                             |              |          |            | (Bad Elements Will Be Black)* is checked        |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Ensemble Attribute Array    | Cryst        | uint32_t | (1)        | Enumeration representing the crystal structure  |
|                             | alStructures |          |            | for each **Ensemble**.                          |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

**Crystal Structure Table**

======================= ============== ================================
String Name             Internal Value Laue Name
======================= ============== ================================
Hexagonal_High          0              Hexagonal-High 6/mmm
Cubic_High              1              Cubic Cubic-High m3m
Hexagonal_Low           2              Hexagonal-Low 6/m
Cubic_Low               3              Cubic Cubic-Low m3 (Tetrahedral)
Triclinic               4              Triclinic -1
Monoclinic              5              Monoclinic 2/m
OrthoRhombic            6              Orthorhombic mmm
Tetragonal_Low          7              Tetragonal-Low 4/m
Tetragonal_High         8              Tetragonal-High 4/mmm
Trigonal_Low            9              Trigonal-Low -3
Trigonal_High           10             Trigonal-High -3m
UnknownCrystalStructure 999            Undefined Crystal Structure
======================= ============== ================================

Created Objects
===============

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Element Attribute Array     | FZ           | float    | (4)        | The Quaternion that represents an orientation   |
|                             | Quaternions  |          |            | within the fundamental zone for each \**Element |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

References
==========

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this plugin.

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.

.. |image1| image:: Images/ReadH5Ebsd_Right.png
.. |image2| image:: Images/Cubic_m3m_IPFLegend.png
.. |image3| image:: Images/GenerateFZQuats_1.png
.. |image4| image:: Images/GenerateFZQuats_2.png
