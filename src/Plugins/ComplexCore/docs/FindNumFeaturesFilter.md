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

## DREAM.3D Mailing Lists ##

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)




## Python Filter Arguments

+ module: complex
+ Class Name: FindNumFeaturesFilter
+ Displayed Name: Find Number of Features

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| ensemble_attribute_matrix_path | Ensemble Attribute Matrix | The path to the ensemble attribute matrix where the number of features array will be stored | complex.DataGroupSelectionParameter |
| feature_phases_array_path | Feature Phases | Array specifying which Ensemble each Feature belongs | complex.ArraySelectionParameter |
| num_features_array_path | Number of Features | The number of Features that belong to each Ensemble | complex.DataObjectNameParameter |

