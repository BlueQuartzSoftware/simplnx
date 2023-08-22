# Rodrigues Convertor

## Group (Subgroup) ##

OrientationAnalysis (Processing)

## Description ##

This *filter* will convert a 3 component Rodrigues vector into a 4 component vector that DREAM.3D expects in its
equations and algorithms. The algorithm is the following:

+ Find the length of the 3 component vector.
+ Divide each component by the length
+ Store the length as the 4th component in the output array

## Parameters ##

| Name                  | Type    | Description                                                                                       |
|-----------------------|---------|---------------------------------------------------------------------------------------------------|
| Delete Original Array | Boolean | Set this to TRUE/ON to have the original 3 component data array deleted at the end of the filter. |

## Required Geometry ##

Required Geometry Type -or- Not Applicable

## Required Objects ##

| Kind                | Default Name           | Type  | Component Dimensions | Description |
|---------------------|------------------------|-------|----------------------|-------------|
| **Attribute Array** | Rodrigues Input Vector | float | (3)                  |             |

## Created Objects ##

| Kind                | Default Name           | Type  | Component Dimensions | Description |
|---------------------|------------------------|-------|----------------------|-------------|
| **Attribute Array** | Rodrigues Ouput Vector | float | (4)                  |             |

## Example Pipelines ##

List the names of the example pipelines where this filter is used.

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: RodriguesConvertorFilter
+ Displayed Name: Rodrigues Convertor

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| delete_original_data | Delete Original Data | Should the original Rodrigues data array be deleted from the DataStructure | complex.BoolParameter |
| output_data_array_path | Converted Rodrigues Data Array | The DataArray name of the converted Rodrigues vectors | complex.DataObjectNameParameter |
| rodrigues_data_array_path | Input Rodrigues Vectors | Specifies the Rodrigues data to convert | complex.ArraySelectionParameter |

