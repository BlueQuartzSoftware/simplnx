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

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Element Attribute Array | Feature Ids | int32 | (1) | Specifies to which **Feature** each **Element** belongs |
| Element Attribute Array | Surface Elements | int32 | (1) | The number of neighboring **Elements** of a given **Element** that belong to a different **Feature** than itself |
|   Attribute Matrix   | Feature Data | AttributeMatrix | N/A | Parent Attribute Matrix for the *Surface Element Fractions* Array to be created in |

## Created Objects

| Kind                      | Default Name | Type     | Comp. Dims | Description                                 |
|---------------------------|--------------|----------|------------|---------------------------------------------|
| Feature Attribute Array | Surface Element Fractions | float32 | (1) | Fraction of **Elements** belonging to the **Feature** that are "surface" **Elements |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


