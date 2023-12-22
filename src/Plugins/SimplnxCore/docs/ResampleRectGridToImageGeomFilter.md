# Resample RectGrid To ImageGeom

## Group (Subgroup)

Sampling (Sampling)

## Description

This **Filter** will resample an existing RectilinearGrid onto a regular grid (Image Geometry) and copy cell data into the newly created Image Geometry Data Container during the process.

The sampling algorithm is a simple "last one wins" algorithm due to the likely hood that interpolating data is probably not the correct algorithm to bring in data to the new Image Geometry.

The algorithm logic is thus: If the ImageGeometry cell would contain multiple RectilinearGrid cells, then we select from the covered cells the cell with the largest X, Y and Z index and copy that data into the Image Geometry Cell Attribute Matrix.

The user can select which cell attribute matrix data arrays will be copied into the newly created Image Geometry Cell Attribute Matrix.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
