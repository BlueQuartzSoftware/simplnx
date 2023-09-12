# Resample RectGrid To ImageGeom

## Group (Subgroup) 

Sampling (Sampling)

## Description 

This **Filter** will resample an existing RectilinearGrid onto a regular grid (Image Geometry) and copy cell data into the newly created Image Geometry Data Container during the process.

The sampling algorithm is a simple "last one wins" algorithm due to the likely hood that interpolating data is probably not the correct algorithm to bring in data to the new Image Geometry.

The algorithm logic is thus: If the ImageGeometry cell would contain multiple RectilinearGrid cells, then we select from the covered cells the cell with the largest X, Y and Z index and copy that data into the Image Geometry Cell Attribute Matrix.

The user can select which cell attribute matrix data arrays will be copied into the newly created Image Geometry Cell Attribute Matrix.

## Parameters 

| Name | Type | Description |
|------|------|------|
| Input Rectilinear Grid| DataPath | Path to the RectGrid Geometry to be re-sampled |
| Selected Cell Attribute Arrays to Copy | Array of (DataPath) | Rect Grid Cell Data to possibly copy |
| Image Geometry Voxel Dimensions | 3 x Int32  | The image geometry voxel dimensions in which to re-sample the rectilinear grid geometry |

## Required Geometry 

Rectilinear Grid Geometry

## Created Objects 

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Image Geometry** | Image Geometry | N/A | N/A | Path to the created Image Geometry |
| **Attribute Matrix** | Cell Data | Element/Feature/Ensemble/etc. | N/A | The name of the cell data Attribute Matrix created with the Image Geometry |
| **Element/Feature/Ensemble/etc. Attribute Array** | Copied from the input Data Container | int32_t/float/etc. | (1)/(3)/etc. | Cell level arrays copied over from the input to the resampled geometry |

## Example Pipelines 

## License & Copyright 

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


