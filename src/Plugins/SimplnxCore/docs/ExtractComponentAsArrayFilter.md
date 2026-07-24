# Extract/Remove Components

## Group (Subgroup)

Core (Memory/Management)

## Description

This **Filter** operates on a single component of a multicomponent **Attribute Array**, supporting three modes of behavior:

- **Remove only** (default): the selected component is deleted from the source array; no new array is created.
- **Extract only** (*Move Extracted Components To New Array* = true, *Remove from Original* = false): the selected component is copied into a new scalar **DataArray**; the source array is unchanged.
- **Extract and remove** (*Move Extracted Components To New Array* = true, *Remove from Original* = true): the selected component is moved into a new scalar **DataArray** and removed from the source.

### Component Indexing

The component selector is **0-based**. For a 3-component array, valid component indices are 0, 1, and 2.

### When to Use This Filter

Use to pull a single channel out of a color image (e.g., extract R from an RGB array), strip a known-bad component from a tensor, or split one specific component while leaving the rest intact. For splitting *all* components into separate arrays at once, use [Split Data Array (By Component)](SplitDataArrayByComponentFilter.md) instead.

### Required Input Sources

- **Source Multi-Component Array** -- any **Attribute Array** with two or more components.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
