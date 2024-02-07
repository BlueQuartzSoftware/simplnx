# Find Attribute Array Statistics

## Group (Subgroup)

DREAM3D Review (Statistics)

## Description

This **Filter** computes a variety of statistics for a given scalar array.  The currently available statistics are array length, minimum, maximum, (arithmetic) mean, median, mode, standard deviation, and summation; any combination of these statistics may be computed by this **Filter**.  Any scalar array, of any primitive type, may be used as input.  The type of the output arrays depends on the kind of statistic computed:

| Statistic               | Primitive Type                      |
|-------------------------|-------------------------------------|
| Histogram               | uint64 (of user set component size) |
| Length                  | signed 64-bit integer               |
| Minimum                 | same type as input                  |
| Maximum                 | same type as input                  |
| Mean                    | double                              |
| Median                  | double                              |
| Mode                    | same type as input                  |
| Standard Deviation      | double                              |
| Summation               | double                              |
| Standardized            | double                              |
| Number of Unique Values | signed 32-bit integer               |

The user may optionally use a mask to specify points to be ignored when computing the statistics; only points where the supplied mask is *true* will be considered when computing statistics.  Additionally, the user may select to have the statistics computed per **Feature** or **Ensemble** by supplying an Ids array.  For example, if the user opts to compute statistics per **Feature** and selects an array that has 10 unique **Feature** Ids, then this **Filter** will compute 10 sets of statistics (e.g., find the mean of the supplied array for each **Feature**, find the total number of points in each **Feature** (the length), etc.).  

The input array may also be *standardized*, meaning that the array values will be adjusted such that they have a mean of 0 and unit variance.  This *Standardize Data* option requires the selection of both the *Find Mean* and *Find Standard Deviation* options.  The standardized data will be saved as a new array object stored in the same **Attribute Matrix** as the input array.  Note that if the *Standardize Data* option is selected, the mean and standard deviation values created by this **Filter** reflect the mean and standard deviation of the *original* array; the new standardized array has a mean of 0 and unit variance.  The standardized array will be computed in double precision.  If the statistics are being computed per **Feature** or **Ensemble**, then the array values are standardized according to the mean and standard deviation *for each **Feature/Ensemble***.  For example, if 5 unique **Features** were being analyzed and *Standardize Data* was selected, then the array values for **Feature** 1 would be standardized according to the mean and standard deviation for **Feature** 1, then the array values for **Feature** 2 would be standardized according to the mean and standard deviation for **Feature** 2, and so on for the remaining **Features**.  

The user must select a destination **Attribute Matrix** in which the computed statistics will be stored.  If electing to *Compute Statistics Per Feature/Ensemble*, then a reasonable selection for this array is the **Feature/Ensemble** **Attribute Matrix** associated with the supplied **Feature/Ensemble** Ids.  However, the only requirement is that the number of columns in the selected destination **Attribute Matrix** match the number of **Features/Ensembles** specified by the supplied Id array.  This requirement is enforced at run time.  If computing statistics for the entire input array, then only one value is computed per statistic; therefore, the arrays produced only contain one value.  In this case, the destination **Attribute Matrix** should only contain 1 tuple.  If such a **Generic Attribute Matrix** does not exist, it can be created.

Special operations occur for certain statistics if the supplied array is of type *bool* (for example, a mask array produced [when thresholding](@ref multithresholdobjects)).  The length, minimum, maximum, median, mode, and summation are computed as normal (although the resulting values may be platform dependent).  The mean and standard deviation for a boolean array will be true if there are more instances of true in the array than false.  If *Standardize Data* is chosen for a boolean array, no actual modifications will be made to the input.  These operations for boolean inputs are chosen as a basic convention, and are not intended be representative of true boolean logic.

**Note**: If *Find Histogram* is on AND *Compute Statistics Per Feature/Ensemble* is on, then any features that have the exact same value throughout the entire feature will have its first histogram bin set to the total count of feature values.  All other bins will be 0.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
