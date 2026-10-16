# Crop Geometry (Image)

## Group (Subgroup)

Core (Spatial)

## Description

This **Filter** extracts a region of interest (ROI) from an **Image Geometry**, producing a new geometry that contains only the selected cells. Bounds can be specified either in cell indices (voxels) or in physical coordinates. Individual dimensions (X, Y, Z) can be cropped independently.

Copied child objects retain their names, including Attribute Matrix and DataArray names that contain the source geometry's name. For example, copying `Geom/Geom Feature Data/Geom Values` to `Geom Out` creates `Geom Out/Geom Feature Data/Geom Values`.

This is the inverse of [Pad Image Geometry](PadImageGeometryFilter.md). Common uses are isolating a sample from its overscan border, focusing analysis on a single feature, or reducing data size for testing.

### Bounds Mode

The *Use Physical Units For Bounds* parameter selects how the crop bounds are interpreted:

- **Use Physical Units For Bounds = false**: bounds are integer **cell indices** (0-based, **inclusive** on both ends). Xmin=50, Xmax=99 keeps cells 50 through 99 (the last 50 cells of a 100-cell volume).
- **Use Physical Units For Bounds = true**: bounds are **physical coordinates** in the geometry's units. The filter computes which cells fall inside the box defined by those coordinates, taking the geometry's origin and spacing into account.

If any bound exceeds the geometry's extent on that axis, the filter clamps to the geometry's actual extent. The filter fails in preflight only when **all** of the requested bounds fall outside the geometry.

A physical crop maximum equal to `origin + dimensions * spacing` on an axis includes the final cell on that axis. The maximum cell index is clamped to `dimensions - 1`. Physical coordinates use the Image Geometry's length units. A maximum beyond the upper bound still produces a warning and is clamped when the requested range overlaps the geometry. A physical crop bound that is NaN fails preflight with error `-50561`.

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
    Use Physical Units For Bounds = false

Result:

![Cropped image using voxels as the bounds](Images/CropImageGeometry_2.png)

#### Example 2 -- Crop to the middle 50 cells

    Xmin = 25, Xmax = 74
    Ymin = 25, Ymax = 74
    Zmin = 0,  Zmax = 0
    Use Physical Units For Bounds = false

Result:

![Cropped image using voxels as the bounds](Images/CropImageGeometry_3.png)

#### Example 3 -- Crop using physical coordinates, with one bound exceeding the volume

    Xmin = 30 microns, Xmax = 65 microns
    Ymin = 30 microns, Ymax = 65 microns
    Zmin = 0 microns,  Zmax = 65 microns
    Use Physical Units For Bounds = true

The Zmax of 65 microns exceeds the geometry's 1-micron Z extent and is clamped with a warning. The crop still succeeds because at least part of the requested box lies inside the geometry.

![Cropped image using voxels as the bounds](Images/CropImageGeometry_4.png)

## Feature Data

A **Feature** is a group of cells with the same Feature Id. A **Feature Attribute Matrix** stores one row of values for each Feature Id. Cropping changes which cells belong to each feature. Features cut by the crop boundary can change size, shape, centroid, neighbors, surface status, and average values. Nothing marks which rows were affected. Treat every value computed before the crop as invalid for the cropped geometry.

Renumbering alone never corrected these values. It changed Feature Ids and moved rows to their new indices. It did not recompute the values in those rows.

*Clear Feature Attribute Matrix* is on by default. It recreates the selected matrix in the cropped geometry with no arrays. This removes numeric arrays, string arrays, and **NeighborLists** (lists of neighboring features). Recompute any arrays needed by later filters. A later filter that selects a cleared array fails preflight until that array is created again.

### Clearing and Renumbering Modes

| Clear Feature Attribute Matrix | Renumber Features | Result |
|---|---|---|
| On | On | Dense matrix with no arrays. Remaining feature IDs become 1 through N, where N is the number of remaining features. The matrix has N+1 rows, including row 0. |
| On | Off (default) | Sparse matrix with no arrays. It keeps the source tuple shape and original Feature Ids. Rows for removed features are unused. Use this mode to track features between the original and cropped volumes. |
| Off | Hidden and ignored | All Attribute Matrices other than the Cell Attribute Matrix are copied unchanged. Feature Ids are unchanged. Use this mode for raw data without features, or for debugging. Copied feature values still describe the original volume. |

*Feature Attribute Matrix*, *Renumber Features*, and *Cell Feature Ids* are shown only when *Clear Feature Attribute Matrix* is on. *Renumber Features* is off by default. Both selection paths are required when Clear is on, even when Renumber is off. Select an `int32` array with one component for *Cell Feature Ids*. When Clear is off, the filter ignores both selections and any saved Renumber setting.

Other Attribute Matrices, including the **Ensemble Attribute Matrix** (data for each phase), are copied unchanged in every mode.

### Keep the Original Values for Debugging

Turn *Perform In Place* off and set *Created Image Geometry* to a new path. The source geometry keeps its original feature values. The new geometry contains the cropped cells and follows the selected clearing mode. When *Perform In Place* is on, the cropped geometry replaces the source and keeps its name.

### Preflight Information and Errors

The preflight value **Cleared Feature Data** lists the full paths of the arrays that will be cleared. Use this list to decide what to recompute after Crop. For example, run [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) again if later filters need neighbor lists. When Clear is off, **Feature Data Not Cleared** explains that copied feature values describe the uncropped geometry. These values are information, not warnings.

When Clear is on, the selected Feature Attribute Matrix must be a direct child of *Selected Image Geometry*. It must not be that geometry's Cell Attribute Matrix.

- **-50563**: The Feature Attribute Matrix is not a direct child of the selected geometry. Select a Feature Attribute Matrix directly under that geometry.
- **-50562**: The selected matrix is the geometry's Cell Attribute Matrix. Select the matrix that holds feature data, or turn Clear off for data without features.

### Required Input Sources

- **Input Image Geometry** -- the geometry to crop. Typically produced by [Create Image Geometry](CreateImageGeometryFilter.md), [Read Image Stack](../ImageProcessing/ReadImageStackFilter.md), or an EBSD reader.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
