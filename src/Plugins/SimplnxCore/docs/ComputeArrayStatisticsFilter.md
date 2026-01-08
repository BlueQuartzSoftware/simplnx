# Compute Attribute Array Statistics

## Group (Subgroup)

DREAM3D Review (Statistics)

## Description

***WARNING: The Histogram functionality has been moved to a new filter.***

This **Filter** computes a variety of statistics for a given scalar array. The currently available statistics are array length, minimum, maximum, (arithmetic) mean, median, mode, standard deviation, and summation; any combination of these statistics may be computed by this **Filter**. Any scalar array, of any primitive type, may be used as input. The type of the output arrays depends on the kind of statistic computed:

| Statistic               | Primitive Type                      |
|-------------------------|-------------------------------------|
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

Special operations occur for certain statistics if the supplied array is of type *bool* (for example, a mask array produced from threshold filters).  The *length*, *minimum*, *maximum*, *median*, *mode*, and *summation* are computed as normal (although the resulting values may be platform dependent).  The *mean* and *standard deviation* for a boolean array will be true if there are more instances of true in the array than false.  If *Standardize Data* is chosen for a boolean array, no actual modifications will be made to the input.  These operations for boolean inputs are chosen as a basic convention, and are not intended be representative of true boolean logic.

## Destination Attribute Matrix 

The user must create a destination **Attribute Matrix** in which the computed statistics will be stored. DREAM3D-NX enforces a rule where any Attribute Matrix cannot contain another Attribute Matrix. With this in mind, the user should select a destination that is not itself an Attribute Matrix, such as the top level of a Geometry or the top level of the Data Structure itself. The user could have also created a group (using a previous filter) and use that group as the destination.

![Images/Compute_Array_Statistics_1.png](Images/Compute_Array_Statistics_1.png)
The user is creating the destination Attribute Matrix inside the `DataContainer` geometry.

![Images/Compute_Array_Statistics_2.png](Images/Compute_Array_Statistics_2.png)
The user is creating the destination Attribute Matrix inside the `Statistics` group which was created by filter #2 in the pipeline.

## Ranges Breakdown

The ranges feature was added to primarily offer the following functionality:

1. option to output an array that has the Feature id in it. (Feature Ids Indexing Array)
2. option to set the "Feature Id" range.

- Allow the user to "pad out the feature ids" to a specific range
- Allow the user to only compute stats for specific feature Ids

3. option to Ignore Feature Id Zero.
4. remove empty spaces for feature ids that start above 1

All of these can be achieved with the new functionality, here's how:

### Option 1

For option 1, this array (Feature Ids Indexing Array) is automatically created for any Range selection other than `None`. The nuance here is that if your range or `Shrink To Fit` contains all the features this array will be redundant and can be removed, however, this is a very niche occurance and users are encouraged to just select `None` if they know this to be the case.

### Option 2

For option 2, this is provided with both the `Padded Custom Range` and `Minimum Size in Custom Range`. The latter is intended for users who are trying to cut down size without aproiri knowledge of the number of features. It will chop anything outside the upper bound or take the max feature if the custom upper bound exceeds it. The same is true for the lower bound in that it will take the higher of the two between provided range and the min Feature Id. `Padded Custom Range` will fill generate/fill extra values for values below and above the minimum and maximum Feature Id respectively. See the bonus section for additional range features.

### Option 3

For option 3, the ability to ignore Feature Id Zero (the invalid Feature Id) is provided directly in the form of `Ignore Feature 0` and indirectly through ranges.

### Option 4

For option 4, the most direct feature to address this is the `Shrink to Fit` range option, however it can also be achived with `Minimum Size in Custom Range`.

*Bonus: If you are unsure of the max feature id in your range, supplying a `-1` will determine the maximum feature id and use it as the upper bound in execution.*

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
