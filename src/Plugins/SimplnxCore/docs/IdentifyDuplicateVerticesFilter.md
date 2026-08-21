# Identify Duplicate Vertices

## Group (Subgroup)

Meshing (Cleanup)

## Description

This filter takes a geometry with a **SharedVertexList** and produces a `uint8` mask that contains `1` at the position of each vertex that duplicates an earlier vertex, and `0` otherwise. Two vertices are considered **duplicates** when their X, Y, and Z coordinates are all equal (the comparison treats coordinate differences smaller than machine epsilon as equal, so this is effectively exact coordinate equality); no user-supplied tolerance is applied.

It should be noted that this filter utilizes quicksort to speed up checks. Quicksort is not a stable sort, meaning the vertex deemed "unique" (i.e., not labeled as a duplicate) is not guaranteed to be the first instance of that coordinate in the original list. See the example below, which uses small text arrays to illustrate this behavior. The intention is for this filter's output to be used as the input for [Remove Flagged Vertices](RemoveFlaggedVerticesFilter.md).

## Example

### Input

Vertex Array

```console
{{1.0f, 1.0f, 1.0f}, {2.0f, 2.0f, 2.0f}, {1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f}}
```

### Possible Output

Mask Array

```console
{0,0,1,0}
```

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
