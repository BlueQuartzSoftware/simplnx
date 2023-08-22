# ITK MHA File Reader

This filter reads the image data from an MHA file.

The transformation matrix from the MHA file can be optionally read and saved as a data array and/or applied to the created image geometry.

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

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKMhaFileReader
+ Displayed Name: ITK MHA File Reader

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| apply_image_transformation | Apply Image Transformation To Geometry | When true, the transformation matrix found in the image's header metadata will be applied to the created image geometry. | complex.BoolParameter |
| cell_data_name | Cell Data Name | The name of the created cell attribute matrix | complex.DataObjectNameParameter |
| file_name | Input MHA File | The input .mha file that will be read. | complex.FileSystemPathParameter |
| geometry_path | Created Image Geometry | The path to the created Image Geometry | complex.DataGroupCreationParameter |
| image_data_array_path | Created Image Data | The path to the created image data array | complex.ArrayCreationParameter |
| interpolation_type | Interpolation Type | The type of interpolation algorithm that is used. 0=NearestNeighbor, 1=Linear | complex.ChoicesParameter |
| save_image_transformation | Save Image Transformation As Array | When true, the transformation matrix found in the image's header metadata will be saved as a data array in the created image geometry. | complex.BoolParameter |
| transformation_matrix_data_array_path | Transformation Matrix | The path to the transformation matrix data array | complex.ArrayCreationParameter |

