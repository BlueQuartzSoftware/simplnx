# Find Volume Fractions of Ensembles

## Group (Subgroup) ##

Statistics (Morphological)

## Description ##

This **Filter** determines the volume fraction of each **Ensemble**. The **Filter** counts the number of **Cells** belonging to each **Ensemble** and stores the number fraction.

## Parameters ##

None 

## Required Geometry ##

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | Phases | int32 | (1) | Specifies to which **Ensemble** each **Cell** belongs |
| **Attribute Matrix** | CellEnsembleData | Ensemble AttributeMatrix | N/A | The path to the cell ensemble **Attribute Matrix** |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Ensemble Attribute Array** | VolFractions | float | (1) | Fraction of volume that belongs to each **Ensemble** |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: FindVolFractionsFilter
+ Displayed Name: Find Volume Fractions of Ensembles

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_ensemble_attribute_matrix_path | Cell Ensemble Attribute Matrix | The path to the cell ensemble attribute matrix where the output volume fractions array will be stored | complex.DataGroupSelectionParameter |
| cell_phases_array_path | Cell Phases | Array specifying which Ensemble each Cell belong | complex.ArraySelectionParameter |
| vol_fractions_array_path | Volume Fractions | Fraction of volume that belongs to each Ensemble | complex.DataObjectNameParameter |

