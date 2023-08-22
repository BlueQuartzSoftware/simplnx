# ITK Not Image Filter (ITKNotImage)

Implements the NOT logical operator pixel-wise on an image.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

This class is templated over the type of an input image and the type of the output image. Numeric conversions (castings) are done by the C++ defaults.

Since the logical NOT operation operates only on boolean types, the input type must be implicitly convertible to bool, which is only defined in C++ for integer types, the images passed to this filter must comply with the requirement of using integer pixel type.

The total operation over one pixel will be

\code
if( !A )

 {

 return this->m_ForegroundValue;

 }

return this->m_BackgroundValue;

\endcode


Where "!" is the unary Logical NOT operator in C++.

## Parameters

| Name | Type | Description |
|------|------|-------------|

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching IntegerPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching IntegerPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKNotImage
+ Displayed Name: ITK Not Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

