===============================
Find Attribute Array Statistics
===============================


Group (Subgroup)
================

DREAM3D Review (Statistics)

Description
===========

This **Filter** computes a variety of statistics for a given scalar array. The currently available statistics are array
length, minimum, maximum, (arithmetic) mean, median, mode, standard deviation, and summation; any combination of these
statistics may be computed by this **Filter**. Any scalar array, of any primitive type, may be used as input. The type
of the output arrays depends on the kind of statistic computed:

======================= ===================================
Statistic               Primitive Type
======================= ===================================
Histogram               uint64 (of user set component size)
Length                  signed 64-bit integer
Minimum                 same type as input
Maximum                 same type as input
Mean                    double
Median                  double
Mode                    same type as input
Standard Deviation      double
Summation               double
Standardized            double
Number of Unique Values signed 32-bit integer
======================= ===================================

The user may optionally use a mask to specify points to be ignored when computing the statistics; only points where the
supplied mask is *true* will be considered when computing statistics. Additionally, the user may select to have the
statistics computed per **Feature** or **Ensemble** by supplying an Ids array. For example, if the user opts to compute
statistics per **Feature** and selects an array that has 10 unique **Feature** Ids, then this **Filter** will compute 10
sets of statistics (e.g., find the mean of the supplied array for each **Feature**, find the total number of points in
each **Feature** (the length), etc.).

The input array may also be *standardized*, meaning that the array values will be adjusted such that they have a mean of
0 and unit variance. This *Standardize Data* option requires the selection of both the *Find Mean* and *Find Standard
Deviation* options. The standardized data will be saved as a new array object stored in the same **Attribute Matrix** as
the input array. Note that if the *Standardize Data* option is selected, the mean and standard deviation values created
by this **Filter** reflect the mean and standard deviation of the *original* array; the new standardized array has a
mean of 0 and unit variance. The standardized array will be computed in double precision. If the statistics are being
computed per **Feature** or **Ensemble**, then the array values are standardized according to the mean and standard
deviation *for each* **Feature/Ensemble**. For example, if 5 unique **Features** were being analyzed and *Standardize
Data* was selected, then the array values for **Feature** 1 would be standardized according to the mean and standard
deviation for **Feature** 1, then the array values for **Feature** 2 would be standardized according to the mean and
standard deviation for **Feature** 2, and so on for the remaining **Features**.

The user must select a destination **Attribute Matrix** in which the computed statistics will be stored. If electing to
*Compute Statistics Per Feature/Ensemble*, then a reasonable selection for this array is the **Feature/Ensemble**
**Attribute Matrix** associated with the supplied **Feature/Ensemble** Ids. However, the only requirement is that the
number of columns in the selected destination **Attribute Matrix** match the number of **Features/Ensembles** specified
by the supplied Id array. This requirement is enforced at run time. If computing statistics for the entire input array,
then only one value is computed per statistic; therefore, the arrays produced only contain one value. In this case, the
destination **Attribute Matrix** should only contain 1 tuple. If such a **Generic Attribute Matrix** does not exist, it
can be created.

Special operations occur for certain statistics if the supplied array is of type *bool* (for example, a mask array
produced `when thresholding <@ref%20multithresholdobjects>`__). The length, minimum, maximum, median, mode, and
summation are computed as normal (although the resulting values may be platform dependent). The mean and standard
deviation for a boolean array will be true if there are more instances of true in the array than false. If *Standardize
Data* is chosen for a boolean array, no actual modifications will be made to the input. These operations for boolean
inputs are chosen as a basic convention, and are not intended be representative of true boolean logic.

**Note**: If *Find Histogram* is on AND *Compute Statistics Per Feature/Ensemble* is on, then any features that have the
exact same value throughout the entire feature will have its first histogram bin set to the total count of feature
values. All other bins will be 0.

Parameters
==========

+--------------------------------------------+----------+-------------------------------------------------------------+
| Name                                       | Type     | Description                                                 |
+============================================+==========+=============================================================+
| Find Histogram                             | bool     | Whether to compute the histogram of the input array         |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Histogram Min Value                        | double   | Min cutoff value for histogram                              |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Histogram Max Value                        | double   | Max cutoff value for histogram                              |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Number of Bins                             | int32_t  | Number of bins in histogram                                 |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Use Full Range For Histogram               | bool     | If true, ignore min and max and use min and max from array  |
|                                            |          | upon which histogram is computed                            |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Length                                | bool     | Whether to compute the length of the input array            |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Minimum                               | bool     | Whether to compute the minimum of the input array           |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Maximum                               | bool     | Whether to compute the maximum of the input array           |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Mean                                  | bool     | Whether to compute the arithmetic mean of the input array   |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Median                                | bool     | Whether to compute the median of the input array            |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Mode                                  | bool     | Whether to compute the mode of the input array. The input   |
|                                            |          | array may have multiple modes, so the modes are stored in a |
|                                            |          | NeighborList.                                               |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Standard Deviation                    | bool     | Whether to compute the standard deviation of the input      |
|                                            |          | array                                                       |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Summation                             | bool     | Whether to compute the summation of the input array         |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Find Number of Unique Values               | bool     | Whether to compute the number of unique values in the input |
|                                            |          | array                                                       |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Use Mask                                   | bool     | Whether to use a boolean mask array to ignore certain       |
|                                            |          | points flagged as *false* from the statistics               |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Compute Statistics Per Feature/Ensemble    | bool     | Whether the statistics should be computed on a              |
|                                            |          | **Feature/Ensemble** basis                                  |
+--------------------------------------------+----------+-------------------------------------------------------------+
| Standardize Data                           | bool     | Whether the input array should be standardized to have mean |
|                                            |          | of 0 and unit variance; *Find Mean* and *Find Standard      |
|                                            |          | Deviation* must be selected to use this option              |
+--------------------------------------------+----------+-------------------------------------------------------------+

Required Geometry
=================

None

Required Objects
================

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Any \**Attribute Array      | None         | Any      | (1)        | Input **Attribute Array** for which to compute  |
|                             |              |          |            | statistics                                      |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | None         | int32_t  | (1)        | Specifies to which **Feature/Ensemble** each    |
|                             |              |          |            | point in the input **Attribute Array** belongs, |
|                             |              |          |            | if *Compute Statistics Per Feature/Ensemble* is |
|                             |              |          |            | checked                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Mask         | bool     | (1)        | Specifies if the point is to be counted in the  |
|                             |              |          |            | statistics, if *Use Mask* is checked            |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Destination \**Attribute    | None         | Any      | N/A        | Attribute Matrix*\* in which to store the       |
| Matrix                      |              |          |            | computed statistics                             |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+

Created Objects
===============

+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Kind                        | Default Name | Type     | Comp. Dims | Description                                     |
+=============================+==============+==========+============+=================================================+
| Attribute Array             | Feat         | bool     | (1)        | Indicates, for each feature, whether or not the |
|                             | ure-Has-Data |          |            | feature actually contains any data (only usable |
|                             |              |          |            | when *Compute Statistics Per Feature/Ensemble*  |
|                             |              |          |            | is turned on). This array is especially useful  |
|                             |              |          |            | to help determine whether or not the outputted  |
|                             |              |          |            | statistics are actually valid or not for a      |
|                             |              |          |            | given feature.                                  |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Histogram    | uint64   | (Number of | Histogram of the input array, if *Find          |
|                             |              |          | Bins)      | Histogram* is checked                           |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Most         | uint64   | (2)        | Most populated bin from the histogram of the    |
|                             | Populated    |          |            | input array, if *Find Histogram* is checked.    |
|                             | Bin          |          |            | First component is the bin index (0-based),     |
|                             |              |          |            | second component is the number of values in the |
|                             |              |          |            | bin.                                            |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Neighbor List*\*            | Modal        | float    | (1)        | Ranges, in physical units, of histogram bin(s)  |
|                             | Histogram    |          |            | that contain mode value(s). *Find Histogram*,   |
|                             | Bin Ranges   |          |            | *Find Modal Histogram Bin Ranges*, and *Find    |
|                             |              |          |            | Mode* must all be checked. The min/max values   |
|                             |              |          |            | are stored in the neighbor list arrays from bin |
|                             |              |          |            | 0 - bin N (Example: [bin0min, bin0max, bin1min, |
|                             |              |          |            | bin1max, … binNmin, binNmax])                   |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Length       | int64_t  | (1)        | Length of the input array, if *Find Length* is  |
|                             |              |          |            | checked                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Minimum      | same as  | (1)        | Minimum of the input array, if *Find Minimum*   |
|                             |              | input    |            | is checked                                      |
|                             |              | \**A     |            |                                                 |
|                             |              | ttribute |            |                                                 |
|                             |              | Array    |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Maximum      | same as  | (1)        | Maximum of the input array, if *Find Maximum*   |
|                             |              | input    |            | is checked                                      |
|                             |              | \**A     |            |                                                 |
|                             |              | ttribute |            |                                                 |
|                             |              | Array    |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Mean         | double   | (1)        | Arithmetic mean of the input array, if *Find    |
|                             |              |          |            | Mean* is checked                                |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Median       | double   | (1)        | Median of the input array, if *Find Median* is  |
|                             |              |          |            | checked                                         |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Neighbor List*\*            | Mode         | same as  | (1)        | Modes of the input array, if *Find Mode* is     |
|                             |              | input    |            | checked                                         |
|                             |              | \**A     |            |                                                 |
|                             |              | ttribute |            |                                                 |
|                             |              | Array    |            |                                                 |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Standard     | double   | (1)        | Standard deviation of the input array, if *Find |
|                             | Deviation    |          |            | Standard Deviation* is checked                  |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Summation    | double   | (1)        | Summation of the input array, if *Find          |
|                             |              |          |            | Summation* is checked                           |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Standardized | double   | (1)        | Standardized version of the input array, if     |
|                             |              |          |            | *Standardize Data* is checked                   |
+-----------------------------+--------------+----------+------------+-------------------------------------------------+
| Attribute Array             | Number of    | int32_t  | (1)        | The number of unique values in the input array, |
|                             | Unique       |          |            | if *Find Number of Unique Values* is checked    |
|                             | Values       |          |            |                                                 |
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
