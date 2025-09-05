# Combine Transformation Matrices Filter

## Group (Subgroup)

Core

## Description

The **Combine Transformation Matrices** filter merges multiple 4×4 transformation matrices into a single composite transformation.

Each input matrix must be provided as a 16-element float32 DataArray in row-major order, representing operations such as translation, rotation, scaling, or shear. During the preflight phase, the filter verifies that at least two matrices are selected, each contains exactly 16 elements (4 tuples × 4 components OR 16 tuples x 1 component OR 1 tuple x 16 components, etc), and all input arrays share the same tuple and component layout.

## Saving the Output Transformation Matrix.

The format of the output DataArray is a flattened array 16 elements in size that represents a 4x4 matrix.
The elements are encoded in ROW MAJOR order, i.e., 

    1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16

represents the following 4x4 Matrix

    1   2   3   4
    5   6   7   8
    9   10  11  12
    13  14  15  16

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
