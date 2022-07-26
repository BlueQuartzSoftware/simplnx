# ITK::Import Images (3D Stack) (KW)  #


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

## DREAM3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)
