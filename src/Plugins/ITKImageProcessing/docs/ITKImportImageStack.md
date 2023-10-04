# ITK::Import Images (3D Stack)

This filter directly wraps an ITK filter of the same name.

## Group (Subgroup)

ITKImageProcessing (ITKImageProcessing)

## Description

Read in a stack of 2D images into a 3D volume with ITK. Supports most common
scalar pixel types and the many file formats supported by ITK.
Images are read in as an **Image Geomotry**. The user must specify the origin
in physical space and resolution (uniform physical size of the resulting **Cells**).

## Parameters

- Input Directory
- File Ordering (Ascending or Descending)
- Origin
- Resolution

## Required Objects

None

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Data Container | ImageDataContainer | N/A | N/A | Created **Data Container** name **Image Geometry |
|   Attribute Matrix   | CellData | Cell | N/A | Created **Cell Attribute Matrix** name  |
| Cell Attribute Array | ImageData | Varies based on input | (n) |  |

## Example Pipelines

- (08) Image Initial Visualization
- (09) Image Segmentation

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
