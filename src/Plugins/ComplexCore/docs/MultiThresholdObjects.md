# Threshold Objects 2 


## Group (Subgroup) ##

Processing (Threshold)

## Description ##

This **Filter** allows the user to input single or multiple criteria for thresholding **Attribute Arrays** in an **Attribute Matrix**. Internally, the algorithm creates the output boolean arrays for each comparison that the user creates.  Comparisons can be either a value and boolean operator (*Less Than*, *Greater Than*, *Equal To*, *Not Equal To*) or a collective set of comparisons. Then all the output arrays are compared with their given comparison operator ( *And* / *Or* ) with the value of a set being the result of its own comparisons calculated from top to bottom.

An example of this **Filter's** use would be after EBSD data is read into DREAM.3D and the user wants to have DREAM.3D consider **Cells** that the user considers *good*. The user would insert this **Filter** and select the criteria that makes a **Cell** *good*. All arrays **must** come from the same **Attribute Matrix** in order for the **Filter** to execute.

For example, an integer array contains the values 1, 2, 3, 4, 5. For a comparison value of 3 and the comparison operator greater than, the boolean threshold array produced will contain *false*, *false*, *false*, *true*, *true*. For the comparison set { *Greater Than* 2 AND *Less Than* 5} OR *Equals* 1, the boolean threshold array produced will contain *true*, *false*, *true*, *true*, *false*.

It is possible to set custom values for both the TRUE and FALSE values that will be output to the threshold array.  For example, if the user selects an output threshold array type of uint32, then they could set a custom FALSE value of 5 and a custom TRUE value of 20.  So then instead of outputting 0's and 1's to the threshold array, the filter would output 5's and 20's.

**NOTE**: If custom TRUE/FALSE values are chosen, then using the resulting mask array in any other filters that require a mask array will break those other filters.  This is because most other filters that require a mask array make the assumption that the true/false values are 1/0.

## Parameters ##

| Name                     | Type            | Description                                                                                                    |
|--------------------------|-----------------|----------------------------------------------------------------------------------------------------------------|
| Data Arrays to Threshold | Comparison List | This is the set of criteria applied to the objects the selected arrays correspond to when doing the thresholding |
| Use Custom TRUE Value    | bool            | Specifies whether to output a custom TRUE value                                                                |
| Custom TRUE Value        | float64         | The custom TRUE value (the default value is 1)                                                                 |
| Use Custom FALSE Value   | bool            | Specifies whether to output a custom FALSE value                                                               |
| Custom FALSE Value       | float64         | The custom FALSE value (the default value is 0)                                                                |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any | (1) | Array(s) selected in criteria set |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | Mask | bool | (1) | Specifies whether the objects passed the set of criteria applied during thresholding |


## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: MultiThresholdObjects
+ Displayed Name: Multi-Threshold Objects

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| array_thresholds | Data Thresholds | DataArray thresholds to mask | complex.ArrayThresholdsParameter |
| created_data_path | Mask Array | DataPath to the created Mask Array | complex.DataObjectNameParameter |
| created_mask_type | Mask Type | DataType used for the created Mask Array | complex.DataTypeParameter |

