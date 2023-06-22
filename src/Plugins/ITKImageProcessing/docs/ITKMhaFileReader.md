# ITK MHA File Reader

This filter reads the image data from an MHA file.

The transformation matrix from the MHA file can be optionally read and saved as a data array and/or applied to the created image geometry.

*Note:* The ITK API hands back the Transform Matrix as a raw pointer to the values.  The actual Transform Matrix internal array has a size of 100 with only the first several slots filled with matrix values (the rest of the slots are 0's).  Since it's only possible to get a raw pointer and no exact size, this filter will key off of the **NDims** header value in the input .mha file to determine how many values should be grabbed from the Transform Matrix raw pointer.  So far it appears that it's 4 values (2x2) for 2D image data and 9 values (3x3) for 3D image data, but it is currently unknown whether it's possible to have >4 matrix values for 2D image data or >9 matrix values for 3D image data.  Therefore, this filter is going to make the following assumptions:

1. 2D image data has a transformation matrix with 4 values
2. 3D image data has a transformation matrix with 9 values

If these assumptions are violated, then unexpected behavior may occur.

## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Reads MHA images and their transformation matrices using ITK

## Parameters ##

| Name             | Type | Description |
|------------------|------|-------------|
| Input MHA File | String | Path to the input .mha file to read. |
| Save Image Transformation As Array | Boolean | Determines whether or not to read and save the transformation matrix found in the .mha file to a data array. |
| Apply Image Transformation To Geometry | Boolean | Determines whether or not to read and apply the transformation matrix found in the .mha file to the created image geometry. |
| Interpolation Type | Choice | The interpolation type used when transforming the cell data arrays in the created image geometry. Only available if **Apply Image Transformation To Geometry** is set to True.|

## Required Objects ##

None.

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Image Geometry** | ImageDataContainer | N/A | N/A | The created image geometry that contains the image data from the .mha file. |
| **Cell Attribute Matrix** | Cell Data | N/A | N/A | The created cell attribute matrix that contains the image data from the .mha file. |
| **Data Array** | ImageData | uint16 | 1 | The created data array that contains the image data from the .mha file. |
| **Data Array** | TransformationMatrix | float32 | 16 | 4x4 transformation matrix read from the .mha input file. Only available if **Save Image Transformation As Array** is set to True. |

## Example Pipelines ##

None.

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
