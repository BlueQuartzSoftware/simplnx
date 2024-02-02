# Split Multicomponent Attribute Array

## Group (Subgroup)

DREAM3D Review (Memory/Management)

## Description

This **Filter** splits an n-component **Attribute Array** into **n** scalar arrays, where each array is one of the original components.  Any arbitrary component array may be split in this manner, and the output arrays will have the same primitive type as the input array.  The original array is not modified (unless the option to remove the original array is selected); instead, **n** new arrays are created.  For example, consider an unsigned 8-bit array with three components:

    { v1 v2 v3 }, { v4 v5 v6 }, { v7 v8 v9 } ...
  
This **Filter** will produce three new scalar unsigned 8-bit arrays:

    { v1 }, { v4 }, { v7 } ...
    { v2 }, { v5 }, { v8 } ...
    { v3 }, { v6 }, { v9 } ...

The user must specificy a postfix string to add to the newly created arrays. For example, if the original multicomponent **Attribute Array** is named "Foo" and the postfix is set to "Component", this **Filter** will produce three new arrays named "FooComponent0", "FooComponent1", and "FooComponent2".  The numbering will always be present regardless of how the postfix is set.  

There is an alternative option which allows the user to select a subset of components to extract instead of extracting all the components by entering the components to be extracted.  The components should be specified starting with the first componet as 0.  So if the original array has 3 components and the user wanted the first and second components, the unput to the component table should be 0 and 1 respectively.

This **Filter** is the opposite operation of the Combine Attribute Arrays **Filter**, and the generalized version of the Extract Component as Attribute Array **Filter**.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
