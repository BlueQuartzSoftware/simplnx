# Compute Feature Face Misorientation (Face)

## Group (Subgroup)

Processing (Crystallography)

## Description

This **Filter** generates a 3 component vector for each **Triangle** in a **Triangle Geometry** that is the axis-angle of the misorientation associated with the **Features** that lie on either side of the **Triangle**. The axis is normalized, so if the magnitude of the vector is viewed, it will be the *misorientation angle* in degrees. This filter works on all valid crystal-structures/laue classes.

If you want to get the "raw" (un-normalized) axis-angle of the misorientation, enable "Store Full Axis Angle" and then (optionally) modify the "Axis Angle Array Name" parameter. The actual axis-angle is stored in a 4 component DataArray with the format as follows:

```text
{{x_axis, y_axis, z_axis, angle}, ...}
```

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (07) Small IN100 Mesh Statistics

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
