# Compute Coordinates/Indices Array From Image Geometry

## Group (Subgroup)

Statistics

## Description

This **Filter** produces one or two arrays that stores implicit image information (indices and physical coordinates of each point) as explicit cell level data. The produced arrays are in XYZ component format and stored as X by Y by Z, starting from the origin. The intention behind this filter is primarily for output compatibility and readability.

The arrays follow the following cell parsing scheme: `0,0,0 -> 1,0,0 -> 2,0,0 -> ... n,0,0 -> 0,1,0 -> 1,1,0 -> 2,1,0 -> ... n,n,0 -> 0,0,1 -> 1,0,1 -> 2,0,1 -> ... n,n,n`.

The printed output will look something like this:

```console
Image Indices_0,Image Indices_1,Image Indices_2,Image Physical Coordinates_0,Image Physical Coordinates_1,Image Physical Coordinates_2
0,0,0,-47.125,0.125,-0.37500411
1,0,0,-46.875,0.125,-0.37500411
2,0,0,-46.625,0.125,-0.37500411
3,0,0,-46.375,0.125,-0.37500411
4,0,0,-46.125,0.125,-0.37500411
5,0,0,-45.875,0.125,-0.37500411
6,0,0,-45.625,0.125,-0.37500411
7,0,0,-45.375,0.125,-0.37500411
8,0,0,-45.125,0.125,-0.37500411
```

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
