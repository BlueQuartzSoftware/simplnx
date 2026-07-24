# Split Data Array (By Component)

## Group (Subgroup)

DREAM3D Review (Memory/Management)

## Description

This **Filter** splits an n-component **Attribute Array** into **n** scalar arrays, where each array is one of the original components.  Any arbitrary component array may be split in this manner, and the output arrays will have the same primitive type as the input array.  The original array is not modified (unless the option to remove the original array is selected); instead, **n** new arrays are created.  For example, consider an unsigned 8-bit array with three components:

    { v1 v2 v3 }, { v4 v5 v6 }, { v7 v8 v9 } ...
  
This **Filter** will produce three new scalar unsigned 8-bit arrays:

    { v1 }, { v4 }, { v7 } ...
    { v2 }, { v5 }, { v8 } ...
    { v3 }, { v6 }, { v9 } ...

The user must specify a postfix string to add to the newly created arrays. For example, if the original multicomponent **Attribute Array** is named "Foo" and the postfix is set to "Component", this **Filter** will produce three new arrays named "FooComponent0", "FooComponent1", and "FooComponent2".  The numbering will always be present regardless of how the postfix is set.  

An alternative *Select Components* option lets the user extract a subset of components rather than all of them. Components are **0-based** -- for a 3-component array, entering 0 and 1 extracts only the first two components.

This **Filter** is the inverse of [Combine Attribute Arrays](CombineAttributeArraysFilter.md), and a generalized version of [Extract/Remove Components](ExtractComponentAsArrayFilter.md).

**Looking to split by *tuples* instead?**  

See the *[Split Data Array (By Tuple)](SplitDataArrayByTupleFilter.md)* filter that separates a data array into several smaller data arrays based on the tuple layout.

### Required Input Sources

- **Multi-Component Input Array** -- any multi-component **Attribute Array**.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
