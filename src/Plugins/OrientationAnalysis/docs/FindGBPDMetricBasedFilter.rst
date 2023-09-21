=================================
Find GBPD (Metric-Based Approach)
=================================


Group (Subgroup)
================

Statistics (Crystallographic)

Description
===========

This **Filter** computes the grain boundary plane distribution (GBPD) like that shown in Fig. 1. It should be noted that
most GBPDs presented so far in literature were obtained using a method based on partition of the grain boundary space
into bins, similar to that implemented in the *Find GBCD* **Filter**. This **Filter** calculates the GBPD using an
alternative approach adapted from the one included in the *Find GBCD (Metric-based Approach)* **Filter** and described
by K. Glowinski and A. Morawiec in `Analysis of experimental grain boundary distributions based on boundary-space
metrics, Metall. Mater. Trans. A 45, 3189-3194 (2014) <http://link.springer.com/article/10.1007%2Fs11661-014-2325-y>`__.
Briefly, the GBPD is probed at evenly distributed sampling directions (similarly to *Find GBCD (Metric-based Approach)*
**Filter**) and areas of mesh segments with their normal vectors deviated by less than a limiting angle ρp from a given
direction are summed. If *n*\ S is the number of crystal symmetry transformations, each boundary plane segment is
represented by up to 4 × *n*\ S equivalent vectors, and all of them are processed. It is enough to sample the
distribution at directions corresponding to the standard stereographic triangle (or, in general, to a fundamental region
corresponding to a considered crystallographic point group); values at remaining points are obtained based on crystal
symmetries. After summing the boundary areas, the distribution is normalized. First, the values at sampling vectors are
divided by the total area of all segments. Then, in order to express the distribution in the conventional units, i.e.,
multiples of random distribution (MRDs), the obtained fractional values are divided by the volume *v* = (*A* nS) / (4π),
where *A* is the area of a spherical cap determined by ρp.

.. figure:: Images/FindGBPDMetricBased_example.png
   :alt: Fig. 1: GBPD obtained for Small IN100 with the limiting distance set to 7° and with triangles adjacent to
   triple lines removed. Units are MRDs.

   Fig. 1: GBPD obtained for Small IN100 with the limiting distance set to 7° and with triangles adjacent to triple
   lines removed. Units are MRDs.

This **Filter** also calculates statistical errors of the distributions using the formula

ε = ( *f* *n* *v* )1/2, where ε

is the relative error of the distribution function at a given point, *f* is the value of the function at that point, and
*n* stands for the number of grain boundaries (**not** the number of mesh triangles) in the considered network. The
errors can be calculated either as their absolute values, i.e., ε × *f* or as relative errors, i.e., 100% × ε. The
latter are computed in a way that if the relative error exceeds 100%, it is rounded down to 100%.

See also the documentation for `Find GBCD (Metric-based Approach) <../FindGBCDMetricBasedFilter/index.html>`__
**Filter** for additional information.

Parameters
==========

+---------------------------+---------------------------+-------------------------------------------------------------+
| Name                      | Type                      | Description                                                 |
+===========================+===========================+=============================================================+
| Phase of Interest         | int32_t                   | Index of the **Ensemble** for which to compute GBPD;        |
|                           |                           | boundaries having grains of this phase on both its sides    |
|                           |                           | will only be taken into account                             |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Limiting Distance         | float                     | ρp as defined above                                         |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Number of Sampling Points | int32_t                   | The **approximate** number of sampling directions           |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Exclude Triangles         | bool                      | Only interiors of **Faces** are included in GBPD            |
| Directly Neighboring      |                           |                                                             |
| Triple Lines              |                           |                                                             |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Output Distribution File  | File Path                 | The output file path (extension .dat, GMT format)           |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Output Distribution       | File Path                 | The output file path (extension .dat, GMT format)           |
| Errors File               |                           |                                                             |
+---------------------------+---------------------------+-------------------------------------------------------------+
| Save Relative Errors      | bool                      | What type of errors to save (see above description for more |
| Instead of Their Absolute |                           | detail)                                                     |
| Values                    |                           |                                                             |
+---------------------------+---------------------------+-------------------------------------------------------------+

Required Geometry
=================

Image + Triangle

Required Objects
================

+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Kind           | Default Name                     | Type                          | Comp Dims           | De        |
|                |                                  |                               |                     | scription |
+================+==================================+===============================+=====================+===========+
| Ensemble       | CrystalStructures                | uint32_t                      | (1)                 | En        |
| Attribute      |                                  |                               |                     | umeration |
| Array          |                                  |                               |                     | rep       |
|                |                                  |                               |                     | resenting |
|                |                                  |                               |                     | the       |
|                |                                  |                               |                     | crystal   |
|                |                                  |                               |                     | structure |
|                |                                  |                               |                     | for each  |
|                |                                  |                               |                     | \*        |
|                |                                  |                               |                     | *Ensemble |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Feature        | AvgEulerAngles                   | float                         | (3)                 | Three     |
| Attribute      |                                  |                               |                     | angles    |
| Array          |                                  |                               |                     | defining  |
|                |                                  |                               |                     | the       |
|                |                                  |                               |                     | or        |
|                |                                  |                               |                     | ientation |
|                |                                  |                               |                     | of the    |
|                |                                  |                               |                     | **        |
|                |                                  |                               |                     | Feature** |
|                |                                  |                               |                     | in Bunge  |
|                |                                  |                               |                     | c         |
|                |                                  |                               |                     | onvention |
|                |                                  |                               |                     | (Z-X-Z)   |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Feature        | Phases                           | int32_t                       | (1)                 | Specifies |
| Attribute      |                                  |                               |                     | to which  |
| Array          |                                  |                               |                     | phase     |
|                |                                  |                               |                     | each      |
|                |                                  |                               |                     | **        |
|                |                                  |                               |                     | Feature** |
|                |                                  |                               |                     | belongs   |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Face Attribute | FaceLabels                       | int32_t                       | (2)                 | Specifies |
| Array          |                                  |                               |                     | which     |
|                |                                  |                               |                     | **F       |
|                |                                  |                               |                     | eatures** |
|                |                                  |                               |                     | are on    |
|                |                                  |                               |                     | either    |
|                |                                  |                               |                     | side of   |
|                |                                  |                               |                     | each      |
|                |                                  |                               |                     | \**Face   |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Face Attribute | FaceNormals                      | double                        | (3)                 | Specifies |
| Array          |                                  |                               |                     | the       |
|                |                                  |                               |                     | normal of |
|                |                                  |                               |                     | each      |
|                |                                  |                               |                     | \**Face   |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Face Attribute | FaceAreas                        | double                        | (1)                 | Specifies |
| Array          |                                  |                               |                     | the area  |
|                |                                  |                               |                     | of each   |
|                |                                  |                               |                     | \**Face   |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Feature Face   | FaceLabels                       | int32_t                       | (2)                 | Specifies |
| Attribute      |                                  |                               |                     | to which  |
| Array*\*       |                                  |                               |                     | phase     |
|                |                                  |                               |                     | each      |
|                |                                  |                               |                     | **Face    |
|                |                                  |                               |                     | Feature** |
|                |                                  |                               |                     | belongs   |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+
| Vertex         | NodeTypes                        | int8_t                        | (1)                 | Specifies |
| Attribute      |                                  |                               |                     | the type  |
| Array*\*       |                                  |                               |                     | of node   |
|                |                                  |                               |                     | in the    |
|                |                                  |                               |                     | Geometry  |
+----------------+----------------------------------+-------------------------------+---------------------+-----------+

Format of Output Files
======================

Output files are formatted to be readable by GMT plotting program. The first line is always “0.0 0.0 0.0 0.0”. Each of
the remaining lines contains three numbers. The first two columns are angles (in degrees) describing a given sampling
direction; let us denote them *col*\ 1 and *col*\ 2, respectively. The third column is either the value of the GBCD (in
MRD) for that direction or its error (in MRD or %, depending on user’s selection). If you use other software, you can
retrive spherical angles θ and φ of the sampling directions in the following way:

θ = 90° - *col*\ 1

φ = *col*\ 2

Then, the directions are given as [ sin θ × cos φ , sin θ × sin φ , cos θ ].

Feedback
========

In the case of any questions, suggestions, bugs, etc., please feel free to email the author of this **Filter** at
kglowinski *at* ymail.com

References
==========

[1] K. Glowinski and A. Morawiec, Analysis of experimental grain boundary distributions based on boundary-space metrics,
Metall. Mater. Trans. A 45, 3189-3194 (2014)

Example Pipelines
=================

License & Copyright
===================

Please see the description file distributed with this **Plugin**.

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
