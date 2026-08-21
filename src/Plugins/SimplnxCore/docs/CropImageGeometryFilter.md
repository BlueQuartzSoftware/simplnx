# Crop Geometry (Image)

## Group (Subgroup)

Core (Spatial)

## Description

This **Filter** extracts a region of interest (ROI) from an **Image Geometry**, producing a new geometry that contains only the selected cells. Bounds can be specified either in cell indices (voxels) or in physical coordinates. Individual dimensions (X, Y, Z) can be cropped independently.

This is the inverse of [Pad Image Geometry](PadImageGeometryFilter.md). Common uses are isolating a sample from its overscan border, focusing analysis on a single feature, or reducing data size for testing.

### Bounds Mode

The *Use Physical Bounds* parameter selects how the crop bounds are interpreted:

- **Use Physical Bounds = false**: bounds are integer **cell indices** (0-based, **inclusive** on both ends). Xmin=50, Xmax=99 keeps cells 50 through 99 (the last 50 cells of a 100-cell volume).
- **Use Physical Bounds = true**: bounds are **physical coordinates** in the geometry's units. The filter computes which cells fall inside the box defined by those coordinates, taking the geometry's origin and spacing into account.

If any bound exceeds the geometry's extent on that axis, the filter clamps to the geometry's actual extent. The filter fails in preflight only when **all** of the requested bounds fall outside the geometry.

### Per-Axis Cropping

The *Crop X Dimension*, *Crop Y Dimension*, and *Crop Z Dimension* booleans toggle whether each axis is cropped at all. An axis with its flag OFF retains all of its cells regardless of the bounds setting.

### Examples

In the following examples, the source image has:

- Origin: (0.0, 0.0, 0.0)
- Spacing: (0.5, 0.5, 1.0)
- Dimensions: (100, 100, 1)

So the physical bounds are (0-50 microns, 0-50 microns, 0-1 micron).

![Base image for examples](Images/CropImageGeometry_1.png)

#### Example 1 -- Crop to the last 50 cells in X and Y

    Xmin = 50, Xmax = 99
    Ymin = 50, Ymax = 99
    Zmin = 0,  Zmax = 0
    Use Physical Bounds = false

Result:

![Cropped image using voxels as the bounds](Images/CropImageGeometry_2.png)

#### Example 2 -- Crop to the middle 50 cells

    Xmin = 25, Xmax = 74
    Ymin = 25, Ymax = 74
    Zmin = 0,  Zmax = 0
    Use Physical Bounds = false

Result:

![Cropped image using voxels as the bounds](Images/CropImageGeometry_3.png)

#### Example 3 -- Crop using physical coordinates, with one bound exceeding the volume

    Xmin = 30 microns, Xmax = 65 microns
    Ymin = 30 microns, Ymax = 65 microns
    Zmin = 0 microns,  Zmax = 65 microns
    Use Physical Bounds = true

The Zmax of 65 microns exceeds the geometry's 1-micron Z extent and is silently clamped. The crop still succeeds because at least part of the requested box lies inside the geometry.

![Cropped image using voxels as the bounds](Images/CropImageGeometry_4.png)

### Renumber Features

Cropping can fully remove some **Features** from the volume, which leaves their Feature IDs unused and produces gaps. If *Renumber Features* is enabled, the Cell Feature Attribute Matrix is resized to drop empty features and remaining Features are renumbered to be contiguous starting from 1. Leave this off if you intend to compare the cropped output against the original larger volume by Feature ID.

The user can save the cropped volume as a new **Data Container** or overwrite the current volume in place.

## WARNING: NeighborList Removal

When *Renumber Features* is enabled and the Cell Feature Attribute Matrix contains any *NeighborList* arrays, those arrays are **removed** because they refer to the old Feature IDs. Re-run [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) afterward to rebuild them.

### Required Input Sources

- **Input Image Geometry** -- the geometry to crop. Typically produced by [Create Image Geometry](CreateImageGeometryFilter.md), [ITK Import Image Stack](../ITKImageProcessing/ITKImportImageStackFilter.md), or an EBSD reader.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
