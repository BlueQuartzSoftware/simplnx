# Convert Quaternion Order

## Group (Subgroup) ##

OrientationAnalysis (Conversions)

## Description ##

Internally DREAM.3D assumes that a quaternion is laid out in the order such that < x, y, z >, w or Vector-Scalar ordering. Codes and algorithms external to DREAM.3D may store quaternions in the opposite or Scalar-Vector order (w < x,y,z >). This filter will allow the user to easily convert imported Quaternions into the representation that DREAM.3D expects.

For Example if the user has imported quaternion data in the form of Scalar-Vector then they would run this filter using a conversion type of **ToVectorScalar** and using the generated quaternions in subsequent filters. If the user wanted to then write out the Quaternions in the Scalar-Vector form then could add this filter again to the end of the pipeline (but before writing out data) to convert the Vector-Scalar quaternions array (assuming something modified the array).

## Parameters ##

| Name | Type | Description |
|------|------|------|
| Delete Original Array | Boolean | Set this to TRUE/ON to have the original 3 component data array deleted at the end of the filter. |
| Conversion Type | Integer | 0=ToScalarVector, 1=ToVectorScalar |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Attribute Array** | Quaternion Input | float | (4) |  |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **Attribute Array** | Quaternion Ouput | float | (4) |  |


## Example Pipelines ##


## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: OrientationAnalysis
+ Class Name: ConvertQuaternionFilter
+ Displayed Name: Convert Quaternion Order

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| conversion_type | Conversion Type | The conversion type: To Scalar Vector=0, To Vector Scalar=1 | complex.ChoicesParameter |
| delete_original_data | Delete Original Data | Should the original quaternions array be deleted from the DataStructure | complex.BoolParameter |
| output_data_array_path | Output Quaternions | The DataPath to the converted quaternions | complex.DataObjectNameParameter |
| quaternion_data_array_path | Input Quaternions | Specifies the quaternions to convert | complex.ArraySelectionParameter |

