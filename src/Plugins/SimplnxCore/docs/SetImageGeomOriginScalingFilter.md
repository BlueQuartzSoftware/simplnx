# Set Origin & Spacing (Image Geom)

## Group (Subgroup)

Core (Spatial)

## Description

This **Filter** updates the origin and/or spacing of an existing **Image Geometry**. The grid dimensions (number of cells per axis) are unchanged; only the physical placement and physical size of the cells are modified. Use this filter to:

- Correct an incorrect origin after import (e.g., reset to (0, 0, 0) after a sample-frame rotation).
- Convert the geometry's spacing to a different physical unit (e.g., millimeters → microns).
- Recenter a geometry so the user-specified point lands at the volume's geometric center.

![Fig. 1: An Image Geometry is defined by its dimensions, spacing, and origin. This filter changes the origin (bottom-left grid point) and/or spacing (distance between grid planes); the dimensions are left unchanged.](Images/CreateImageGeometry_OriginSpacingDimensions.png)

### Origin

The *Origin* parameter sets the new physical location of the bottom-left corner of the geometry. For example, to change the origin from (0, 0, 0) to (10, 4, 8):

- X Origin: 10
- Y Origin: 4
- Z Origin: 8

Origin coordinates are in the geometry's physical units (microns, millimeters, etc.).

### Spacing

The *Spacing* parameter sets the new physical distance between grid planes along each axis. Spacing values must be positive and non-zero. Updating spacing rescales the entire physical extent of the geometry (since the number of cells is unchanged) but leaves cell data values untouched.

### Put Input Origin at the Center of Geometry

If *Put Input Origin at the Center of Geometry* is enabled, the user-supplied origin value is treated as the **geometric center** of the volume rather than the bottom-left corner. The filter computes the corresponding bottom-left origin so that the user-supplied point lands at the center. This is convenient for centering a geometry on a meaningful reference point (sample center, beam axis, etc.) without computing the corner offset by hand.

### Required Input Sources

- **Input Image Geometry** -- the geometry to modify in place. Typically produced by [Create Image Geometry](CreateImageGeometryFilter.md), [ITK Import Image Stack](../ITKImageProcessing/ITKImportImageStackFilter.md), or an EBSD reader.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
