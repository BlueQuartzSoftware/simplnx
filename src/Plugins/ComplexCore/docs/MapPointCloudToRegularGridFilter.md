Map Point Cloud to Regular Grid
=============

## Group (Subgroup)

Sampling (Mapping)

## Description

This **Filter** determines, for a user-defined grid, in which voxel each point in a **Vertex Geometry** lies.  The user can either construct a sampling grid by specifying the dimensions, or select a pre-existing **Image Geometry** to use as the sampling grid.  The voxel indices that each point lies in are stored on the vertices.  

Additionally, the user may opt to use a mask; points for which the mask are false are ignored when computing voxel indices (instead, they are initialized to voxel 0).

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| Sampling Grid Type | Enumeration | The method used to create the sampling grid, either *Manual* or *Use Existing Image Geometry* |
| Grid Dimensions | int 3x | Dimensions of the sampling grid, if *Manual* is selected |
| Use Mask | bool | Whether to use a mask for the input **Vertex Geometry |

## Required Geometry

Vertex

## Required Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Data Container | None | N/A | N/A | Data Container holding the input **Vertex Geometry |
| Data Container | None | N/A | N/A | Data Container holding the sampling **Image Geometry**, if *Use Existing Image Geometry* is selected |
| Vertex Attribute Array | None | bool | (1) | Vertex mask, if *Use Mask* is selected |

## Created Objects

| Kind                      | Default Name | Type     | Comp Dims | Description                                 |
|---------------------------|--------------|----------|--------|---------------------------------------------|
| Vertex Attribute Array | VoxelIndices | size_t | (1) | Indices of the voxels in which each point lies |

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
