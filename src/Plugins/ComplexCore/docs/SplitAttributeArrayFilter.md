# Split Multicomponent Attribute Array 


## Group (Subgroup) ##

DREAM3D Review (Memory/Management)

## Description ##

This **Filter** splits an n-component **Attribute Array** into **n** scalar arrays, where each array is one of the original components.  Any arbitrary component array may be split in this manner, and the output arrays will have the same primitive type as the input array.  The original array is not modified (unless the option to remove the original array is selected); instead, **n** new arrays are created.  For example, consider an unsigned 8-bit array with three components:

    { v1 v2 v3 }, { v4 v5 v6 }, { v7 v8 v9 } ...
  
This **Filter** will produce three new scalar unsigned 8-bit arrays:

    { v1 }, { v4 }, { v7 } ...
    { v2 }, { v5 }, { v8 } ...
    { v3 }, { v6 }, { v9 } ...

The user must specificy a postfix string to add to the newly created arrays. For example, if the original multicomponent **Attribute Array** is named "Foo" and the postfix is set to "Component", this **Filter** will produce three new arrays named "FooComponent0", "FooComponent1", and "FooComponent2".  The numbering will always be present regardless of how the postfix is set.  

There is an alternative option which allows the user to select a subset of components to extract instead of extracting all the components by entering the components to be extracted.  The components should be specified starting with the first componet as 0.  So if the original array has 3 components and the user wanted the first and second components, the unput to the component table should be 0 and 1 respectively.

This **Filter** is the opposite operation of the [Combine Attribute Arrays](@ref combineattributearrays) **Filter**, and the generalized version of the [Extract Component as Attribute Array](@ref extractcomponentasarray) **Filter**.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Postfix | string | Postfix to add to the end of the split **Attribute Arrays**; this value may be empty |

## Required Geometry ###

None

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| Any **Attribute Array** | None | Any | > (1) | The multicomponent **Attribute Array** to split |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Attribute Arrays** | Input array name + _Postfix_ + component number | Same as input array | (1)| The set of scalar **Attribute Arrays**, equal to the number of components of the input **Attribute Array** |

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:


## Python Filter Arguments

+ module: complex
+ Class Name: SplitAttributeArrayFilter
+ Displayed Name: Split Multicomponent Attribute Array

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| components_to_extract | Components to Extract | The components from the input array to be extracted into separate arrays | complex.DynamicTableParameter |
| delete_original_array | Remove Original Array | Whether or not to remove the original multicomponent array after splitting | complex.BoolParameter |
| multicomponent_array | Multicomponent Attribute Array | The multicomponent Attribute Array to split | complex.ArraySelectionParameter |
| postfix | Postfix | Postfix to add to the end of the split Attribute Arrays | complex.StringParameter |
| select_components_to_extract | Select Specific Components to Extract | Whether or not to specify only certain components to be extracted | complex.BoolParameter |

