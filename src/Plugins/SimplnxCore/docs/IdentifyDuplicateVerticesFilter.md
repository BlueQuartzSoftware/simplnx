# Identify Duplicate Vertices

## Group (Subgroup)

Meshing (Cleanup)

## Description

This filter takes a geometry with a **SharedVertexList** and produces a `uint8` mask. Two vertices are duplicates only when their X, Y, and Z coordinates are exactly equal. No tolerance is used. Negative zero and positive zero are equal. A vertex with any NaN coordinate is never a duplicate.

For each group of equal coordinates, the first vertex in input order gets `0`. All later copies get `1`. Other vertices get `0`. Use this mask as the input for [Remove Flagged Vertices](RemoveFlaggedVerticesFilter.md).

## Example

### Input

Vertex Array

```console
{{1.0f, 1.0f, 1.0f}, {2.0f, 2.0f, 2.0f}, {1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f}}
```

### Output

Mask Array

```console
{0,0,1,0}
```

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
