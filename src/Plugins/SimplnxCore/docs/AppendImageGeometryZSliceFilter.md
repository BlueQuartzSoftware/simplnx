# Append Z-Slice

## Group (Subgroup)

Sampling (Memory/Management)

## Description

This filter allows the user to append an Image Geometry onto the "end" of another Image Geometry. The input and
destination **ImageGeometry** objects must have the same X&Y dimensions. Optional Checks for equal **Resolution** values
can also be performed.

For example, if the user has an already existing **Image Geometry** that is 100 voxels in the *X* direction by 200 pixels in the
*Y* direction and composed of 5 *Z* slices then appending another data set that is the same dimensions in X & Y but contains
10 *Z* slices then the resulting **Image Geometry** will have a total of 15 *Z* slices.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
