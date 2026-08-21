# Combine Transformation Matrices

## Group (Subgroup)

Core

## Description

This **Filter** multiplies multiple 4x4 transformation matrices together to produce a single composite 4x4 matrix. Use it when you have a sequence of transformations (rotations, translations, scales, etc.) that need to be combined before applying them to an Image Geometry.

### When to Use This Filter

[Apply Transformation to Geometry](ApplyTransformationToGeometryFilter.md) re-grids cell data each time it runs. Applying multiple transformations one at a time to an Image Geometry compounds re-gridding artifacts. The correct approach is to combine all the transformations into a single 4x4 matrix first using this filter, then apply that combined matrix as a single transformation. See the *Image Geometry Caveat* section in [Apply Transformation to Geometry](ApplyTransformationToGeometryFilter.md) for the visual demonstration.

For node-based geometries (Vertex, Edge, Triangle, Quad, Tet, Hex), there is no re-gridding step, so successive transformations are safe and this filter is not strictly needed -- but combining is still slightly more efficient than chaining.

### Input Format

Each input matrix must be a 16-element float32 **Attribute Array** in **row-major** order:

    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16

represents the 4x4 matrix:

    1   2   3   4
    5   6   7   8
    9   10  11  12
    13  14  15  16

The component shape can be any layout that yields 16 total elements (4×4, 16×1, or 1×16). All input matrices must share the same tuple and component layout.

### Multiplication Order

The matrices are multiplied in the order listed in the parameter. The result represents applying the **first** matrix first, then the second, and so on. In matrix algebra, this means:

    M_combined = M_last * M_(last-1) * ... * M_2 * M_1

which corresponds to applying M_1 first to a vector v: `v' = M_combined * v = M_last * ... * M_2 * M_1 * v`.

### Required Input Sources

- **Transformation Matrices** -- at least two 4x4 float32 matrices, typically authored by hand or produced by an earlier [Apply Transformation to Geometry](ApplyTransformationToGeometryFilter.md) run with the *Saving the Final Transformation Matrix* option enabled.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
