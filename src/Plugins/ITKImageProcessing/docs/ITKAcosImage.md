# ITK Acos Image Filter (ITKAcosImage)

Computes the inverse cosine of each pixel.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

This filter is templated over the pixel type of the input image and the pixel type of the output image.

The filter walks over all the pixels in the input image, and for each pixel does do the following:

- cast the pixel value to double , 
- apply the std::acos() function to the double value 
- cast the double value resulting from std::acos() to the pixel type of the output image 
- store the casted value into the output image.



The filter expects both images to have the same dimension (e.g. both 2D, or both 3D, or both ND).

## Parameters

| Name | Type | Description |
|------|------|-------------|

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching BasicPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching BasicPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKAcosImage
+ Displayed Name: ITK Acos Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

