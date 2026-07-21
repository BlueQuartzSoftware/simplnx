:topic: Reference Frame Notes

.. index::
   triple: DREAM3D-NX; Reference Frame; Notes

Reference Frame Notes
###############################


When dealing with **orientation** data such as Euler angles, Quaternions
and Rodrigues vectors, the user will often need to ensure that the
proper convention is followed to transform data from the **sample**
reference frame to the **crystal** reference frame. DREAM3D-NX makes the
following assumptions about reference frames:

-  Only Passive Rotations are used, i.e., rotations between reference
   frames;
-  A rotations always transforms the **sample** reference frame to the
   **crystal** reference frame.

DREAM3D-NX can represent orientations in seven different forms:

+---------------+----+------------------------------------------------+
| Name          | A  | Number of Components                           |
|               | bb |                                                |
|               | r. |                                                |
+===============+====+================================================+
| Eulers        | e  | 3                                              |
+---------------+----+------------------------------------------------+
| Rodrigues     | r  | 4                                              |
+---------------+----+------------------------------------------------+
| Orientation   | o  | 3x3                                            |
| Matrix        |    |                                                |
+---------------+----+------------------------------------------------+
| Quaternion    | q  | 4 (< x, y, z > w) Note the order of the data.  |
|               |    | Vector-Scalar                                  |
+---------------+----+------------------------------------------------+
| Axis-Angle    | a  | 4 (< ax0, ax1, ax2 >, w)                       |
+---------------+----+------------------------------------------------+
| Cubochoric    | c  | 3                                              |
+---------------+----+------------------------------------------------+
| Homochoric    | h  | 3                                              |
+---------------+----+------------------------------------------------+

Master Table of Available Conversions
======================================

In many cases, a direct transformation is available from one
representation to another; in some cases, an intermediate representation
is used, e.g., from homochoric (h) to Euler (e), DREAM3D-NX first
transforms to an axis angle pair (a), then to an orientation matrix (o),
and finally to Euler angles.

+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| From/To            | Euler | Orientation Matrix | Axis Angle | Rodrigues | Quaternion | Homochoric | Cubochoric | Stereographic |
+====================+=======+====================+============+===========+============+============+============+===============+
| Euler              | -     | X                  | X          | X         | X          | a          | ah         | q             |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| Orientation Matrix | X     | --                 | X          | e         | X          | a          | ah         | q             |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| Axis Angle         | o     | X                  | --         | X         | X          | X          | h          | q             |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| Rodrigues          | o     | a                  | X          | --        | a          | X          | h          | q             |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| Quaternion         | X     | X                  | X          | X         | --         | X          | h          | q             |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| Homochoric         | ao    | a                  | X          | a         | a          | --         | X          | q             |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| Cubochoric         | hao   | ha                 | h          | ha        | ha         | X          | --         | q             |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+
| Stereographic      | a     | a                  | X          | X         | a          | X          | hc         | --            |
+--------------------+-------+--------------------+------------+-----------+------------+------------+------------+---------------+

Filters to Assist the User
============================

When importing EBSD data through the various readers (.ang, .ctf files),
there are a number of filters that can assist the user in performing the
necessary transformations. The following is a non-exhaustive list of
filters that will help the user perform various manipulations on the
orientation data.

**NOTE:** When importing data generated by non-EBSD instruments, it is
very important to know what conventions were used to represent the
rotations/orientations; an incorrect interpretation of these conventions
**will** lead to incorrect results.

+----------+-------------------------------------------------------------+
| Filter   | Discussion                                                  |
+==========+=============================================================+
| Rotate   | This will rotate the Crystal Reference frame. Input         |
| Euler    | Orientation Data is possibly changed                        |
| R        |                                                             |
| eference |                                                             |
| Frame    |                                                             |
+----------+-------------------------------------------------------------+
| Rotate   | This will rotate the actual data into new locations in an   |
| Sample   | Image Geometry array                                        |
| R        |                                                             |
| eference |                                                             |
| Frame    |                                                             |
+----------+-------------------------------------------------------------+
| R        | DREAM3D-NX expects Rodrigues vectors to have 4 components,  |
| odrigues | where the 4th component is the length of the vector. This   |
| C        | filter will convert 3 component Rodrigues vectors to the    |
| onvertor | internal 4 component convention                             |
+----------+-------------------------------------------------------------+
| Generate | This uses an input set of Quaternions and generates the     |
| Qu       | conjugate quaternions. A copy of the input array is made    |
| aternion | and changes are made to that array                          |
| C        |                                                             |
| onjugate |                                                             |
+----------+-------------------------------------------------------------+
| Generate | This will generate the transpose of an array of 3x3         |
| Ori      | matrices. A copy of the input array is made and changes     |
| entation | are made to that array                                      |
| Matrix   |                                                             |
| T        |                                                             |
| ranspose |                                                             |
+----------+-------------------------------------------------------------+

Examples of what can go wrong
================================

In this section, we show an IPF generated from data imported from a
LabDCT experiment. The LabDCT input data provided 3 component Rodrigues
vectors which were converted to the 4 component internal format. The
LabDCT program generated the IPF on the left, and DREAM3D-NX generated the
IPF on the right, after application of the Rodrigues vector conversion.
Note that while some grain colors match up, most do not. This is because
the LabDCT output was generated with the convention that a rotation
transforms the **Crystal** reference frame to the **Sample** reference
frame, *which is the opposite of what DREAM3D-NX expects*.

+----------------------------------+-------------------------------------+
| LabDCT Generated IPF 001 Colors  | DREAM3D-NX Generated IPF 001 Colors |
+==================================+=====================================+
| |Correctly imported orientation  | |Incorrectly imported orientation   |
| data|                            | data|                               |
+----------------------------------+-------------------------------------+

While both transformations (Crystal-to-Sample and Sample-to-Crystal) are
passive rotations (they convert reference frames into one another), the
transformations are each other’s inverse. DREAM3D-NX always expects a
rotation angle to be positive and in the interval [0,pi]; thus,
computing the inverse of a rotation involves changing the sign of the
rotation axis unit vector. Depending on the rotation representation, the
inverse of a rotation can then be implemented by computation of:

-  the negative of the 3-component Rodrigues vector (use the Attribute
   Array Calculator filter) [note that this also works for the
   homochoric and cubochoric representations, and for the axis vector of
   the axis-angle pair]
-  the conjugate of the Quaternion (Generate Quaternion Conjugate
   filter)
-  the transpose of the Orientation Matrix (Generate Orientation Matrix
   Transpose filter)

When the input LabDCT Rodrigues vectors are multiplied by -1 **before**
conversion to the internal 4-component format, then the following IPF is
generated, showing perfect agreement with the original LabDCT IPF map.
This example shows that it is absolutely crucial to know and understand
the conventions that were used to generate orientation data in any type
of diffraction experiment **and** to ensure that the correct conversions
are applied upon loading the data into DREAM3D-NX.

.. figure:: Images/AlCu-485-Transformed.png
   :alt: Correctly imported orientation data to conform to DREAM3D-NX’s assumptions

   Correctly imported orientation data to conform to DREAM3D-NX’s
   assumptions

.. |Correctly imported orientation data| image:: Images/AlCu-485-LabDCT.png
.. |Incorrectly imported orientation data| image:: Images/AlCu-485-WRONG.png

Crystallographic Information
==============================

+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  # | Point Group (H–M) | Rotation Point Group | Space Group No(s). | Schoenflies   | Crystal system | Laue class  | Laue Ops         |
+====+===================+======================+====================+===============+================+=============+==================+
|  1 | 1                 | 1                    | 1                  | C₁            | Triclinic      | (\bar{1})   | TriclinicOps     |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  2 | (\bar{1})         | 1                    | 2                  | C(_i)         | Triclinic      | (\bar{1})   |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  3 | 2                 | 2                    | 3–5                | C₂            | Monoclinic     | 2/m         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  4 | m                 | 1                    | 6–9                | C(_s)         | Monoclinic     | 2/m         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  5 | 2/m               | 2                    | 10–15              | C(_{2h})      | Monoclinic     | 2/m         | MonoclinicOps    |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  6 | 222               | 222                  | 16–24              | D₂            | Orthorhombic   | mmm         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  7 | mm2               | 2                    | 25–46              | C(_{2v})      | Orthorhombic   | mmm         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  8 | mmm               | 222                  | 47–74              | D(_{2h})      | Orthorhombic   | mmm         | OrthorhombicOps  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
|  9 | 4                 | 4                    | 75–80              | C₄            | Tetragonal     | 4/m         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 10 | (\bar{4})         | 2                    | 81–82              | S₄            | Tetragonal     | 4/m         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 11 | 4/m               | 4                    | 83–88              | C(_{4h})      | Tetragonal     | 4/m         | TetragonalLowOps |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 12 | 422               | 422                  | 89–98              | D₄            | Tetragonal     | 4/mmm       |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 13 | 4mm               | 4                    | 99–110             | C(_{4v})      | Tetragonal     | 4/mmm       |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 14 | (\bar{4}2m)       | 222                  | 111–122            | D(_{2d})      | Tetragonal     | 4/mmm       |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 15 | 4/mmm             | 422                  | 123–142            | D(_{4h})      | Tetragonal     | 4/mmm       | TetragonalOps    |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 16 | 3                 | 3                    | 143–146            | C₃            | Trigonal       | (\bar{3})   |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 17 | (\bar{3})         | 3                    | 147–148            | C(_{3i}) (S₆) | Trigonal       | (\bar{3})   | TrigonalLowOps   |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 18 | 32                | 32                   | 149–155            | D₃            | Trigonal       | (\bar{3}m)  |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 19 | 3m                | 3                    | 156–161            | C(_{3v})      | Trigonal       | (\bar{3}m)  |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 20 | (\bar{3}m)        | 32                   | 162–167            | D(_{3d})      | Trigonal       | (\bar{3}m)  | TrigonalOps      |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 21 | 6                 | 6                    | 168–173            | C₆            | Hexagonal      | 6/m         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 22 | (\bar{6})         | 3                    | 174                | C(_{3h})      | Hexagonal      | 6/m         |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 23 | 6/m               | 6                    | 175–176            | C(_{6h})      | Hexagonal      | 6/m         | HexagonalLowOps  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 24 | 622               | 622                  | 177–182            | D₆            | Hexagonal      | 6/mmm       |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 25 | 6mm               | 6                    | 183–186            | C(_{6v})      | Hexagonal      | 6/mmm       |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 26 | (\bar{6}m2)       | 32                   | 187–190            | D(_{3h})      | Hexagonal      | 6/mmm       |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 27 | 6/mmm             | 622                  | 191–194            | D(_{6h})      | Hexagonal      | 6/mmm       | HexagonalOps     |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 28 | 23                | 23                   | 195–199            | T             | Cubic          | m(\bar{3})  |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 29 | m(\bar{3})        | 23                   | 200–206            | T(_h)         | Cubic          | m(\bar{3})  | CubicLowOps      |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 30 | 432               | 432                  | 207–214            | O             | Cubic          | m(\bar{3})m |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 31 | (\bar{4}3m)       | 23                   | 215–220            | T(_d)         | Cubic          | m(\bar{3})m |                  |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
| 32 | m(\bar{3})m       | 432                  | 221–230            | O(_h)         | Cubic          | m(\bar{3})m | CubicOps         |
+----+-------------------+----------------------+--------------------+---------------+----------------+-------------+------------------+
