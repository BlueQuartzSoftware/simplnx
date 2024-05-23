# Resample Data (Image Geometry)

## Group (Subgroup)

Sampling (Resample)

## Description

This **Filter** changes the **Cell** spacing/resolution based on inputs from the user. There are several resampling modes:

## WARNING: NeighborList Removal

If the option to "Renumber Features" is turn ON and the Cell Feature AttributeMatrix contains any *NeighborList* data arrays, those arrays will be **REMOVED** because those lists are now invalid. Re-run the *Find Neighbors* filter to re-create the lists.

### Spacing

The values entered are the desired new spacings (not multiples of the current resolution).  The number of **Cells** in the volume will change when the spacing values are changed and thus the user should be cautious of generating "too many" **Cells** by entering very small values (i.e., very high resolution).

**Example 1**:

Image Geometry with cell dimensions (524, 390, 164) and spacing (1, 1, 1).

If the new spacing value is (2, 2, 2), then the geometry will have cell dimensions (262, 195, 82) and spacing (2, 2, 2).

**Example 2**:

Image Geometry with cell dimensions (524, 390, 164) and spacing (1, 1, 1).

If the new spacing value is (0.25, 0.7, 2.3), then the geometry will have cell dimensions (2096, 557, 71) and spacing (0.25, 0.7, 2.3).

### Scale Factor

The values entered are the desired scaling factor for each dimension, in percentages.

**Example 1**:

Image Geometry with cell dimensions (524, 390, 164) and spacing (1, 1, 1).

If the new scaling value is (30%, 30%, 30%), then the geometry will have cell dimensions (157, 117, 49) and spacing (3.3333, 3.3333, 3.3333).

**Example 2**:

Image Geometry with cell dimensions (524, 390, 164) and spacing (1, 1, 1).

If the new scaling value is (120.4%, 50.74%, 68.12%), then the geometry will have cell dimensions (630, 197, 111) and spacing (0.830565, 1.97083, 1.468).

### Exact Dimensions

The values entered are the desired cell dimensions of the resampled geometry.  (100, 100, 100) would resample the geometry so that there are 100 cells in each dimension.

**Example 1**:

Image Geometry with cell dimensions (524, 390, 164) and spacing (1, 1, 1).

If the new exact dimensions are (100, 100, 100), then the geometry will have cell dimensions (100, 100, 100) and spacing (5.24, 3.9, 1.64).

**Example 2**:

Image Geometry with cell dimensions (524, 390, 164) and spacing (1, 1, 1).

If the new exact dimensions are (100, 500, 20), then the geometry will have cell dimensions (100, 500, 20) and spacing (5.24, 0.78, 8.2).

---

A new grid of **Cells** is created and "overlaid" on the existing grid of **Cells**.  There is currently no *interpolation* performed, rather the attributes of the old **Cell** that is closest to each new **Cell's** is assigned to that new **Cell**.

*Note:* Present **Features** may disappear when down-sampling to coarse resolutions. If *Renumber Features* is checked, the **Filter** will check if this is the case and resize the corresponding **Feature Attribute Matrix** to comply with any changes. Additionally, the **Filter** will renumber **Features** such that they remain contiguous.

*Note:* This filter does NOT change the overall bounds of the volume, just the spacing and number of cells in the volume.  To change the overall bounds of the volume, apply a scaling transformation with the [Apply Transformation To Geometry](./ApplyTransformationToGeometryFilter.md) filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

SimplnxCore/ResamplePorosityImage

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
