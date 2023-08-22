# ITK::Import Images (3D Stack)

This filter directly wraps an ITK filter of the same name.

## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Read in a stack of 2D images into a 3D volume with ITK. Supports most common
scalar pixel types and the many file formats supported by ITK.
Images are read in as an **Image Geomotry**. The user must specify the origin
in physical space and resolution (uniform physical size of the resulting **Cells**).

## Parameters ##

- Input Directory
- File Ordering (Ascending or Descending)
- Origin
- Resolution

## Required Objects ##

None

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | ImageDataContainer | N/A | N/A | Created **Data Container** name **Image Geometry** |
| **Attribute Matrix** | CellData | Cell | N/A | Created **Cell Attribute Matrix** name  |
| **Cell Attribute Array**  | ImageData | Varies based on input | (n) |  |

## Example Pipelines ##

+ (08) Image Initial Visualization
+ (09) Image Segmentation

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKImportImageStack
+ Displayed Name: ITK Import Images (3D Stack)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_data_name | Cell Data Name | The name of the created cell attribute matrix | complex.DataObjectNameParameter |
| image_data_array_path | Created Image Data | The path to the created image data array | complex.DataObjectNameParameter |
| image_geometry_path | Created Image Geometry | The path to the created Image Geometry | complex.DataGroupCreationParameter |
| image_transform_choice | Optional Slice Operations | Operation that is performed on each slice. 0=None, 1=Flip on X, 2=Flip on Y | complex.ChoicesParameter |
| input_file_list_info | Input File List | The list of 2D image files to be read in to a 3D volume | complex.GeneratedFileListParameter |
| origin | Origin | The origin of the 3D volume | complex.VectorFloat32Parameter |
| spacing | Spacing | The spacing of the 3D volume | complex.VectorFloat32Parameter |

