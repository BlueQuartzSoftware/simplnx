# Identify Duplicate Vertices

## Group (Subgroup)

Meshing (Cleanup)

## Description

This filter takes a geometry with a **SharedVertexList** and produces a `uint8` mask that contains `1` in positions where a duplicate of an existing vertex exists. It should be noted that this filter utilizes quicksort to speed up checks, quick sort is not a stable sort meaning the vertex point deemed "unique" (ie not labeled as a duplicate) is not guaranteed to be the first instance of the vertex point. See example section below for a visual explanation. The intention is for this filters output to be used as the input for *RemoveFlaggedVertices*.

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
