# Find NeighborList Statistics

## Group (Subgroup) ##

Statistics (Misc Filters)

## Description ##

This **Filter** finds the selected statistics for each list contained in a NeighborList container. Each of those statistics are reported back as new Attribute Arrays. The user selectable statistics are:

+ Length of each list
+ Minimum value from each list
+ Maximum value from each list
+ Mean of each list
+ Median of each list
+ Standard Deviation of each list
+ Summation of each list

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
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

## Required Geometry ###

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Matrix** | Attribute Matrix Name | N/A | N/A | Output Attribute Matrix |
| **Input NeighborList** | NeighborList Name | int32_t/float/etc. | (1) | The input NeighborList. Boolean Not allowed. |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Length Attribute Array** | Length | float | (1) | Output Length Array |
| **Minimum Attribute Array** | Minimum | float | (1) | Output Minimum Array |
| **Maximum Attribute Array** | Maximum | float | (1) | Output Maximum Array |
| **Mean Attribute Array** | Mean | float | (1) | Output Mean Array |
| **Median Attribute Array** | Median | float | (1) | Output Median Array |
| **Standard Deviation Attribute Array** | Standard Deviation | float | (1) | Output Standard Deviation Array |
| **Summation Attribute Array** | Summation | float | (1) | Output Summation Array |

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
[https://groups.google.com/forum/?hl=en#!forum/dream3d-users](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: FindNeighborListStatistics
+ Displayed Name: Find Neighbor List Statistics

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| find_length | Find Length | Specifies whether or not the filter creates the Length array during calculations | complex.BoolParameter |
| find_maximum | Find Maximum | Specifies whether or not the filter creates the Maximum array during calculations | complex.BoolParameter |
| find_mean | Find Mean | Specifies whether or not the filter creates the Mean array during calculations | complex.BoolParameter |
| find_median | Find Median | Specifies whether or not the filter creates the Median array during calculations | complex.BoolParameter |
| find_minimum | Find Minimum | Specifies whether or not the filter creates the Minimum array during calculations | complex.BoolParameter |
| find_standard_deviation | Find Standard Deviation | Specifies whether or not the filter creates the Standard Deviation array during calculations | complex.BoolParameter |
| find_summation | Find Summation | Specifies whether or not the filter creates the Summation array during calculations | complex.BoolParameter |
| input_array | NeighborList to Compute Statistics | Input Data Array to compute statistics | complex.NeighborListSelectionParameter |
| length | Length | Path to create the Length array during calculations | complex.DataObjectNameParameter |
| maximum | Maximum | Path to create the Maximum array during calculations | complex.DataObjectNameParameter |
| mean | Mean | Path to create the Mean array during calculations | complex.DataObjectNameParameter |
| median | Median | Path to create the Median array during calculations | complex.DataObjectNameParameter |
| minimum | Minimum | Path to create the Minimum array during calculations | complex.DataObjectNameParameter |
| standard_deviation | Standard Deviation | Path to create the Standard Deviation array during calculations | complex.DataObjectNameParameter |
| summation | Summation | Path to create the Summation array during calculations | complex.DataObjectNameParameter |

