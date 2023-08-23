# ITK Invert Intensity Image Filter (ITKInvertIntensityImage)

Invert the intensity of an image.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

InvertIntensityImageFilter inverts intensity of pixels by subtracting pixel value to a maximum value. The maximum value can be set with SetMaximum and defaults the maximum of input pixel type. This filter can be used to invert, for example, a binary image, a distance map, etc.

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

- IntensityWindowingImageFilter ShiftScaleImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Maximum | float64 | Set/Get the maximum intensity value for the inversion. |

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
+ Class Name: ITKInvertIntensityImage
+ Displayed Name: ITK Invert Intensity Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| maximum | Maximum | Set/Get the maximum intensity value for the inversion. | complex.Float64Parameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

