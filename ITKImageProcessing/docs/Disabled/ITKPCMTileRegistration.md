# PCM Tile Registration #


## Group (Subgroup) ##

ITKImageProcessing (Input)


## Description ##

Registers tiles into a montage using PCM algorithm. Tiles are contained in a set of input data containers with names ending in rXcX where X represents the row and column number.

## Parameters ##

| Name             |  Type  |
|------------------|--------|
| Montage Size | int x 3 |
| Image Data Containers | DataContainerProxy |
| Image Data Array Path | DataArrayPath |

## Required DataContainers ##

NONE

## Required Objects ##

NONE

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container** | Data Container Name | N/A | N/A |  |
| ImageGeometry |  | N/A | N/A |  |
| Cell AttributeMatrix |  | N/A | N/A |  |
| Image Data |  | N/A | N/A |  |


## Authors: ##

**Contact Info** dream3d@bluequartz.net

**Version** 1.0.0

**License**  See the License.txt file that came with DREAM3D.

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users

