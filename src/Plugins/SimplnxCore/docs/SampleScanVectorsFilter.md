# Sample Scan Vectors

## Description

This **Filter** will take an Edge geometry of scan vectors and sample each edge at a fixed spatial resolution to generate a new Vertex geometry of sample points.  For each interpolated point it will:

- Compute the 3D coordinates along the edge.
- Interpolate the timestamp between the edge’s start and end times.
- Copy over the power value from the original edge.
- Copy over the slice ID from the original edge.
- Record the linear distance from the beginning of its scan vector.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions. 