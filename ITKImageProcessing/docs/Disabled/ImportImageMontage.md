# Import Image Montage #


## Group (Subgroup) ##

ITKImageProcessing (Input)


## Description ##

Imports multiple images for the purpose of montage assembly. These images must be 8 bit greyscale, and have the same X pixel dimensions and the same Y pixel dimensions. Each image is stored in it's own attribute array.

Utilizes the *itkReadImage* filter.

## Parameters ##

| Name             |  Type  |
|------------------|--------|
| Input File List | VectorFileListInfo type |
| Origin | float x 3 |
| Resolution | float x 3 |
| Data Container Name | String |
| Cell Attribute Matrix Name | String |
| Data Array Name | String |

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

