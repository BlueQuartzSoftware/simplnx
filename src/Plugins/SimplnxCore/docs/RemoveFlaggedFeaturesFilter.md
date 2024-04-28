# Remove/Extract Flagged Features

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** will remove **Features** that have been flagged by another **Filter** from the structure.  The **Filter** requires that the user point to a boolean array at the **Feature** level that tells the **Filter** whether the **Feature** should remain in the structure.  If the boolean array is *false* for a **Feature**, then all **Cells** that belong to that **Feature** are temporarily *unassigned*. Optionally, after all *undesired* **Features** are removed, the remaining **Features** are isotropically coarsened to fill in the gaps left by the removed **Features**.

| Operation | Meaning |
|-----------|---------|
| 0 | Remove features. |
| 1 | Extract features into new geometry. |
| 2 | Extract features and then remove them. |

## WARNING: NeighborList Removal

If the operation is [0] or [2] and the Cell Feature AttributeMatrix contains any *NeighborList* data arrays, those arrays will be **REMOVED** because those lists are now invalid. Re-run the *Find Neighbors* filter to re-create the lists.

## Caveats

This filter will **ONLY** run on an Image Geometry.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
