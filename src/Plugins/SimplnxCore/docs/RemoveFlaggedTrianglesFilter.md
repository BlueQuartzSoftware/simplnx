# Remove Flagged Triangles

## Group (Subgroup)

Surface Meshing (Misc)

## Description

This **Filter** removes **Triangles** from the supplied **Triangle Geometry** that are flagged by a boolean mask array.  
Specifically, **Triangles** flagged as _true_ are removed from the **Geometry**.  A new reduced **Geometry** is created
that contains all the remaining **Triangles**.  It is unknown until run time how many **Triangles** will be removed from the
**Geometry**. Therefore, this **Filter** requires that a new **TriangleGeom** be created to contain the reduced **Triangle Geometry**.  
This new **Geometry** will NOT contain copies of any **Feature** or **Ensemble** **Attribute Matrices** from the original **Geometry**.  
Additionally, all **Vertex** data will be copied, with tuples _removed_ for any **Vertices** removed by the **Filter**.  
The user must supply a name for the reduced **Geometry**.

The mask is expected to be over the triangles themselves so it should be based on something from the **_Face Data_** **Attribute Matrix**, generally we suggest basing the mask on the created **_Region Ids_** array from the _Label Triangle Geometry Filter_.

_Note:_ Since it cannot be known before run time how many **Vertices** will be removed, the new **Vertex Geometry** and
all associated **Vertex** data to be copied will be initialized to have size 0.

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
