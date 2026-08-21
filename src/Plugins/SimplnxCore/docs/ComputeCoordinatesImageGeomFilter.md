# Compute Coordinates/Indices Array From Image Geom

## Group (Subgroup)

Statistics

## Description

This **Filter** writes the per-cell coordinates and/or grid indices of an **Image Geometry** into regular cell-level **Attribute Arrays**. In an Image Geometry, each cell's (i, j, k) index and (x, y, z) physical position are implicit -- derivable from the geometry's dimensions, origin, and spacing. This filter makes them explicit so they can be exported to CSV/text, used as inputs to downstream math filters, or visualized directly.

### When to Use This Filter

Most commonly used as a preparation step before exporting cell data to CSV or other text formats that need explicit coordinate columns. Also useful when a downstream filter needs the (x, y, z) of each cell as a regular array (e.g., for distance calculations).

### Output Array(s) Type

- **Physical Coordinates [0]**: produces a single 3-component float array containing the (x, y, z) physical coordinates of the center of each cell, computed from the geometry's origin and spacing.
- **Indices [1]**: produces a single 3-component integer array containing the (i, j, k) grid indices of each cell. Indices are **0-based**.
- **Both [2]**: produces both arrays.

### Cell Order

The arrays are stored in the geometry's default cell raster order: X varies fastest, then Y, then Z.

    (0,0,0) → (1,0,0) → (2,0,0) → ... (n,0,0) → (0,1,0) → (1,1,0) → ... (n,n,0) → (0,0,1) → ...

A sample of the output for a 9-column slice (showing both indices and physical coordinates):

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

### Units

- **Physical coordinates** are in the geometry's physical units (microns, millimeters, etc.).
- **Indices** are dimensionless integers (0-based cell indices along each axis).

### Required Input Sources

- **Input Image Geometry** -- the geometry whose per-cell coordinates/indices will be made explicit. Typically produced by [Create Image Geometry](CreateImageGeometryFilter.md), [ITK Import Image Stack](../ITKImageProcessing/ITKImportImageStackFilter.md), or an EBSD reader.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
