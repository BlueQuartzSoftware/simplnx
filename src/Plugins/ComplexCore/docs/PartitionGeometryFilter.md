# Partition Geometry #

## Group (Subgroup) ##

Reconstruction (Reconstruction)

## Description ##

This **Filter** splits a given geometry into partitions using the partition's X, Y, and Z bounds, grid dimensions, origin, and spacing.

If the **Filter** determines that any voxel or node of a partition is out-of-bounds compared to the original geometry, the **Out-Of-Bounds Partition ID** will be used as the partition ID in the output partition IDs array.

There are four available modes to choose from:
+ **Basic** - Just provide the starting ID and the number of partitions per axis, and the filter will automatically create a partitioning scheme that fits your geometry data.
+ **Advanced** - Provide the starting ID, out-of-bounds ID, number of partitions per axis, partitioning scheme origin, and length per partition, and the filter will create a partitioning scheme using these inputs.  It IS possible, using this scheme, to create a partitioning scheme that does not completely fit the input geometry.
+ **Bounding Box** - Provide the starting ID, number of partitions per axis, lower left coordinate (**ll**), and upper right coordinate (**ur**), and the filter will create a partitioning scheme that has bounds [**ll**,**ur**).  It IS possible, using this scheme, to create a partitioning scheme that does not completely fit the input geometry.  **Note**: Voxels/nodes that are exactly on the edge between two partitions will be labeled with the upper partition's ID, or if they are on the upper edge of the partitioning scheme, they will be labeled with the **Out-Of-Bounds Partition ID**.

+ **Existing Partitioning Scheme** - Provide an existing partitioning scheme to partition the data.

## Parameters ##

| Name | Type | Description |
|------|------|------|
| Geometry to Partition | DataPath | The path to the geometry object that the filter will partition. |
| Attribute Matrix | DataPath | The path to the attribute matrix where the partition ids array will be stored. |
| Partitioning Mode | int | The mode used to partition the geometry.  The choices are Basic, Advanced, Bounding Box, and Existing Partitioning Scheme. |
| Starting Partition ID | int | The starting ID to use when creating the partitioning scheme. |
| Out-Of-Bounds Value | int | The default value used when the voxel or node in a partition is out-of-bounds compared to the original geometry. Only available in Advanced and Bounding Box modes. |
| Number of Partitions Per Axis (X,Y,Z) | IntVec3 | The number of partitions that will be created in each axis. |
| Partitioning Scheme Origin (X,Y,Z) | FloatVec3 | The origin of the partitioning scheme. Only available in Advanced mode. |
| Length Per Partition (X,Y,Z) | FloatVec3 | The length in each axis for each partition. Only available in Advanced mode. |
| Lower Left Coordinate (X,Y,Z) | FloatVec3 | The lower left coordinate for the bounding box of the partitioning scheme. Only available in Bounding Box mode. |
| Upper Right Coordinate (X,Y,Z) | FloatVec3 | The upper right coordinate for the bounding box of the partitioning scheme. Only available in Bounding Box mode. |
| Existing Partitioning Scheme | DataPath | The path to an image geometry that is an existing partitioning scheme. Only available in Existing Partitioning Scheme mode. |
| Use Vertex Mask | bool | Determines whether or not to use a vertex mask when partitioning the input geometry. |
| Vertex Mask | DataPath | The path to the vertex mask array. |
| Feature Attribute Matrix Name | string | The name of the feature attribute matrix that will be created. |
| Partition Ids Array Name | string | The name of the partition ids array that will be created. |
| Partitioning Scheme Geometry | DataPath | The path to the partitioning scheme geometry that will be created. |
| Partitioning Scheme Attribute Matrix | string | The name of the attribute matrix that will be created as a child of the partitioning scheme geometry. |
| Partitioning Scheme Ids | string | The name of the partitioning scheme ids array that will be created as a child of the Partitioning Scheme Attribute Matrix. |

## Example Pipelines ##

None

## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users