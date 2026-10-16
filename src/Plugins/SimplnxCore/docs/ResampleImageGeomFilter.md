# Resample Data (Image Geometry)

## Group (Subgroup)

Sampling (Resample)

## Description

This **Filter** changes the cell resolution of an **Image Geometry** by overlaying a new regular grid on the existing data and copying the *closest-cell* value from the old grid to each new cell. No interpolation is performed.

Copied child objects retain their names, including Attribute Matrix and DataArray names that contain the source geometry's name. For example, copying `Geom/Geom Feature Data/Geom Values` to `Geom Out` creates `Geom Out/Geom Feature Data/Geom Values`.

The overall **bounds** of the volume do not change -- only the spacing and the number of cells. To scale the physical extent of a geometry, apply a scaling transformation with [Apply Transformation To Geometry](ApplyTransformationToGeometryFilter.md) instead.

![Fig. 1: Resampling overlays a new grid of a different spacing on the same physical volume; doubling the spacing along an axis halves the cell count there. Each new cell takes the value of the nearest source cell — no interpolation is performed.](Images/ResampleImageGeom_SpacingCellCount.png)

### Resampling Mode

The *Resampling Mode* parameter provides three ways to specify the new grid:

- **Spacing [0]**: Set the new spacing directly (in physical units; same as the input geometry's spacing). The cell count adjusts accordingly. Use small spacings cautiously -- they can produce very large cell counts.
- **Scaling [1]**: Scale spacing by a percentage per axis. A scaling factor of 30% reduces cell count to roughly 30% of the original (and increases spacing by ~3.33x).
- **Exact Dimensions [2]**: Set the exact new cell counts per axis (integer). Spacing is computed automatically to span the original bounds.

#### Spacing Examples

Image with cell dimensions (524, 390, 164) and spacing (1, 1, 1) (units in microns/voxel).

- New spacing (2, 2, 2) → cell dimensions (262, 195, 82), spacing (2, 2, 2) microns/voxel.
- New spacing (0.25, 0.7, 2.3) → cell dimensions (2096, 557, 71), spacing (0.25, 0.7, 2.3).

#### Scaling Examples

Same starting image.

- Scaling (30%, 30%, 30%) → cell dimensions (157, 117, 49), spacing (3.333, 3.333, 3.333).
- Scaling (120.4%, 50.74%, 68.12%) → cell dimensions (630, 197, 111), spacing (0.831, 1.971, 1.468).

#### Exact Dimensions Examples

Same starting image.

- Exact dimensions (100, 100, 100) → cell dimensions (100, 100, 100), spacing (5.24, 3.9, 1.64).
- Exact dimensions (100, 500, 20) → cell dimensions (100, 500, 20), spacing (5.24, 0.78, 8.2).

### Resampling Algorithm

A new regular grid is overlaid on the old one. For each new cell, the filter finds the old cell whose center is closest and copies that old cell's attributes (Feature Ids, phases, orientations, etc.) into the new cell. There is **no interpolation** -- this is the right behavior for label data (Feature Ids), but it does mean that small features can disappear during downsampling.

## Feature Data

A **Feature** is a group of cells with the same Feature Id. A **Feature Attribute Matrix** stores one row of values for each Feature Id. Resampling changes the cell membership and cell count of features, even when no feature disappears. Feature sizes, shapes, centroids, neighbors, surface status, and average values can change. Treat every feature value computed before the resample as invalid for the resampled geometry.

Renumbering alone does not correct these values. It compacts Feature Ids. It does not recompute feature values.

*Clear Feature Attribute Matrix* is on by default. It recreates the selected matrix in the resampled geometry with no arrays. This removes numeric arrays, string arrays, and **NeighborLists** (lists of neighboring features). Recompute any arrays needed by later filters. A later filter that selects a cleared array fails preflight until that array is created again.

### Clearing and Renumbering Modes

| Clear Feature Attribute Matrix | Renumber Features | Result |
|---|---|---|
| On | On | Dense matrix with no arrays. Remaining Feature Ids become 1 through N, where N is the number of remaining features. The matrix has N+1 rows, including row 0. |
| On | Off (default) | Sparse matrix with no arrays. It keeps the source tuple shape and original Feature Ids. Rows for removed features are unused. Use this mode to track features between the original and resampled volumes. |
| Off | Hidden and ignored | All Attribute Matrices other than the Cell Attribute Matrix are copied unchanged. Feature Ids keep their original labels. Use this mode for raw data without features, or for debugging. Copied feature values still describe the original volume. |

*Feature Attribute Matrix*, *Renumber Features*, and *Cell Feature Ids* are shown only when *Clear Feature Attribute Matrix* is on. *Renumber Features* is off by default. Both selection paths are required when Clear is on, even when Renumber is off. Select an `int32` array with one component for *Cell Feature Ids*. When Clear and Renumber are both on, *Cell Feature Ids* must be in the selected Image Geometry's Cell Attribute Matrix. When Clear is off, the filter ignores both selections and any saved Renumber setting.

Other Attribute Matrices, including the **Ensemble Attribute Matrix** (data for each phase), are copied unchanged in every mode.

### Keep the Original Values for Debugging

Turn *Perform In Place* off and set *Created Image Geometry* to a new path. The source geometry keeps its original feature values. The new geometry contains the resampled cells and follows the selected clearing mode. When *Perform In Place* is on, the resampled geometry replaces the source and keeps its name.

### Preflight Information and Errors

The preflight value **Cleared Feature Data** lists the full paths of all children of the selected Feature Attribute Matrix. Use this list to decide what to recompute after Resample. For example, run [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) again if later filters need neighbor lists. When Renumber is on, this value also explains that Feature Ids will be renumbered and the matrix resized. When Clear is off, **Feature Data Not Cleared** explains that copied feature values describe the original geometry. These values are information, not warnings.

When Clear is on, the selected Feature Attribute Matrix must be a direct child of *Selected Image Geometry*. It must not be that geometry's Cell Attribute Matrix.

- **-55503**: The Feature Attribute Matrix is not a direct child of the selected geometry. Select a Feature Attribute Matrix directly under that geometry.
- **-55504**: The selected matrix is the geometry's Cell Attribute Matrix. Select the matrix that holds feature data, or turn Clear off for data without features.
- **-55505**: Cell Feature Ids must be in the selected Image Geometry's Cell Attribute Matrix when Clear and Renumber are both on.

### Required Input Sources

- **Input Image Geometry** -- the geometry to be resampled. Typically produced by [Create Image Geometry](CreateImageGeometryFilter.md), [Read Image Stack](../ImageProcessing/ReadImageStackFilter.md), or an EBSD reader.

% Auto generated parameter table will be inserted here

## Example Pipelines

SimplnxCore/ResamplePorosityImage

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
