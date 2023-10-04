# Find NeighborList Statistics

## Group (Subgroup)

Statistics (Misc Filters)

## Description

This **Filter** finds the selected statistics for each list contained in a NeighborList container. Each of those statistics are reported back as new Attribute Arrays. The user selectable statistics are:

+ Length of each list
+ Minimum value from each list
+ Maximum value from each list
+ Mean of each list
+ Median of each list
+ Standard Deviation of each list
+ Summation of each list

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| Find Length | Bool | Find the Length of each List |
| Find Minimum | Bool | Find the Minimum of each List |
| Find Maximum | Bool | Find the Maximum of each List |
| Find Mean | Bool | Find the Mean of each List |
| Find Median | Bool | Find the Median of each List |
| Find Standard Deviation | Bool | Find the Standard Deviation of each List |
| Find Summation | Bool | Find the Summation of each List |
| Input Neighbor List  | NeighborList\<T\> | The input NeighborList |
| Output AttributeMatrix  | DataArrayPath | The Output NeighborList |
| Length Array Name | String | The name of the Output Length Attribute Array |
| Minimum Array Name | String | The name of the Output Minimum Attribute Array |
| Maximum Array Name | String | The name of the Output Maximum Attribute Array |
| Mean Array Name | String | The name of the Output Mean Attribute Array |
| Median Array Name | String | The name of the Output Median Attribute Array |
| Standard Deviation Array Name | String | The name of the Output Standard Deviation Attribute Array |
| Summation Array Name | String | The name of the Output Summation Attribute Array |

## Required Geometry #

Not Applicable

## Required Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
|   Attribute Matrix   | Attribute Matrix Name | N/A | N/A | Output Attribute Matrix |
| Input NeighborList | NeighborList Name | int32_t/float/etc. | (1) | The input NeighborList. Boolean Not allowed. |

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Length Attribute Array | Length | float | (1) | Output Length Array |
| Minimum Attribute Array | Minimum | float | (1) | Output Minimum Array |
| Maximum Attribute Array | Maximum | float | (1) | Output Maximum Array |
| Mean Attribute Array | Mean | float | (1) | Output Mean Array |
| Median Attribute Array | Median | float | (1) | Output Median Array |
| Standard Deviation Attribute Array | Standard Deviation | float | (1) | Output Standard Deviation Array |
| Summation Attribute Array | Summation | float | (1) | Output Summation Array |

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
