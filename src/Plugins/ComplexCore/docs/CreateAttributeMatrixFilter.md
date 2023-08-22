# Create Attribute Matrix  #


## Group (Subgroup) ##

Core (Generation)


## Description ##

This **Filter** creates a new **Attribute Matrix**.

### Example Usage ###

If you wanted to create an **Attribute Matrix** to represent a 3D volume where the dimensions of the 3 orthogonal axesare X=3, y=4 and Z=5, then the _Tuple Dimensions_ would have a value of (3, 4, 5).

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| DataObject Path | DataPath | Where **Attribute Matrix** will be created |
| Attribute Matrix Dimensions | Any | An array that contains the size of each tuple dimension |

## Required Geometry ##

Not Applicable

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Data Container**  | None | N/A | N/A | **Data Container** for the created **Attribute Matrix**  |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Matrix**  | None | Any | N/A | Created **Attribute Matrix** name  |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this **Plugin**



## Python Filter Arguments

+ module: complex
+ Class Name: CreateAttributeMatrixFilter
+ Displayed Name: Create Attribute Matrix

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| Data_Object_Path | DataObject Path | The complete path to the Attribute Matrix being created | complex.DataGroupCreationParameter |
| tuple_dimensions | Attribute Matrix Dimensions (Slowest to Fastest Dimensions) | Slowest to Fastest Dimensions | complex.DynamicTableParameter |

