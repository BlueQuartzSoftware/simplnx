# Rename DataObject

## Group (Subgroup)

Core (Memory/Management)

## Description

This **Filter** renames a single **Data Object** of any type (geometry, attribute matrix, data group, data array, neighbor list, string array, etc.). The object remains at its original parent in the **Data Structure**; only the leaf name changes.

### When to Use This Filter

- Standardize names imported from external files (e.g., rename "Phi1, PHI, Phi2" to "EulerAngles_0, _1, _2" before using a multi-component-array filter).
- Resolve a name collision before adding a new object with the same name (alternatively, use [Delete Data](DeleteDataFilter.md) to remove the colliding object).
- Make pipeline output names match the expectations of a downstream consumer.

## WARNING: Downstream References Do Not Auto-Update

If any filter later in the pipeline references the renamed object by its old name, that filter will fail preflight when re-run. Re-select the renamed object in any downstream filter parameters.

### Name Constraints

The new name must be non-empty and must not contain `/`. Invalid names produce a preflight error before the Data Structure changes. When **Allow Overwrite** is disabled, the new name must not collide with another object under the same parent.

When **Allow Overwrite** is enabled, the filter replaces the object at the new name under the selected parent. Other placements and their children are kept, including placements at the root. Objects left with no placement are deleted. This rule also applies to each descendant of a deleted object. A name clash under another placement of the source object is an error. Choose a different name or remove that conflicting object first.

### Required Input Sources

- **Data Object to Rename** -- a path to any existing **DataObject** in the Data Structure.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
