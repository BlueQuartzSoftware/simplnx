# ImportDeformKeyFilev12

## Group (Subgroup)

SimulationIO (SimulationIO)

## Description

This **Filter** reads DEFORM v12 key files and saves the data in a newly created **Data Container**.

It reads the quadrilateral mesh data (nodal coordinates and connectivity), and the value of variables such as stress, strain, ndtmp, etc at cells and nodes.

## Parameters

| Name | Type | Description |
|------|------|------|
| Input File | Path | Name and address of the input DEFORM v12 key file |

## Required Geometry

Not Applicable

## Required Objects

## Created Objects

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|-------------|---------|-----|
| **QuadGeom** | Quad Geometry | QuadGeom | N/A | Created **QuadGeom** path and name |
| **Attribute Matrix** | Vertex Data | Vertex | N/A | Created **Vertex Attribute Matrix** name |
| **Attribute Matrix** | Cell Data | Cell | N/A | Created **Cell Attribute Matrix** name |

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: complex
+ Class Name: ImportDeformKeyFileV12Filter
+ Displayed Name: Import Deform Key File (v12)

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| cell_attribute_matrix_name | Cell Data Name | The name of the created Attribute Matrix for cell data | complex.DataObjectNameParameter |
| input_file_path | Input File | File path that points to the imported file | complex.FileSystemPathParameter |
| quad_geom_path | Quad Geometry | The created Quad Geometry from  imported from file | complex.DataGroupCreationParameter |
| vertex_attribute_matrix_name | Vertex Data Name | The name of the created Attribute Matrix for vertex data | complex.DataObjectNameParameter |

