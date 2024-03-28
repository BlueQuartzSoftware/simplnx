# VTK STRUCTURED_POINTS Importer

## Group (Subgroup)

IO (Input)

## Description

This **Filter** reads a _STRUCTURED_POINTS_ type of 3D array from a legacy .vtk file. A _STRUCTURED_POINTS_ file is a more general type of **Image Geometry** where data can be stored at the vertices of each voxel. The currently supported VTK dataset attribute types are SCALARS and VECTORS. Other dataset attributes will not be read correctly and may cause issues when running the **Filter**. The VTK data must be _POINT_DATA_ and/or _CELL_DATA_ and can be either binary or ASCII. The **Filter** will create a new **Data Container** with an **Image** geometry for each of the types of data (i.e., _POINT_DATA_ and/or _CELL_DATA_) selected to be read, along with a **Cell Attribute Matrix** to hold the imported data.

*Note:* In a _STRUCTURED_POINTS_ file, _POINT_DATA_ lies on the vertices of each unit element voxel (i.e., eight values per voxel), while _CELL_DATA_ lies at the voxel center.  This Filter will import *both* types of data as **Image Geometries**, since either form a structured rectilinear grid.  This is to enable easier visualization of the _POINT_DATA_, and to enable greater flexibility when using DREAM.3D analysis tools, many of which rely on an **Image Geometry**.

### Example Input

    # vtk DataFile Version 2.0
    GrainIds Stored in Vtk File
    ASCII
    DATASET STRUCTURED_POINTS
    DIMENSIONS 3 4 6
    SPACING 1 1 1
    ORIGIN 0 0 0
    POINT_DATA 72
    SCALARS GrainIds char 1
    LOOKUP_TABLE default
    0 0 0 0 0 0 0 0 0 0 0 0
    0 5 10 15 20 25 25 20 15
    10 5 0 0 10 20 30 40 50
    50 40 30 20 10 0 0 10 20
    30 40 50 50 40 30 20 10 0
    0 5 10 15 20 25 25 20 15
    10 5 0 0 0 0 0 0 0 0 0 0 0 0 0

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**
