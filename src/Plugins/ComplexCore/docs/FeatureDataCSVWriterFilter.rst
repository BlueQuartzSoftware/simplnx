==============================
Write Feature Data as CSV File
==============================


Group (Subgroup)
================

IO (Output)

Description
===========

This **Filter** writes the data associated with each **Feature** to a file name specified by the user in *CSV* format.
Every array in the **Feature** map is written as a column of data in the *CSV* file. The user can choose to also write
the neighbor data. Neighbor data are data arrays that are associated with the neighbors of a **Feature**, such as: list
of neighbors, list of misorientations, list of shared surface areas, etc. These blocks of info are written after the
scalar data arrays. Since the number of neighbors is variable for each **Feature**, the data is written as follows (for
each **Feature**): Id, number of neighbors, value1, value2,…valueN.

Example Output
--------------

The *CSV* file:

::

   5414
   Feature_ID,Active,AvgEuler_0,AvgEuler_1,AvgEuler_2,AvgQuats_0,AvgQuats_1,AvgQuats_2,AvgQuats_3,NumNeighbors2,Phases
   1,1,4.7367158,0.40832964,1.1398145,-0.045759138,0.19751814,0.19773535,0.95905864,6,1
   2,1,3.315398,0.74920797,3.2314293,0.3655816,0.015357733,-0.12232465,0.92257845,2,1
   3,1,3.8660862,0.20160854,2.8935425,0.088968314,0.047029439,-0.23477697,0.96682608,2,1
   4,1,1.2659892,0.89199567,5.6040425,-0.24293284,-0.35644585,-0.26093844,0.86362094,1,1
   5,1,0.96062404,0.576078,5.2663703,-0.15617171,-0.23729226,0.026934497,0.95842445,26,1

The *CSV* file with Write Neighbor Data checked:

::

   5414
   Feature_ID,Active,AvgEuler_0,AvgEuler_1,AvgEuler_2,AvgQuats_0,AvgQuats_1,AvgQuats_2,AvgQuats_3,NumNeighbors2,Phases
   1,1,4.7367158,0.40832964,1.1398145,-0.045759138,0.19751814,0.19773535,0.95905864,6,1
   2,1,3.315398,0.74920797,3.2314293,0.3655816,0.015357733,-0.12232465,0.92257845,2,1
   3,1,3.8660862,0.20160854,2.8935425,0.088968314,0.047029439,-0.23477697,0.96682608,2,1
   4,1,1.2659892,0.89199567,5.6040425,-0.24293284,-0.35644585,-0.26093844,0.86362094,1,1
   5,1,0.96062404,0.576078,5.2663703,-0.15617171,-0.23729226,0.026934497,0.95842445,26,1
   .
   .
   .
   Feature_ID,NumNeighbors,NeighborList2
   1,6,229,607,639,2432,3280,5057
   2,2,518,3933
   3,2,1561,3145
   4,1,886
   5,26,61,224,278,454,786,923,1119,1137,1478,1517,1525,1651,1812,1814,2227,2233,2731,2750,2907,2930,3175,3548,3619,4492,4791,5010

Parameters
==========

+---------------------------------------+---------------------------------------+---------------------------------------+
| Name                                  | Type                                  | Description                           |
+=======================================+=======================================+=======================================+
| Output File                           | File Path                             | The output .csv file path             |
+---------------------------------------+---------------------------------------+---------------------------------------+
| Write Neighbor Data                   | bool                                  | Whether to write the **Feature**      |
|                                       |                                       | neighbor data                         |
+---------------------------------------+---------------------------------------+---------------------------------------+
| Write Number of Features Line         | bool                                  | Write the total number of features as |
|                                       |                                       | the first line. Writing this line may |
|                                       |                                       | interfere with standard CSV parsers.  |
|                                       |                                       | Default=ON                            |
+---------------------------------------+---------------------------------------+---------------------------------------+
| Delimiter                             | choice                                | The delimiter character used to parse |
|                                       |                                       | the file (Takes *COMMA*, *SEMICOLON*, |
|                                       |                                       | *COLON*, *TAB*, or *SPACE*)           |
+---------------------------------------+---------------------------------------+---------------------------------------+

Required Geometry
=================

Not Applicable

Required Objects
================

=============== ============ ======= ========== ===============================================
Kind            Default Name Type    Comp. Dims Description
=============== ============ ======= ========== ===============================================
AttributeMatrix None         Feature N/A        AttributeMatrix*\* that holds the data to write
=============== ============ ======= ========== ===============================================

Created Objects
===============

None

Example Pipelines
=================

None

License & Copyright
===================

Please see the description file distributed with this **Plugin**

DREAM3DNX Help
==============

Check out our GitHub community page at `DREAM3DNX-Issues <https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues>`__ to
report bugs, ask the community for help, discuss features, or get help from the developers.
