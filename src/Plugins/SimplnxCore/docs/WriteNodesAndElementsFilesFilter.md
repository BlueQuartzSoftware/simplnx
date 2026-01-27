# Write Nodes and Elements File(s)

## Group (Subgroup)

IO (Output)

## Description

This **Filter** exports node based geometries into structured text files.  It allows users to save the following:

1. Node Data (Vertices): Export the coordinates of points that define the geometry.
2. Element Data (Connectivity): Export the node indices that make up the edges, faces, or volumes of the geometry.

The filter gives the user the option to export a node file, element file, or both.  It also allows the user to decide whether or not to number the nodes and elements and whether or not to include headers for the node and element files.

## Example Node File

In this example output, the following filter parameter options have been selected:

- Number Nodes: ON
- Include Node File Header: ON

The output is listed below where column 1 is controlled by the `Number Nodes` parameter and line #2 is controlled by the `Include Node File Header` parameter.
```
# This file was created by simplnx v1.7.0
NODE_NUM X Y Z
0 0.0000 0.0000 0.0000
1 0.0000 0.2500 0.0000
2 0.2500 0.0000 0.0000
3 0.2500 0.2500 0.0000
4 0.5000 0.0000 0.0000
```

## Example Elements File

In this example output, the following filter parameter options have been selected:

- Number Elements/Cells: ON
- Include Element/Cell File Header: ON

The output is listed below where column 1 is controlled by the `Number Elements/Cells` parameter and line #2 is controlled by the `Include Element/Cell File Header` parameter.

Some explanation as to the columns is requried for this output. 

- Column #1 is just the index of the element
- Column #2 is the number of vertices in this element. in this output, 3 since this was exported from a Triangle Geometry
- Columns #3,4,5 are the indices into the node array of the 3 vertices that make up this element. The number of columns may change depending on the kind of geometry being written.

```
# This file was created by simplnx v1.7.0
ELEMENT_NUM NUM_VERTS_IN_ELEMENT V0_Index V1_Index V2_Index
0 3 0 1 2
1 3 2 1 3
2 3 2 3 4
3 3 4 3 5
4 3 4 5 6
5 3 6 5 7
6 3 6 7 8
7 3 8 7 9
8 3 8 9 10
9 3 10 9 11
10 3 10 11 12
```

### Number of Nodes for each Geometry Type

| Geometry Type | Number of Nodes |
|---------------|-----------------|
| Edge          | 2               |
| Triangle      | 3               |
| Quad          | 4               |
| Tetrahedral   | 4               |
| Hexahedral    | 6               |


% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
