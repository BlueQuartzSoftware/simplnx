# ITK::Image Writer (KW)

## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

This **Filter** will save images based on an array that represents grayscale, RGB or ARGB color values. If the input array represents a 3D volume, the **Filter** will output a series of slices along one of the orthogonal axes.  The options are to produce XY slices along the Z axis, XZ slices along the Y axis or YZ slices along the X axis. The user has the option to save in one of 3 standard image formats: TIF, BMP, or PNG. The output files will be numbered sequentially starting at zero (0) and ending at the total dimensions for the chosen axis. For example, if the Z axis has 117 dimensions, 117 XY image files will be produced and numbered 0 to 116. Unless the data is a single slice then only a single image will be produced using the name given in the Output File parameter.

An example of a **Filter** that produces color data that can be used as input to this **Filter** is the [Generate IPF Colors](generateipfcolors.html) **Filter**, which will generate RGB values for each voxel in the volume.

## Parameters ##

| Name             | Type | Description |
|------------------|------| --------------|
| Output File | String | Path to the output file to write. |
| Plane | Enumeration | Selection for plane normal for writing the images (XY, XZ, or YZ) |
| Index Offeset | Integer | Default=0, This is the starting index when writing mulitple images |

## Required Geometry ##

Image 

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None| uint8_t | (n) | Selected color data for output image. The data should represent grayscale, RGB or ARGB color values. The dimensionality of the array depends on the kind of image read: (1) for grayscale, (3) for RGB, and (4) for ARGB |


## Created Objects ##

None

## Example Pipelines ##

+ INL Export
+ TxCopper_Exposed
+ TxCopper_Unexposed
+ Edax IPF Colors

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKImageWriter
+ Displayed Name: ITK Image Export

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| file_name | Output File | Path to the output file to write. | complex.FileSystemPathParameter |
| image_array_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |
| index_offset | Index Offset | This is the starting index when writing mulitple images | complex.UInt64Parameter |
| plane | Plane | Selection for plane normal for writing the images (XY, XZ, or YZ) | complex.ChoicesParameter |

