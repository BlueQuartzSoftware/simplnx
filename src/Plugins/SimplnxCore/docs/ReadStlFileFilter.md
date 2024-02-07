# Read STL File

## Group (Subgroup)

IO (Input)

## Description

This **Filter**  will read a binary STL File and create a **Triangle Geometry** object in memory. The STL reader is very strict to the STL specification. An explanation of the STL file format can be found on [Wikipedia](https://en.wikipedia.org/wiki/STL). The structure of the file is as follows:

    UINT8[80]     Header
    UINT32     Number of triangles

    foreach triangle
    REAL32[3]     Normal vector
    REAL32[3]     Vertex 1
    REAL32[3]     Vertex 2
    REAL32[3]     Vertex 3
    UINT16     Attribute byte count
    end

**It is very important that the "Attribute byte Count" is correct as DREAM.3D follows the specification strictly.** If you are writing an STL file be sure that the value for the "Attribute byte count" is *zero* (0). If you chose to encode additional data into a section after each triangle then be sure that the "Attribute byte count" is set correctly. DREAM.3D will obey the value located in the "Attribute byte count".

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
