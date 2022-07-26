# Illumination Correction #

## Group (Subgroup) ##

ITKImageProcessing (Processing)

## Description ##

This filter takes a series of grayscale images (8 bit or 16 bit) in a given set of Data Containers averages them. The image content only contributes to the average if the value at a given pixel is between the lowest and highest allowed image value set by the user. The user can optionally apply a median filter to the resulting background image. The user can optionally apply the background correction to the input images. The user can optionally export the corrected images to a directory on the file system.

If the user selects Subtract Background from Current Images, the background will be subtracted, and new image data will be created.

## Parameters ##

| Name             | Type |
|------------------|------|
| List of DataContainers that have the input images. One per Data Container | String List |
| Lowest Allowed Image Value | int |
| Highest Allowed Image Value | int |
| Apply Median Filter to background Image | bool |
| Median Radius | Float [3] |
| Apply Illumination Correction to Input Images | bool |

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| DataContainer(s) |  | QStringList | N/A | List of DataContainers that contain a Cell Attribute Matrix that holds an input image. |
| String | Tile Data | Cell AttributeMatrix | N/A | Name of the Cell AttributeMatrix that is common to all the input DataContainers |
| String | Image Data | AttributeArray | 8 or 16 bit GrayScale Images | Name of the input image that is in common to the input Cell AttributeMatrix |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| Attribute Array |  | Corrected Image | N/A | The corrected image (optional) that is stored next to each input image |
| DataContainer | DataContainer Name |  | N/A |  |
| AttributeMatrix | AttributeMatrix Name |  |  |  |
| AttributeMatrix | AttributeArray Name |  |  |  |

## Example Pipelines ##

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
