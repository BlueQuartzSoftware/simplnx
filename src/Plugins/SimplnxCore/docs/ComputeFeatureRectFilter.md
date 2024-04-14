# Compute Feature Corners

## Group (Subgroup)

Reconstruction (Reconstruction)

## Description

This **Filter** computes the XYZ minimum and maximum coordinates for each **Feature** in a segmentation. This data can be important for finding the smallest encompassing volume. This values are given in **Pixel** coordinates.

|       | 0 | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|---|
| 0 | 0 | 0 | 1 | 0 | 0 |
| 1 | 0 | 0 | 1 | 1 | 0 |
| 2 | 0 | 1 | 1 | 1 | 1 |
| 3 | 0 | 0 | 1 | 1 | 0 |
| 4 | 0 | 0 | 0 | 0 | 0 |

If the example matrix above which represents a single feature where the feature ID = 1, the output of the filter would be:

    X Min = 1
    Y Min = 0
    Z Min = 0

    X Max = 4
    Y Max = 3
    Z Max = 0

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
