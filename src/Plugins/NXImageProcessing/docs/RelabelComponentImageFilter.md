# Relabel Component Image Filter

Relabel the components in an already-labeled image such that consecutive labels are used.

## Group (Subgroup)

ImageProcessing (Segmentation)

## Description

Remaps the labels of an already-labeled image (as from the output of the Connected Component Image Filter) so that the label numbers are consecutive with no gaps. By default, the relabeling also sorts the labels by the size (pixel count) of the object: the largest object receives label 1, the second largest label 2, and so on. If two objects have the same size, their original relative label order is kept. Sorting by size can be disabled with **Sort By Object Size**.

Label 0 is always treated as the background and is left unaltered.

If **Minimum Object Size** is set above 0, every object with fewer pixels than the minimum is discarded (mapped to the background) before the consecutive labels are assigned, so the surviving objects are still numbered with no gaps.

The input array must be single-component (scalar) and may be **any integer type** (signed or unsigned). The output has the **same type as the input** (type-preserving); this filter does **not** perform connected-component labeling itself, it only relabels an already-labeled image. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Relabel Component Image Filter, and matches it exactly (byte-identical label values).

### Minimum Object Size

The minimum size, in pixels, for an object to survive relabeling. Objects smaller than this are discarded and mapped to background (0). Default is 0 (no objects discarded).

### Sort By Object Size

Controls whether the object labels are sorted by size (largest first). If false, the initial label order (ascending original label) is kept. Default is On.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
