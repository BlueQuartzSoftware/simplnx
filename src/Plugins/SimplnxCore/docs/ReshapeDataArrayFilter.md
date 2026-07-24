# Reshape Data Array

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** updates the **tuple dimensions** (the *shape* metadata) of an existing **Data Array**, **Neighbor List**, or **String Array** in place. The total number of tuples is unchanged; only how the shape is described is changed.

## ⚠ CRITICAL WARNING: Values Are Not Reordered

**This filter does NOT move any data in memory.** It only updates the tuple-dimensions metadata. If the data's existing memory layout does not match the new shape's expected layout, **the filter will silently produce incorrectly-shaped data** that downstream filters will misinterpret.

### When This Is Safe vs Dangerous

DREAM3DNX uses **C-order (row-major)** layout: the last dimension varies fastest in memory. So `(3, 45)` is stored as 3 rows × 45 columns -- 3 contiguous chunks of 45 values each.

- **Safe**: changing `(135)` → `(3, 45)` simply tells the system "treat this 135-tuple array as a 3×45 array". The bytes in memory are the same; only the shape interpretation changes.
- **Safe**: changing `(3, 45)` → `(135)` flattens to 1D. Memory is identical.
- **DANGEROUS**: reading an array from an HDF5 file that was stored 3 rows × 45 columns and trying to use it as 3-component 45-vertex data. In DREAM3DNX, 3-vertex data is stored as `(N, 3)` with 3 contiguous values per tuple. Calling Reshape from `(3, 45)` to `(45, 3)` **does not transpose** -- it just relabels the shape, and the bytes are now in the wrong order for the new shape.

To actually transpose or reorder data, use external preprocessing (e.g., reshape the HDF5 dataset before reading) or use an array-arithmetic filter to manually rebuild the array with the desired ordering.

### Validation

The filter validates that the new tuple dimensions are positive and that the total tuple count matches the array's existing total. A shape change that would alter the total tuple count fails preflight.

### Neighbor Lists and String Arrays

Neighbor Lists and String Arrays do not support multi-dimensional tuple shapes. If you specify a multi-dimensional new shape for one of these array types, the filter emits a warning and falls back to an equivalent 1-D tuple count.

### Required Input Sources

- **Array to Reshape** -- any **Data Array**, **Neighbor List**, or **String Array** in the Data Structure.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
