# Move Data

## Group (Subgroup)

Core (Memory/Management)

## Description

This **Filter** moves one or more **Data Objects** to a new parent **Group** (a **DataGroup**, **Attribute Matrix**, or **Geometry**). The objects retain their names; only their parent changes.

### Tuple Compatibility When Moving Arrays

When moving a **Data Array** *into* an **Attribute Matrix**, the destination must be tuple-compatible: the **number of tuples** of the array must equal the number of tuples implied by the destination matrix's tuple dimensions. The *shape* of the tuple dimensions does not need to match -- moving a 1-D array of 60 tuples into a (3, 4, 5) Attribute Matrix is allowed, because both equal 60 total tuples. Moving the same array into a (10, 10) matrix would fail (only 100 total tuples).

When moving between non-Attribute-Matrix parents (e.g., between two DataGroups), tuple matching is not enforced because DataGroups have no tuple-dimension contract.

### When to Use This Filter

- Reorganize pipeline output so related arrays end up in one matrix.
- Move a computed array (e.g., output of a custom filter that landed in a temporary group) into the proper Cell/Feature/Ensemble matrix where downstream filters expect to find it.
- Move a Geometry under a DataGroup to organize multiple geometries.

### Required Input Sources

- **Objects to Move** -- a list of paths to existing **DataObjects**.
- **New Parent Path** -- an existing **DataGroup**, **Attribute Matrix**, or **Geometry**.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
