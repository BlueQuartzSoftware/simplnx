# Delete Data

## Group (Subgroup)

Core (Memory/Management)

## Description

This **Filter** removes one or more **Data Objects** from the **Data Structure**. Common use cases:

- **Free memory.** When subsequent filters need as much RAM as possible and some arrays are no longer needed.
- **Avoid name collisions.** DREAM3DNX does not allow two objects at the same hierarchy level to share the same name. If a downstream filter will create an object with the same name as an existing one, delete the existing one first.
- **Clean up after extraction.** When *Move* or *Extract* options on other filters left behind unused parent containers.

### Cascade Behavior

Deletion is recursive on containers. Deleting a:

- **Geometry** also deletes its child Attribute Matrices and all arrays inside them.
- **Attribute Matrix** also deletes all child arrays.
- **DataGroup** also deletes all of its children, including nested DataGroups, recursively.
- **Data Array** removes only that array.

## WARNING: Downstream Preflight Will Fail

Any filter later in the pipeline that references a deleted object by path will fail preflight when re-run. Delete only objects you are certain are not used downstream, or be prepared to update downstream filter parameters.

### Required Input Sources

- **Data Objects to Delete** -- a list of paths to existing **DataObjects** in the Data Structure.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
