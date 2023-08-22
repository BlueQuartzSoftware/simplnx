# ITK::Image Reader

This filter directly wraps an ITK filter of the same name.

## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Reads images through ITK

## Parameters ##

| Name             | Type |
|------------------|------|
| Input File | String | Path to the input file to read. |

## Required Objects ##

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | Data Container Name | N/A | N/A | Description of object... |

## Example Pipelines ##

+ Image Histogram

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKImageReader
+ Displayed Name: ITK Image Reader

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_data_name | Cell Data Name | The name of the created cell attribute matrix | complex.DataObjectNameParameter |
| file_name | File | Input image file | complex.FileSystemPathParameter |
| geometry_path | Created Image Geometry | The path to the created Image Geometry | complex.DataGroupCreationParameter |
| image_data_array_path | Created Image Data | The path to the created image data array | complex.ArrayCreationParameter |

