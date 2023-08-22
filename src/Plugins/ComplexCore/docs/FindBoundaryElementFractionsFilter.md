# Find Feature Boundary Element Fractions


## Group (Subgroup)

Statistics (Morphological)

## Description

This **Filter** calculates the fraction of **Elements** of each **Feature** that are on the "surface" of that **Feature**.  The **Filter** simply iterates through all **Elements** asking for the **Feature** that owns them and if the **Element** is a "surface" **Element**.  Each **Feature** counts the total number of **Elements** it owns and the number of those **Elements** that are "surface" **Elements**.  The fraction is then stored for each **Feature**.

## Parameters

None

## Required Geometry

Not Applicable

## Required Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Element Attribute Array** | Feature Ids | int32 | (1) | Specifies to which **Feature** each **Element** belongs |
| **Element Attribute Array** | Surface Elements | int32 | (1) | The number of neighboring **Elements** of a given **Element** that belong to a different **Feature** than itself |
| **Attribute Matrix** | Feature Data | AttributeMatrix | N/A | Parent Attribute Matrix for the *Surface Element Fractions* Array to be created in |

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Feature Attribute Array** | Surface Element Fractions | float32 | (1) | Fraction of **Elements** belonging to the **Feature** that are "surface" **Elements** |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM.3D Mailing Lists

If you need more help with a **Filter**, please consider asking your question on the [DREAM.3D Users Google group!](https://groups.google.com/forum/?hl=en#!forum/dream3d-users)


## Python Filter Arguments

+ module: complex
+ Class Name: FindBoundaryElementFractionsFilter
+ Displayed Name: Find Feature Boundary Element Fractions

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| boundary_cell_fractions_array_name | Surface Element Fractions | Name of created Data Array containing fraction of Elements belonging to the Feature that are "surface" Elements | complex.DataObjectNameParameter |
| boundary_cells_array_path | Surface Elements | DataArray containing the number of neighboring Elements of a given Element that belong to a different Feature than itself | complex.ArraySelectionParameter |
| feature_data_attribute_matrix_path | Feature Data | Parent Attribute Matrix for the Surface Element Fractions Array to be created in | complex.AttributeMatrixSelectionParameter |
| feature_ids_array_path | Feature Ids | Data Array that specifies to which Feature each Element belongs | complex.ArraySelectionParameter |

