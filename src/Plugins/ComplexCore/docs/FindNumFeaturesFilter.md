# Find Number of Features


## Group (Subgroup) ##

Statistics (Morphological)

## Description ##

This **Filter** determines the number of **Features** in each **Ensemble** by summing the total number of rows in the feature attribute matrix belonging to each phase.

## Parameters ##

None 

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | Phases | int32 | (1) | Specifies to which **Ensemble** each **Feature** belongs |
| **Attribute Matrix** | EnsembleData | Ensemble AttributeMatrix | N/A | The path to the ensemble **Attribute Matrix** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Ensemble Attribute Array** | NumFeatures | int32 | (1) | Number of **Features** that belong each **Ensemble** |

## Example Pipelines ##

+ INL Export

## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


