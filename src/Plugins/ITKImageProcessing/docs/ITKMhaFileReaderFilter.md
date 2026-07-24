# ITK MHA File Reader

This filter reads the image data from an MHA file.

The transformation matrix from the MHA file can be optionally read and saved as a data array and/or applied to the created image geometry.

## Group (Subgroup)

ITKImageProcessing (ITKImageProcessing)

## Description

Reads MHA images and the transformation matrix using ITK. Some select headers from the MHA file are read. If those headers are not available then the default values are used.

- Number of Dimensions
- Center of Rotation (0,0,0 is the default)
- Offset (used as the origin for the created Image Geometry; 0,0,0 is the default)
- Transformation Matrix

This is a variant of the [Read Image (ITK)](ITKImageReaderFilter.md) filter that also handles the transformation matrix embedded in the MHA file.

### Use of the Transformation Matrix Notes

There is an option to use the transpose of the Transformation Matrix. This can be useful if the stored transformation matrix is not the correct active transformation. If the determinant of the transformation matrix is NOT 1.0 (or really close) AND the user has selected to transpose the matrix a preflight error will be thrown. Using the *transpose* of the transformation matrix **only** works if the transformation matrix purely represents a rotation: Other affine transforms are **NOT** allowed such as shear, scale and translation.

#### Technical Discussion

For a pure rotation matrix (no translation, scaling, or shear) the transpose is equal to the inverse, because rotation matrices are orthogonal (their rows and columns are orthonormal). This is why transposing is only valid when the transformation represents a pure rotation.

### Interpolation Type

The *Interpolation Type* parameter controls the interpolation method used when resampling the image data:

- **Nearest Neighbor [0]**: Uses the value of the nearest voxel when resampling. Preserves original values but may produce blocky results.
- **Linear Interpolation [1]**: Computes a weighted average of surrounding voxels when resampling. Produces smoother results but may introduce new values.

% Auto generated parameter table will be inserted here

## Example Pipelines

None.

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
