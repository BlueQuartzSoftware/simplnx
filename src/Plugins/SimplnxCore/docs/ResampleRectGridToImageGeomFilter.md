# Resample Rectilinear Grid to Image Geom

## Group (Subgroup)

Sampling (Sampling)

## Description

This **Filter** converts a **Rectilinear Grid Geometry** (which has variable spacing along each axis) into an **Image Geometry** (which has uniform spacing along each axis). Selected cell-level arrays from the rectilinear grid are copied onto the new uniform grid using a *last-one-wins* rule.

### When to Use This Filter

Many downstream filters and most external visualization tools expect uniform-grid data. This filter is the right choice when:

- You have data on a variable-spacing rectilinear grid (e.g., imported from a simulation or instrument that uses non-uniform sampling).
- You need uniform Image Geometry data for downstream processing.
- The data being resampled is *categorical* (Feature Ids, phase labels) and interpolation would produce nonsensical intermediate values.

For interpolatable scalar data (intensity, density), prefer a true resampling filter with interpolation rather than this filter.

### Algorithm

If a single Image Geometry cell covers multiple Rectilinear Grid cells, the filter selects the rectilinear cell with the **largest X, Y, and Z index** among the covered cells and copies that one's data. This is a deterministic "last one wins" rule chosen because interpolating across categorical data would be incorrect.

The user selects:

- The target Image Geometry's dimensions, origin, and spacing.
- Which cell-level Attribute Arrays from the rectilinear grid should be copied onto the new geometry.

### Required Input Sources

- **Rectilinear Grid Geometry** -- the source geometry. Typically produced by [Create Geometry](CreateGeometryFilter.md) with type Rectilinear Grid, or imported from an external simulation.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
