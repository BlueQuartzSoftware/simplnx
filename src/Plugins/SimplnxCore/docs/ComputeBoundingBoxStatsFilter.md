# Compute Statistics Within Bounding Boxes

## Group (Subgroup)

Statistics

## Description

***NOTE:* This filter allows each **Cell** value to contribute to 0 or more bounding boxes (a 1-to-many architecture). This comes with the caveat that the *Standardize Data* option offered by [Compute Attribute Array Statistics](ComputeArrayStatisticsFilter.md) cannot be included as functionality in this filter.**

This filter computes a variety of statistics for a given scalar array, restricted to the **Cells** that fall inside each supplied bounding box. Because boxes may overlap (or leave gaps), a single **Cell** may be counted toward several boxes, exactly one box, or none. The currently available statistics are array length, minimum, maximum, (arithmetic) mean, median, mode, standard deviation, summation, and number of unique values; any combination of these statistics may be computed. Any scalar array, of any primitive type, may be used as input. The type of each output array depends on the kind of statistic computed:

| Statistic               | Primitive Type                      |
|-------------------------|-------------------------------------|
| Length                  | signed 64-bit integer               |
| Minimum                 | same type as input                  |
| Maximum                 | same type as input                  |
| Mean                    | 32-bit float                        |
| Median                  | 32-bit float                        |
| Mode                    | same type as input                  |
| Standard Deviation      | 32-bit float                        |
| Summation               | 32-bit float                        |
| Number of Unique Values | signed 32-bit integer               |

The *Unified Bounds Array* is a 6-component **DataArray** that contains the lower XYZ bound and upper XYZ bound for each bounding box. The component ordering is `{Min-X, Min-Y, Min-Z, Max-X, Max-Y, Max-Z}`. These bounds are expressed in the **Geometry**'s physical coordinate units (the same length units as the geometry's origin and spacing, typically microns). This ordering is provided so the user knows how to format imported external data. The [Compute Feature Bounding Boxes](ComputeFeatureBoundsFilter.md) filter's *Unified Array* option produces a correctly formatted array for input to this filter.

The user must select or create a destination **Attribute Matrix** in which the computed statistics are stored. When selecting an existing **Attribute Matrix**, a reasonable choice is the **Attribute Matrix** that the *Unified Bounds Array* comes from. However, the only requirement is that the number of tuples in the selected destination **Attribute Matrix** match the number of tuples (boxes) in the *Unified Bounds Array*. This requirement is enforced during preflight. If the option to create a new **Attribute Matrix** is selected, it is created in the supplied **Geometry**, sized appropriately.

Special operations occur for certain statistics if the supplied array is of type *bool* (for example, a mask array produced by [Multi-Threshold Objects](MultiThresholdObjectsFilter.md)). The length, minimum, maximum, median, mode, and summation are computed as normal (although the resulting values may be platform dependent). The mean and standard deviation for a boolean array are *true* if there are more instances of *true* in the array than *false*. These operations for boolean inputs are chosen as a basic convention, and are not intended to be representative of true boolean logic.

### Required Input Sources

- **Unified Bounds Array** -- a 6-component bounds array produced by the *Unified Array* output option of [Compute Feature Bounding Boxes](ComputeFeatureBoundsFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
