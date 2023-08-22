# ITK Signed Maurer Distance Map Image Filter (ITKSignedMaurerDistanceMapImage)

This filter calculates the Euclidean distance transform of a binary image in linear time for arbitrary dimensions.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

\par Inputs and Outputs
This is an image-to-image filter. The dimensionality is arbitrary. The only dimensionality constraint is that the input and output images be of the same dimensions and size. To maintain integer arithmetic within the filter, the default output is the signed squared distance. This implies that the input image should be of type "unsigned int" or "int" whereas the output image is of type "int". Obviously, if the user wishes to utilize the image spacing or to have a filter with the Euclidean distance (as opposed to the squared distance), output image types of float or double should be used.


The inside is considered as having negative distances. Outside is treated as having positive distances. To change the convention, use the InsideIsPositive(bool) function.

\par Parameters
Set/GetBackgroundValue specifies the background of the value of the input binary image. Normally this is zero and, as such, zero is the default value. Other than that, the usage is completely analogous to the itk::DanielssonDistanceImageFilter class except it does not return the Voronoi map.


Reference: C. R. Maurer, Jr., R. Qi, and V. Raghavan, "A Linear Time Algorithm
 for Computing Exact Euclidean Distance Transforms of Binary Images in
 Arbitrary Dimensions", IEEE - Transactions on Pattern Analysis and Machine Intelligence, 25(2): 265-270, 2003.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| InsideIsPositive | bool | Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels. |
| SquaredDistance | bool | Set if the distance should be squared. |
| UseImageSpacing | bool | Set if image spacing should be used in computing distances. |
| BackgroundValue | float64 | Set the background value which defines the object. Usually this value is = 0. |

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


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users




## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKSignedMaurerDistanceMapImage
+ Displayed Name: ITK Signed Maurer Distance Map Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| background_value | BackgroundValue | Set the background value which defines the object. Usually this value is = 0. | complex.Float64Parameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| inside_is_positive | InsideIsPositive | Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels. | complex.BoolParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |
| squared_distance | SquaredDistance | Set if the distance should be squared. | complex.BoolParameter |
| use_image_spacing | UseImageSpacing | Set if image spacing should be used in computing distances. | complex.BoolParameter |

