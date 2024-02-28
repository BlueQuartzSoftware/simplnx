# ITK MHA File Reader

This filter reads the image data from an MHA file.

The transformation matrix from the MHA file can be optionally read and saved as a data array and/or applied to the created image geometry.

## Group (Subgroup)

ITKImageProcessing (ITKImageProcessing)

## Description

Reads MHA images and their transformation matrices using ITK. Some select headers from the MHA file are read. If those headers are not available then
then default values are used.

- Number of Dimensions
- Center of Rotation (0,0,0 is the default)
- Offset (used at the origin for the created Image Geometry)
- Transformation Matrix

### Use of the Transformation Matrix Notes

There is an option to use the transpose of the Transformation Matrix. This can be useful if the stored transformation matrix is not the correct active transformation. If the determinant of the transformation matrix is NOT 1.0 (or really close) AND the user has selected to transpose the matrix a preflight error will be thrown. Using the *transpose* of the transformation matrix **only** works if the transformation matrix is purely represents a rotation: Other affine transforms are **NOT** allowed such as shear, scale and translation.

% Auto generated parameter table will be inserted here

## Example Pipelines

None.

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
