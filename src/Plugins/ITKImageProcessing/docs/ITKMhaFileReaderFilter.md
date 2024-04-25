# ITK MHA File Reader

This filter reads the image data from an MHA file.

The transformation matrix from the MHA file can be optionally read and saved as a data array and/or applied to the created image geometry.

## Group (Subgroup)

ITKImageProcessing (ITKImageProcessing)

## Description

Reads MHA images and the transformation matrix using ITK. Some select headers from the MHA file are read. If those headers are not available then the default values are used.

- Number of Dimensions
- Center of Rotation (0,0,0 is the default)
- Offset (used at the origin for the created Image Geometr: 0,0,0 is the default)
- Transformation Matrix

### Use of the Transformation Matrix Notes

There is an option to use the transpose of the Transformation Matrix. This can be useful if the stored transformation matrix is not the correct active transformation. If the determinant of the transformation matrix is NOT 1.0 (or really close) AND the user has selected to transpose the matrix a preflight error will be thrown. Using the *transpose* of the transformation matrix **only** works if the transformation matrix purely represents a rotation: Other affine transforms are **NOT** allowed such as shear, scale and translation.

#### Technical Discussion

A 4x4 transformation matrix that involves only rotation (and no translation, scaling, or other transformations), the transpose of the matrix is equivalent to its inverse. This property is specific to orthogonal matrices, which are matrices where the rows and columns form a set of orthonormal vectors. In the context of 3D graphics and transformations, a rotation matrix is a type of orthogonal matrix.

The key properties that make the transpose of a rotation matrix equivalent to its inverse are:

- **Preservation of Dot Product**: The rotation does not change the dot product between vectors, preserving angles and lengths.
- **Orthonormal Rows and Columns**: The rows (and columns) of a rotation matrix are orthonormal, meaning they have unit length and are perpendicular to each other. This property ensures that multiplying the matrix by its transpose results in the identity matrix, indicating that the transpose is indeed the inverse.

% Auto generated parameter table will be inserted here

## Example Pipelines

None.

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
