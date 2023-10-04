# Rotate Sample Reference Frame

## Group (Subgroup)

Sampling (Rotating/Transforming)

## Description

**NOTE: As of July 2023, this filter is only verified to work with a rotation angle of 180 degrees, a rotation axis of (010), and a (0, 0, 0) origin.**

This **Filter** will rotate the *spatial reference frame* around a user defined axis, by a user defined angle.  The **Filter** will modify the (X, Y, Z) positions of each **Cell** to correctly represent where the **Cell** sits in the newly defined reference frame. For example, if a user selected a *rotation angle* of 90<sup>o</sup> and a *rotation axis* of (001), then a **Cell** sitting at (10, 0, 0) would be transformed to (0, -10, 0), since the new *reference frame* would have x'=y and y'=-x.

The equivalent rotation matrix for the above rotation would be the following:

|   |   |   |
| - | - | - |
| 0 | -1 | 0 |
| 1 | 0 | 0 |
| 0 | 0 | 1 |

## Example

When importing EBSD data from EDAX typically the user will need to rotate the sample reference frame about the <010> (Y) axis. This results in the image comparison below. Note that in the original image the origin of the data is at (0, 0) microns but after rotation the origin now becomes (-189, 0) microns. If you need to reset the origin back to (0,0) then the filter "Set Origin & Spacing" can be run.

![Imported EBSD Data Rotated about the <010> axis](Images/RotateSampleRefFrame_1.png)

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| Rotation Representation | Enumeration | Which form used to represent rotation (**axis angle** or **rotation matrix) |
| Rotation Axis (ijk) | float (3x) | Axis in sample reference frame to rotate about (if **axis angle**) |
| Rotation Angle (Degrees) | float | Magnitude of rotation (in degrees) about the rotation axis (if **axis angle**) |
| Rotation Matrix | float (3x3) | Axis in sample reference frame to rotate about (if **rotation matrix**) |

## Required Geometry

Image

## Required Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
|   DataArray   | N/A | Any | Same as geometry | List of **DataArrays** associated with the geometry |

## Created Objects

None

## Example Pipelines

+ INL Export
+ Export Small IN100 ODF Data (StatsGenerator)
+ TxCopper_Exposed
+ TxCopper_Unexposed
+ Edax IPF Colors
+ Confidence Index Histogram

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
