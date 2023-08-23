# ITK H Minima Image Filter (ITKHMinimaImage)

Suppress local minima whose depth below the baseline is less than h.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

HMinimaImageFilter suppresses local minima that are less than h intensity units below the (local) background. This has the effect of smoothing over the "low" parts of the noise in the image without smoothing over large changes in intensity (region boundaries). See the HMaximaImageFilter to suppress the local maxima whose height is less than h intensity units above the (local) background.

If original image is subtracted from the output of HMinimaImageFilter , the significant "valleys" in the image can be identified. This is what the HConcaveImageFilter provides.

This filter uses the GrayscaleGeodesicErodeImageFilter . It provides its own input as the "mask" input to the geodesic dilation. The "marker" image for the geodesic dilation is the input image plus the height parameter h.

Geodesic morphology and the H-Minima algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.* GrayscaleGeodesicDilateImageFilter , HMinimaImageFilter , HConvexImageFilter 
* MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Height | float64 | Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background. |
| FullyConnected | bool | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. |

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
+ Class Name: ITKHMinimaImage
+ Displayed Name: ITK H Minima Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| fully_connected | FullyConnected | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. | complex.BoolParameter |
| height | Height | Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background. | complex.Float64Parameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

