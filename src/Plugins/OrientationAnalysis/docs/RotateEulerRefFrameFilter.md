# RotateEulerRefFrame

## Group (Subgroup) ##

Orientation Analysis (Conversion)

## Description ##

This **Filter** performs a passive rotation (Right hand rule) of the Euler Angles about a user defined axis. The *reference frame* is being rotated and thus the *Euler Angles* necessary to represent the same orientation must change to account for the new *reference frame*.  The user can set an *angle* and an *axis* to define the rotation of the *reference frame*.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Rotation Axis-Angle | float (4x) | Axis-Angle in sample reference frame to rotate about. |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Data Array** | EulerAngles | float | (3) | Three angles defining the orientation of the **Cell** in Bunge convention (Z-X-Z) |

## Created Objects ##

None

## Example Pipelines ##

+ INL Export
+ Export Small IN100 ODF Data (StatsGenerator)
+ Edax IPF Colors
+ Confidence Index Histogram

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: RotateEulerRefFrameFilter
+ Displayed Name: Rotate Euler Reference Frame

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_euler_angles_array_path | Euler Angles | Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z) | complex.ArraySelectionParameter |
| rotation_axis | Rotation Axis-Angle [<ijk>w] | Axis-Angle in sample reference frame to rotate about. | complex.VectorFloat32Parameter |

