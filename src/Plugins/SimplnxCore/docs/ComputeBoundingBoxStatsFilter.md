# Compute Statistics Within Bounding Boxes

## Group (Subgroup)

Statistics

## Description

***NOTE:* This filter is intended to allow each cell value to map to 0 or more bounding boxes (1 to Many architecture). This comes with the caveat that the option `Standardize Data` offered by `Compute Array Statistics` cannot be included as functionality in this filter.**

This **Filter** computes a variety of statistics for a given scalar array. The currently available statistics are array length, minimum, maximum, (arithmetic) mean, median, mode, standard deviation, summation, and Number of Unique values; any combination of these statistics may be computed by this **Filter**. Any scalar array, of any primitive type, may be used as input. The type of the output arrays depends on the kind of statistic computed:

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
| Number of Unique Values | signed 32-bit integer               |

The *Unified Bounds Array* is a 6 component **DataArray** that contains the lower XYZ bound and upper XYZ bound for the bounding box. The component ordering is expected as follows `{Min-X, Min-Y, Min-Z, Max-X, Max-Y, Max-Z}`. This ordering is provided so the user knows how to format imported external data. The *Compute Feature Bounds* filter's `Unified Array` option puts out a correctly formatted array for input to this filter.

The user must select or create a destination **Attribute Matrix** in which the computed statistics will be stored.  If selecting an exisitng **Attribute Matrix**, then a reasonable selection for this array is the **Attribute Matrix** that the *Unified Bounds Array* comes from.  However, the only requirement is that the number of columns in the selected destination **Attribute Matrix** match the number of tuples in the *Unified Bounds Array*. This requirement is enforced during preflight. If the option to create an Attribute Matrix is selected, it will be created in the supplied geometry sized appropriately.

Special operations occur for certain statistics if the supplied array is of type *bool* (for example, a mask array produced [when thresholding](@ref multithresholdobjects)).  The length, minimum, maximum, median, mode, and summation are computed as normal (although the resulting values may be platform dependent).  The mean and standard deviation for a boolean array will be true if there are more instances of true in the array than false. These operations for boolean inputs are chosen as a basic convention, and are not intended be representative of true boolean logic.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
